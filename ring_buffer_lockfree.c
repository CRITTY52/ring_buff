/**
 * @file    ring_buffer_lockfree.c
 * @brief   环形缓冲区无锁实现（增强版）
 * @author  CRITTY.熙影
 * @date    2024-12-27
 * @version 2.2
 * 
 * @details
 * 适用场景：
 * - 单生产者单消费者（SPSC）
 * - ISR → 主循环
 * - DMA 回调 → 任务处理
 * 
 * 线程安全保证：
 * - 无需加锁，依赖内存顺序保证
 * - 生产者只修改 head，消费者只修改 tail
 * 
 * @warning 禁止多个生产者或多个消费者同时访问
 * 
 * @note 版本 2.2 改进:
 *       - 增强日志分级(ERROR/WARN/INFO)
 *       - 完善错误日志信息
 *       - 缓冲区满时不再打印日志(正常情况)
 */

#include "ring_buffer.h"

#if RING_BUFFER_ENABLE_LOCKFREE

/* Private functions ---------------------------------------------------------*/

/**
 * @brief 计算可读数据量（内部函数，无参数校验）
 */
static inline uint16_t lockfree_available_internal(const ring_buffer_t *rb)
{
    uint16_t head = rb->head;
    uint16_t tail = rb->tail;
    
    if (head >= tail) {
        return head - tail;
    } else {
        return rb->size - tail + head;
    }
}

/**
 * @brief 计算剩余空间（内部函数，无参数校验）
 */
static inline uint16_t lockfree_free_space_internal(const ring_buffer_t *rb)
{
    return rb->size - 1 - lockfree_available_internal(rb);
}

/* Exported functions (Implementation) ---------------------------------------*/

static bool lockfree_write(ring_buffer_t *rb, uint8_t data)
{
    /* 防御性检查 */
    if (!rb) {
        RB_LOG_ERROR("rb is NULL");
        return false;
    }
    
    if (!rb->buffer) {
        RB_LOG_ERROR("buffer is NULL (rb=%p)", rb);
        return false;
    }
    
    uint16_t next_head = (rb->head + 1) % rb->size;
    
    /* 检查是否已满 */
    if (next_head == rb->tail) {
#if RING_BUFFER_ENABLE_STATISTICS
        rb->overflow_count++;
#endif
        /* 缓冲区满是正常情况,不打印错误日志 */
        return false;
    }
    
    /* 写入数据 */
    rb->buffer[rb->head] = data;
    rb->head = next_head;
    
#if RING_BUFFER_ENABLE_STATISTICS
    rb->write_count++;
#endif
    
    return true;
}

static bool lockfree_read(ring_buffer_t *rb, uint8_t *data)
{
    /* 防御性检查 */
    if (!rb) {
        RB_LOG_ERROR("rb is NULL");
        return false;
    }
    
    if (!rb->buffer) {
        RB_LOG_ERROR("buffer is NULL (rb=%p)", rb);
        return false;
    }
    
    if (!data) {
        RB_LOG_ERROR("data is NULL (rb=%p)", rb);
        return false;
    }
    
    /* 检查是否为空 */
    if (rb->tail == rb->head) {
        /* 空缓冲区是正常情况,不打印日志 */
        return false;
    }
    
    /* 读取数据 */
    *data = rb->buffer[rb->tail];
    rb->tail = (rb->tail + 1) % rb->size;
    
#if RING_BUFFER_ENABLE_STATISTICS
    rb->read_count++;
#endif
    
    return true;
}

static uint16_t lockfree_write_multi(ring_buffer_t *rb, const uint8_t *data, uint16_t len)
{
    /* 防御性检查 */
    if (!rb) {
        RB_LOG_ERROR("rb is NULL");
        return 0;
    }
    
    if (!rb->buffer) {
        RB_LOG_ERROR("buffer is NULL (rb=%p)", rb);
        return 0;
    }
    
    if (!data) {
        RB_LOG_ERROR("data is NULL (rb=%p, len=%u)", rb, len);
        return 0;
    }
    
    if (len == 0) {
        RB_LOG_WARN("len is 0");
        return 0;
    }
    
    /* 计算可写入数量 */
    uint16_t free = lockfree_free_space_internal(rb);
    uint16_t to_write = (len > free) ? free : len;
    
    if (to_write == 0) {
#if RING_BUFFER_ENABLE_STATISTICS
        rb->overflow_count++;
#endif
        /* 缓冲区满是正常情况 */
        return 0;
    }
    
    /* 快照当前状态，避免在操作过程中被修改 */
    uint16_t head = rb->head;
    uint16_t size = rb->size;
    
    /* 分段写入 */
    if (head + to_write <= size) {
        /* 单段写入 */
        memcpy(&rb->buffer[head], data, to_write);
        rb->head = (head + to_write) % size;
    } else {
        /* 双段写入（环绕） */
        uint16_t first_chunk = size - head;
        uint16_t second_chunk = to_write - first_chunk;
        
        memcpy(&rb->buffer[head], data, first_chunk);
        memcpy(&rb->buffer[0], &data[first_chunk], second_chunk);
        
        rb->head = second_chunk;
    }
    
#if RING_BUFFER_ENABLE_STATISTICS
    rb->write_count += to_write;
    if (to_write < len) {
        rb->overflow_count++;
    }
#endif
    
    /* 部分写入时打印警告 */
    if (to_write < len) {
        RB_LOG_WARN("Partial write: requested=%u, written=%u, free=%u", 
                    len, to_write, free);
    }
    
    return to_write;
}

