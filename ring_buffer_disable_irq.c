/**
 * @file    ring_buffer_disable_irq.c
 * @brief   环形缓冲区关中断实现（增强版）
 * @author  CRITTY.熙影
 * @date    2024-12-27
 * @version 2.2
 * 
 * @details
 * 适用场景：
 * - 裸机系统（无 RTOS）
 * - 多个中断源共享缓冲区
 * - 中断与多任务之间通信
 * 
 * 线程安全保证：
 * - 通过关闭中断创建临界区
 * - 适用于 Cortex-M 等单核 MCU
 * 
 * @warning
 * - 会增加中断延迟
 * - 不适用于多核系统
 * 
 * @note 版本 2.2 改进:
 *       - 在关中断前增加参数校验(关键修复!)
 *       - 增强日志系统
 */

#include "ring_buffer.h"

#if RING_BUFFER_ENABLE_DISABLE_IRQ

/* 中断控制宏在 ring_buffer_config.h 中定义 */

/* 复用无锁实现的内部逻辑 */
extern const struct ring_buffer_ops ring_buffer_lockfree_ops;

/* Exported functions (Implementation) ---------------------------------------*/

static bool disable_irq_write(ring_buffer_t *rb, uint8_t data)
{
    /* 关键修复: 必须在关中断前进行参数校验! */
    if (!rb) {
        RB_LOG_ERROR("rb is NULL");
        return false;
    }
    
    if (!rb->buffer) {
        RB_LOG_ERROR("buffer is NULL (rb=%p)", rb);
        return false;
    }
    
    irq_state_t state;
    IRQ_SAVE(state);
    
    bool ret = ring_buffer_lockfree_ops.write(rb, data);
    
    IRQ_RESTORE(state);
    return ret;
}

static bool disable_irq_read(ring_buffer_t *rb, uint8_t *data)
{
    /* 关键修复: 必须在关中断前进行参数校验! */
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
    
    irq_state_t state;
    IRQ_SAVE(state);
    
    bool ret = ring_buffer_lockfree_ops.read(rb, data);
    
    IRQ_RESTORE(state);
    return ret;
}

static uint16_t disable_irq_write_multi(ring_buffer_t *rb, const uint8_t *data, uint16_t len)
{
    /* 关键修复: 必须在关中断前进行参数校验! */
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
    
    irq_state_t state;
    IRQ_SAVE(state);
    
    uint16_t ret = ring_buffer_lockfree_ops.write_multi(rb, data, len);
    
    IRQ_RESTORE(state);
    return ret;
}

static uint16_t disable_irq_read_multi(ring_buffer_t *rb, uint8_t *data, uint16_t len)
{
    /* 关键修复: 必须在关中断前进行参数校验! */
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
    
    irq_state_t state;
    IRQ_SAVE(state);
    
    uint16_t ret = ring_buffer_lockfree_ops.read_multi(rb, data, len);
    
    IRQ_RESTORE(state);
    return ret;
}

static uint16_t disable_irq_available(const ring_buffer_t *rb)
{
    /* 关键修复: 必须在关中断前进行参数校验! */
    if (!rb) {
        RB_LOG_ERROR("rb is NULL");
        return 0;
    }
    
    irq_state_t state;
    IRQ_SAVE(state);
    
    uint16_t ret = ring_buffer_lockfree_ops.available(rb);
    
    IRQ_RESTORE(state);
    return ret;
}

static uint16_t disable_irq_free_space(const ring_buffer_t *rb)
{
    /* 关键修复: 必须在关中断前进行参数校验! */
    if (!rb) {
        RB_LOG_ERROR("rb is NULL");
        return 0;
    }
    
    irq_state_t state;
    IRQ_SAVE(state);
    
    uint16_t ret = ring_buffer_lockfree_ops.free_space(rb);
    
    IRQ_RESTORE(state);
    return ret;
}

static bool disable_irq_is_empty(const ring_buffer_t *rb)
{
    /* 关键修复: 必须在关中断前进行参数校验! */
    if (!rb) {
        RB_LOG_ERROR("rb is NULL");
        return true;
    }
    
    irq_state_t state;
    IRQ_SAVE(state);
    
    bool ret = ring_buffer_lockfree_ops.is_empty(rb);
    
    IRQ_RESTORE(state);
    return ret;
}

static bool disable_irq_is_full(const ring_buffer_t *rb)
{
    /* 关键修复: 必须在关中断前进行参数校验! */
    if (!rb) {
        RB_LOG_ERROR("rb is NULL");
        return false;
    }
    
    irq_state_t state;
    IRQ_SAVE(state);
    
    bool ret = ring_buffer_lockfree_ops.is_full(rb);
    
    IRQ_RESTORE(state);
    return ret;
}

static void disable_irq_clear(ring_buffer_t *rb)
{
    /* 关键修复: 必须在关中断前进行参数校验! */
    if (!rb) {
        RB_LOG_ERROR("rb is NULL");
        return;
    }
    
    irq_state_t state;
    IRQ_SAVE(state);
    
    ring_buffer_lockfree_ops.clear(rb);
    
    IRQ_RESTORE(state);
    
    RB_LOG_INFO("Disable_irq buffer cleared");
}

/* Exported constant ---------------------------------------------------------*/

const ring_buffer_ops_t ring_buffer_disable_irq_ops = {
    .write       = disable_irq_write,
    .read        = disable_irq_read,
    .write_multi = disable_irq_write_multi,
    .read_multi  = disable_irq_read_multi,
    .available   = disable_irq_available,
    .free_space  = disable_irq_free_space,
    .is_empty    = disable_irq_is_empty,
    .is_full     = disable_irq_is_full,
    .clear       = disable_irq_clear,
};

#endif /* RING_BUFFER_ENABLE_DISABLE_IRQ */
