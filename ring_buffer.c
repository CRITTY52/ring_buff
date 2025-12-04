/**
 * @file    ring_buffer.c
 * @brief   环形缓冲区工厂函数实现
 * @author  CRITTY.熙影
 * @date    2024-12-27
 * @version 2.2
 * 
 * @note 版本 2.2 改进:
 *       - 增强日志系统(ERROR/WARN/INFO三级)
 *       - 所有错误路径都有日志输出
 */

#include "ring_buffer.h"

/* External declarations -----------------------------------------------------*/
#if RING_BUFFER_ENABLE_LOCKFREE
extern const ring_buffer_ops_t ring_buffer_lockfree_ops;
#endif
#if RING_BUFFER_ENABLE_DISABLE_IRQ
extern const ring_buffer_ops_t ring_buffer_disable_irq_ops;
#endif
#if RING_BUFFER_ENABLE_MUTEX
extern const ring_buffer_ops_t ring_buffer_mutex_ops;
extern bool ring_buffer_mutex_init(ring_buffer_t *rb);
extern void ring_buffer_mutex_deinit(ring_buffer_t *rb);
#endif

/* Private types -------------------------------------------------------------*/
typedef struct {
    ring_buffer_type_t type;
    const ring_buffer_ops_t *ops;
} custom_ops_entry_t;

/* Private variables ---------------------------------------------------------*/
static custom_ops_entry_t custom_ops_registry[RING_BUFFER_MAX_CUSTOM_OPS];
static uint8_t custom_ops_count = 0;

/* Private functions ---------------------------------------------------------*/
static bool ring_buffer_init_common(ring_buffer_t *rb, uint8_t *buffer, uint16_t size)
{
    if (!rb) {
        RB_LOG_ERROR("rb is NULL");
        return false;
    }
    
    if (!buffer) {
        RB_LOG_ERROR("buffer is NULL");
        return false;
    }
    
    if (size < RING_BUFFER_MIN_SIZE) {
        RB_LOG_ERROR("size=%u < MIN_SIZE=%u", size, RING_BUFFER_MIN_SIZE);
        return false;
    }
    
    rb->buffer = buffer;
    rb->size = size;
    rb->head = 0;
    rb->tail = 0;
    rb->lock = NULL;
    rb->ops = NULL;
    
#if RING_BUFFER_ENABLE_STATISTICS
    rb->write_count = 0;
    rb->read_count = 0;
    rb->overflow_count = 0;
#endif
    
    return true;
}

static const struct ring_buffer_ops* find_custom_ops(ring_buffer_type_t type)
{
    for (uint8_t i = 0; i < custom_ops_count; i++) {
        if (custom_ops_registry[i].type == type) {
            return custom_ops_registry[i].ops;
        }
    }
    return NULL;
}

/* Exported functions --------------------------------------------------------*/
bool ring_buffer_create(
    ring_buffer_t *rb,
    uint8_t *buffer,
    uint16_t size,
    ring_buffer_type_t type)
{
    if (!ring_buffer_init_common(rb, buffer, size)) {
        return false;
    }
    
    switch (type) {
        
#if RING_BUFFER_ENABLE_LOCKFREE
        case RING_BUFFER_TYPE_LOCKFREE:
            rb->ops = &ring_buffer_lockfree_ops;
            RB_LOG_INFO("Created lockfree buffer (size=%u)", size);
            return true;
#endif
        
#if RING_BUFFER_ENABLE_DISABLE_IRQ
        case RING_BUFFER_TYPE_DISABLE_IRQ:
            rb->ops = &ring_buffer_disable_irq_ops;
            RB_LOG_INFO("Created disable_irq buffer (size=%u)", size);
            return true;
#endif
        
#if RING_BUFFER_ENABLE_MUTEX
        case RING_BUFFER_TYPE_MUTEX:
            if (!ring_buffer_mutex_init(rb)) {
                RB_LOG_ERROR("Mutex init failed");
                return false;
            }
            rb->ops = &ring_buffer_mutex_ops;
            RB_LOG_INFO("Created mutex buffer (size=%u)", size);
            return true;
#endif
        
        default:
            if (type >= RING_BUFFER_TYPE_CUSTOM_BASE) {
                const struct ring_buffer_ops *custom_ops = find_custom_ops(type);
                if (custom_ops) {
                    rb->ops = custom_ops;
                    RB_LOG_INFO("Created custom buffer (type=%d, size=%u)", type, size);
                    return true;
                }
                RB_LOG_ERROR("Custom type %d not registered", type);
                return false;
            }
            
            RB_LOG_ERROR("Unsupported type %d", type);
            return false;
    }
}

void ring_buffer_destroy(ring_buffer_t *rb)
{
    if (!rb) {
        RB_LOG_ERROR("Destroy failed: rb is NULL");
        return;
    }
    
#if RING_BUFFER_ENABLE_MUTEX
    if (rb->lock) {
        ring_buffer_mutex_deinit(rb);
    }
#endif
    
    RB_LOG_INFO("Buffer destroyed");
    
    rb->buffer = NULL;
    rb->size = 0;
    rb->head = 0;
    rb->tail = 0;
    rb->lock = NULL;
    rb->ops = NULL;
}

