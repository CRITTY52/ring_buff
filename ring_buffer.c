/**
 * @file    ring_buffer.c
 * @brief   环形缓冲区工厂函数实现
 * @author  CRITTY.熙影
 * @date    2024-12-27
 * @version 2.1
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
    if (!rb || !buffer || size < RING_BUFFER_MIN_SIZE) 
	{
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
    for (uint8_t i = 0; i < custom_ops_count; i++) 
	{
        if (custom_ops_registry[i].type == type) 
		{
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
    if (!ring_buffer_init_common(rb, buffer, size)) 
	{
        return false;
    }
    
    switch (type) 
	{
        
#if RING_BUFFER_ENABLE_LOCKFREE
        case RING_BUFFER_TYPE_LOCKFREE:
            rb->ops = &ring_buffer_lockfree_ops;
            RB_LOG("Created lockfree buffer (size=%u)", size);
            return true;
#endif
        
#if RING_BUFFER_ENABLE_DISABLE_IRQ
        case RING_BUFFER_TYPE_DISABLE_IRQ:
            rb->ops = &ring_buffer_disable_irq_ops;
            RB_LOG("Created disable_irq buffer (size=%u)", size);
            return true;
#endif
        
#if RING_BUFFER_ENABLE_MUTEX
        case RING_BUFFER_TYPE_MUTEX:
            if (!ring_buffer_mutex_init(rb)) 
		    {
                return false;  // 互斥锁创建失败
            }
            rb->ops = &ring_buffer_mutex_ops;
            RB_LOG("Created mutex buffer (size=%u)", size);
            return true;
#endif
        
        default:
            if (type >= RING_BUFFER_TYPE_CUSTOM_BASE) 
			{
                const struct ring_buffer_ops *custom_ops = find_custom_ops(type);
                if (custom_ops) 
				{
                    rb->ops = custom_ops;
                    RB_LOG("Created custom buffer (type=%d, size=%u)", type, size);
                    return true;
                }
            }
            
            RB_LOG("Create failed: unsupported type %d", type);
            return false;
    }
}

void ring_buffer_destroy(ring_buffer_t *rb)
{
    if (!rb) 
	{
        return;
    }
    
#if RING_BUFFER_ENABLE_MUTEX
    if (rb->lock) 
	{
        ring_buffer_mutex_deinit(rb);
    }
#endif
    
    RB_LOG("Destroyed buffer");
    
    rb->buffer = NULL;
    rb->size = 0;
    rb->head = 0;
    rb->tail = 0;
    rb->lock = NULL;
    rb->ops = NULL;
}

bool ring_buffer_register_ops(ring_buffer_type_t type, const struct ring_buffer_ops *ops)
{
    if (!ops || type < RING_BUFFER_TYPE_CUSTOM_BASE || 
        custom_ops_count >= RING_BUFFER_MAX_CUSTOM_OPS) 
	{
        return false;
    }
    
    if (find_custom_ops(type)) 
	{
        return false;  // 已注册
    }
    
    custom_ops_registry[custom_ops_count].type = type;
    custom_ops_registry[custom_ops_count].ops = ops;
    custom_ops_count++;
    
    RB_LOG("Registered custom ops (type=%d)", type);
    return true;
}

/* ==================== 便捷封装 API 实现 ==================== */

bool ring_buffer_write(ring_buffer_t *rb, uint8_t data)
{
    if (!rb || !rb->ops || !rb->ops->write) 
	{
        return false;
    }
    return rb->ops->write(rb, data);
}

bool ring_buffer_read(ring_buffer_t *rb, uint8_t *data)
{
    if (!rb || !data || !rb->ops || !rb->ops->read) 
	{
        return false;
    }
    return rb->ops->read(rb, data);
}

uint16_t ring_buffer_write_multi(ring_buffer_t *rb, const uint8_t *data, uint16_t len)
{
    if (!rb || !data || len == 0 || !rb->ops || !rb->ops->write_multi) 
	{
        return 0;
    }
    return rb->ops->write_multi(rb, data, len);
}

uint16_t ring_buffer_read_multi(ring_buffer_t *rb, uint8_t *data, uint16_t len)
{
    if (!rb || !data || len == 0 || !rb->ops || !rb->ops->read_multi) 
	{
        return 0;
    }
    return rb->ops->read_multi(rb, data, len);
}

uint16_t ring_buffer_available(const ring_buffer_t *rb)
{
    if (!rb || !rb->ops || !rb->ops->available) 
	{
        return 0;
    }
    return rb->ops->available(rb);
}

uint16_t ring_buffer_free_space(const ring_buffer_t *rb)
{
    if (!rb || !rb->ops || !rb->ops->free_space) 
	{
        return 0;
    }
    return rb->ops->free_space(rb);
}

bool ring_buffer_is_empty(const ring_buffer_t *rb)
{
    if (!rb || !rb->ops || !rb->ops->is_empty) 
	{
        return true;
    }
    return rb->ops->is_empty(rb);
}

bool ring_buffer_is_full(const ring_buffer_t *rb)
{
    if (!rb || !rb->ops || !rb->ops->is_full) 
	{
        return false;
    }
    return rb->ops->is_full(rb);
}

void ring_buffer_clear(ring_buffer_t *rb)
{
    if (!rb || !rb->ops || !rb->ops->clear) 
	{
        return;
    }
    rb->ops->clear(rb);
}