static uint16_t lockfree_read_multi(ring_buffer_t *rb, uint8_t *data, uint16_t len)
{
    /* 防御性检查 */
    if (!rb) {
        RB_LOG_ERROR("rb is NULL");
        return 0;
    }
    
    if (!rb->buffer) {
        RB_LOG_ERROR("buffer is NULL (rb=%p)", rb);
        return 0;
    }
    
    if (!data) {
        RB_LOG_ERROR("data is NULL (rb=%p, len=%u)", rb, len);
        return 0;
    }
    
    if (len == 0) {
        RB_LOG_WARN("len is 0");
        return 0;
    }
    
    /* 计算可读取数量 */
    uint16_t available = lockfree_available_internal(rb);
    uint16_t to_read = (len > available) ? available : len;
    
    if (to_read == 0) {
        /* 空缓冲区是正常情况 */
        return 0;
    }
    
    /* 快照当前状态，避免在操作过程中被修改 */
    uint16_t tail = rb->tail;
    uint16_t size = rb->size;
    
    /* 分段读取 */
    if (tail + to_read <= size) {
        /* 单段读取 */
        memcpy(data, &rb->buffer[tail], to_read);
        rb->tail = (tail + to_read) % size;
    } else {
        /* 双段读取（环绕） */
        uint16_t first_chunk = size - tail;
        uint16_t second_chunk = to_read - first_chunk;
        
        memcpy(data, &rb->buffer[tail], first_chunk);
        memcpy(&data[first_chunk], &rb->buffer[0], second_chunk);
        
        rb->tail = second_chunk;
    }
    
#if RING_BUFFER_ENABLE_STATISTICS
    rb->read_count += to_read;
#endif
    
    /* 部分读取时打印警告 */
    if (to_read < len) {
        RB_LOG_WARN("Partial read: requested=%u, read=%u, available=%u", 
                    len, to_read, available);
    }
    
    return to_read;
}

static uint16_t lockfree_available(const ring_buffer_t *rb)
{
    /* 防御性检查 */
    if (!rb) {
        RB_LOG_ERROR("rb is NULL");
        return 0;
    }
    
    return lockfree_available_internal(rb);
}

static uint16_t lockfree_free_space(const ring_buffer_t *rb)
{
    /* 防御性检查 */
    if (!rb) {
        RB_LOG_ERROR("rb is NULL");
        return 0;
    }
    
    return lockfree_free_space_internal(rb);
}

static bool lockfree_is_empty(const ring_buffer_t *rb)
{
    /* 防御性检查 */
    if (!rb) {
        RB_LOG_ERROR("rb is NULL");
        return true;  /* 返回 true 防止误操作 */
    }
    
    return (rb->head == rb->tail);
}

static bool lockfree_is_full(const ring_buffer_t *rb)
{
    /* 防御性检查 */
    if (!rb) {
        RB_LOG_ERROR("rb is NULL");
        return false;  /* 返回 false 防止误判 */
    }
    
    return ((rb->head + 1) % rb->size == rb->tail);
}

static void lockfree_clear(ring_buffer_t *rb)
{
    /* 防御性检查 */
    if (!rb) {
        RB_LOG_ERROR("rb is NULL");
        return;
    }
    
    rb->tail = rb->head;
    
#if RING_BUFFER_ENABLE_STATISTICS
    rb->write_count = 0;
    rb->read_count = 0;
    rb->overflow_count = 0;
#endif
    
    RB_LOG_INFO("Lockfree buffer cleared");
}

/* Exported constant ---------------------------------------------------------*/

const ring_buffer_ops_t ring_buffer_lockfree_ops = {
    .write       = lockfree_write,
    .read        = lockfree_read,
    .write_multi = lockfree_write_multi,
    .read_multi  = lockfree_read_multi,
    .available   = lockfree_available,
    .free_space  = lockfree_free_space,
    .is_empty    = lockfree_is_empty,
    .is_full     = lockfree_is_full,
    .clear       = lockfree_clear,
};

#endif /* RING_BUFFER_ENABLE_LOCKFREE */