bool ring_buffer_register_ops(ring_buffer_type_t type, const ring_buffer_ops_t *ops)
{
    if (!ops) {
        RB_LOG_ERROR("Register failed: ops is NULL");
        return false;
    }
    
    if (type < RING_BUFFER_TYPE_CUSTOM_BASE) {
        RB_LOG_ERROR("Invalid type %d (must >= %d)", type, RING_BUFFER_TYPE_CUSTOM_BASE);
        return false;
    }
    
    if (custom_ops_count >= RING_BUFFER_MAX_CUSTOM_OPS) {
        RB_LOG_ERROR("Registry full (%d/%d)", custom_ops_count, RING_BUFFER_MAX_CUSTOM_OPS);
        return false;
    }
    
    if (find_custom_ops(type)) {
        RB_LOG_ERROR("Type %d already registered", type);
        return false;
    }
    
    custom_ops_registry[custom_ops_count].type = type;
    custom_ops_registry[custom_ops_count].ops = ops;
    custom_ops_count++;
    
    RB_LOG_INFO("Registered custom ops (type=%d, total=%d)", type, custom_ops_count);
    return true;
}

/* ==================== 便捷封装 API 实现 ==================== */

bool ring_buffer_write(ring_buffer_t *rb, uint8_t data)
{
    if (!rb) {
        RB_LOG_ERROR("rb is NULL");
        return false;
    }
    
    if (!rb->ops || !rb->ops->write) {
        RB_LOG_ERROR("ops or write is NULL");
        return false;
    }
    
    return rb->ops->write(rb, data);
}

bool ring_buffer_read(ring_buffer_t *rb, uint8_t *data)
{
    if (!rb) {
        RB_LOG_ERROR("rb is NULL");
        return false;
    }
    
    if (!data) {
        RB_LOG_ERROR("data is NULL");
        return false;
    }
    
    if (!rb->ops || !rb->ops->read) {
        RB_LOG_ERROR("ops or read is NULL");
        return false;
    }
    
    return rb->ops->read(rb, data);
}

uint16_t ring_buffer_write_multi(ring_buffer_t *rb, const uint8_t *data, uint16_t len)
{
    if (!rb) {
        RB_LOG_ERROR("rb is NULL");
        return 0;
    }
    
    if (!data) {
        RB_LOG_ERROR("data is NULL");
        return 0;
    }
    
    if (len == 0) {
        RB_LOG_WARN("len is 0");
        return 0;
    }
    
    if (!rb->ops || !rb->ops->write_multi) {
        RB_LOG_ERROR("ops or write_multi is NULL");
        return 0;
    }
    
    return rb->ops->write_multi(rb, data, len);
}

uint16_t ring_buffer_read_multi(ring_buffer_t *rb, uint8_t *data, uint16_t len)
{
    if (!rb) {
        RB_LOG_ERROR("rb is NULL");
        return 0;
    }
    
    if (!data) {
        RB_LOG_ERROR("data is NULL");
        return 0;
    }
    
    if (len == 0) {
        RB_LOG_WARN("len is 0");
        return 0;
    }
    
    if (!rb->ops || !rb->ops->read_multi) {
        RB_LOG_ERROR("ops or read_multi is NULL");
        return 0;
    }
    
    return rb->ops->read_multi(rb, data, len);
}

uint16_t ring_buffer_available(const ring_buffer_t *rb)
{
    if (!rb) {
        RB_LOG_ERROR("rb is NULL");
        return 0;
    }
    
    if (!rb->ops || !rb->ops->available) {
        RB_LOG_ERROR("ops or available is NULL");
        return 0;
    }
    
    return rb->ops->available(rb);
}

uint16_t ring_buffer_free_space(const ring_buffer_t *rb)
{
    if (!rb) {
        RB_LOG_ERROR("rb is NULL");
        return 0;
    }
    
    if (!rb->ops || !rb->ops->free_space) {
        RB_LOG_ERROR("ops or free_space is NULL");
        return 0;
    }
    
    return rb->ops->free_space(rb);
}

bool ring_buffer_is_empty(const ring_buffer_t *rb)
{
    if (!rb) {
        RB_LOG_ERROR("rb is NULL");
        return true;
    }
    
    if (!rb->ops || !rb->ops->is_empty) {
        RB_LOG_ERROR("ops or is_empty is NULL");
        return true;
    }
    
    return rb->ops->is_empty(rb);
}

bool ring_buffer_is_full(const ring_buffer_t *rb)
{
    if (!rb) {
        RB_LOG_ERROR("rb is NULL");
        return false;
    }
    
    if (!rb->ops || !rb->ops->is_full) {
        RB_LOG_ERROR("ops or is_full is NULL");
        return false;
    }
    
    return rb->ops->is_full(rb);
}

void ring_buffer_clear(ring_buffer_t *rb)
{
    if (!rb) {
        RB_LOG_ERROR("rb is NULL");
        return;
    }
    
    if (!rb->ops || !rb->ops->clear) {
        RB_LOG_ERROR("ops or clear is NULL");
        return;
    }
    
    rb->ops->clear(rb);
}
