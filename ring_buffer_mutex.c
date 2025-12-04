/**
 * @file    ring_buffer_mutex.c
 * @brief   环形缓冲区互斥锁实现（增强版）
 * @author  CRITTY.熙影
 * @date    2024-12-27
 * @version 2.2
 * 
 * @details
 * 适用场景：
 * - FreeRTOS / RT-Thread / μC/OS 等 RTOS 环境
 * - 多线程之间的缓冲区共享
 * - 需要阻塞等待的场景
 * 
 * 线程安全保证：
 * - 使用 RTOS 互斥锁（Mutex）保护
 * - 支持优先级继承（防止优先级反转）
 * 
 * @warning 不可在 ISR 中使用
 * 
 * @note 版本 2.2 改进:
 *       - 在加锁前增加参数校验(关键修复!)
 *       - 增强日志系统
 */

#include "ring_buffer.h"

#if RING_BUFFER_ENABLE_MUTEX

/* RTOS 互斥锁宏在 ring_buffer_config.h 中定义 */

/* 复用无锁实现的内部逻辑 */
extern const struct ring_buffer_ops ring_buffer_lockfree_ops;

/* Exported functions (for factory) ------------------------------------------*/

bool ring_buffer_mutex_init(ring_buffer_t *rb)
{
    if (!rb) {
        RB_LOG_ERROR("rb is NULL");
        return false;
    }
    
    mutex_t mutex = MUTEX_CREATE();
    if (!MUTEX_IS_VALID(mutex)) {
        RB_LOG_ERROR("Mutex create failed (rb=%p)", rb);
        return false;
    }
    
    rb->lock = (void*)mutex;
    RB_LOG_INFO("Mutex created successfully");
    return true;
}

void ring_buffer_mutex_deinit(ring_buffer_t *rb)
{
    if (!rb) {
        RB_LOG_ERROR("rb is NULL");
        return;
    }
    
    if (!rb->lock) {
        RB_LOG_WARN("lock is NULL (rb=%p), nothing to delete", rb);
        return;
    }
    
    mutex_t mutex = (mutex_t)rb->lock;
    MUTEX_DELETE(mutex);
    rb->lock = NULL;
    
    RB_LOG_INFO("Mutex deleted");
}

/* Exported functions (Implementation) ---------------------------------------*/

static bool mutex_write(ring_buffer_t *rb, uint8_t data)
{
    /* 关键修复: 必须在加锁前进行参数校验! */
    if (!rb) {
        RB_LOG_ERROR("rb is NULL");
        return false;
    }
    
    if (!rb->lock) {
        RB_LOG_ERROR("lock is NULL (rb=%p)", rb);
        return false;
    }
    
    if (!rb->buffer) {
        RB_LOG_ERROR("buffer is NULL (rb=%p)", rb);
        return false;
    }
    
    mutex_t mutex = (mutex_t)rb->lock;
    MUTEX_LOCK(mutex);
    
    bool ret = ring_buffer_lockfree_ops.write(rb, data);
    
    MUTEX_UNLOCK(mutex);
    return ret;
}

static bool mutex_read(ring_buffer_t *rb, uint8_t *data)
{
    /* 关键修复: 必须在加锁前进行参数校验! */
    if (!rb) {
        RB_LOG_ERROR("rb is NULL");
        return false;
    }
    
    if (!data) {
        RB_LOG_ERROR("data is NULL (rb=%p)", rb);
        return false;
    }
    
    if (!rb->lock) {
        RB_LOG_ERROR("lock is NULL (rb=%p)", rb);
        return false;
    }
    
    if (!rb->buffer) {
        RB_LOG_ERROR("buffer is NULL (rb=%p)", rb);
        return false;
    }
    
    mutex_t mutex = (mutex_t)rb->lock;
    MUTEX_LOCK(mutex);
    
    bool ret = ring_buffer_lockfree_ops.read(rb, data);
    
    MUTEX_UNLOCK(mutex);
    return ret;
}

static uint16_t mutex_write_multi(ring_buffer_t *rb, const uint8_t *data, uint16_t len)
{
    /* 关键修复: 必须在加锁前进行参数校验! */
    if (!rb) {
        RB_LOG_ERROR("rb is NULL");
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
    
    if (!rb->lock) {
        RB_LOG_ERROR("lock is NULL (rb=%p)", rb);
        return 0;
    }
    
    if (!rb->buffer) {
        RB_LOG_ERROR("buffer is NULL (rb=%p)", rb);
        return 0;
    }
    
    mutex_t mutex = (mutex_t)rb->lock;
    MUTEX_LOCK(mutex);
    
    uint16_t ret = ring_buffer_lockfree_ops.write_multi(rb, data, len);
    
    MUTEX_UNLOCK(mutex);
    return ret;
}

static uint16_t mutex_read_multi(ring_buffer_t *rb, uint8_t *data, uint16_t len)
{
    /* 关键修复: 必须在加锁前进行参数校验! */
    if (!rb) {
        RB_LOG_ERROR("rb is NULL");
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
    
    if (!rb->lock) {
        RB_LOG_ERROR("lock is NULL (rb=%p)", rb);
        return 0;
    }
    
    if (!rb->buffer) {
        RB_LOG_ERROR("buffer is NULL (rb=%p)", rb);
        return 0;
    }
    
    mutex_t mutex = (mutex_t)rb->lock;
    MUTEX_LOCK(mutex);
    
    uint16_t ret = ring_buffer_lockfree_ops.read_multi(rb, data, len);
    
    MUTEX_UNLOCK(mutex);
    return ret;
}

static uint16_t mutex_available(const ring_buffer_t *rb)
{
    /* 关键修复: 必须在加锁前进行参数校验! */
    if (!rb) {
        RB_LOG_ERROR("rb is NULL");
        return 0;
    }
    
    if (!rb->lock) {
        RB_LOG_ERROR("lock is NULL (rb=%p)", rb);
        return 0;
    }
    
    mutex_t mutex = (mutex_t)rb->lock;
    MUTEX_LOCK(mutex);
    
    uint16_t ret = ring_buffer_lockfree_ops.available(rb);
    
    MUTEX_UNLOCK(mutex);
    return ret;
}

static uint16_t mutex_free_space(const ring_buffer_t *rb)
{
    /* 关键修复: 必须在加锁前进行参数校验! */
    if (!rb) {
        RB_LOG_ERROR("rb is NULL");
        return 0;
    }
    
    if (!rb->lock) {
        RB_LOG_ERROR("lock is NULL (rb=%p)", rb);
        return 0;
    }
    
    mutex_t mutex = (mutex_t)rb->lock;
    MUTEX_LOCK(mutex);
    
    uint16_t ret = ring_buffer_lockfree_ops.free_space(rb);
    
    MUTEX_UNLOCK(mutex);
    return ret;
}

static bool mutex_is_empty(const ring_buffer_t *rb)
{
    /* 关键修复: 必须在加锁前进行参数校验! */
    if (!rb) {
        RB_LOG_ERROR("rb is NULL");
        return true;
    }
    
    if (!rb->lock) {
        RB_LOG_ERROR("lock is NULL (rb=%p)", rb);
        return true;
    }
    
    mutex_t mutex = (mutex_t)rb->lock;
    MUTEX_LOCK(mutex);
    
    bool ret = ring_buffer_lockfree_ops.is_empty(rb);
    
    MUTEX_UNLOCK(mutex);
    return ret;
}

static bool mutex_is_full(const ring_buffer_t *rb)
{
    /* 关键修复: 必须在加锁前进行参数校验! */
    if (!rb) {
        RB_LOG_ERROR("rb is NULL");
        return false;
    }
    
    if (!rb->lock) {
        RB_LOG_ERROR("lock is NULL (rb=%p)", rb);
        return false;
    }
    
    mutex_t mutex = (mutex_t)rb->lock;
    MUTEX_LOCK(mutex);
    
    bool ret = ring_buffer_lockfree_ops.is_full(rb);
    
    MUTEX_UNLOCK(mutex);
    return ret;
}

static void mutex_clear(ring_buffer_t *rb)
{
    /* 关键修复: 必须在加锁前进行参数校验! */
    if (!rb) {
        RB_LOG_ERROR("rb is NULL");
        return;
    }
    
    if (!rb->lock) {
        RB_LOG_ERROR("lock is NULL (rb=%p)", rb);
        return;
    }
    
    mutex_t mutex = (mutex_t)rb->lock;
    MUTEX_LOCK(mutex);
    
    ring_buffer_lockfree_ops.clear(rb);
    
    MUTEX_UNLOCK(mutex);
    
    RB_LOG_INFO("Mutex buffer cleared");
}

/* Exported constant ---------------------------------------------------------*/

const ring_buffer_ops_t ring_buffer_mutex_ops = {
    .write       = mutex_write,
    .read        = mutex_read,
    .write_multi = mutex_write_multi,
    .read_multi  = mutex_read_multi,
    .available   = mutex_available,
    .free_space  = mutex_free_space,
    .is_empty    = mutex_is_empty,
    .is_full     = mutex_is_full,
    .clear       = mutex_clear,
};

#endif /* RING_BUFFER_ENABLE_MUTEX */
