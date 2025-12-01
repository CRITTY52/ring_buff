/**
 * @file    ring_buffer.h
 * @brief   环形缓冲区公共接口
 * @author  CRITTY.熙影
 * @date    2024-12-27
 * @version 2.1
 */

#ifndef __RING_BUFFER_H
#define __RING_BUFFER_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include "ring_buffer_config.h"

/* Forward declarations ------------------------------------------------------*/
typedef struct ring_buffer_ops ring_buffer_ops_t;

/* Exported types ------------------------------------------------------------*/
/**
 * @brief 线程安全策略枚举
 */
typedef enum {
    RING_BUFFER_TYPE_LOCKFREE = 0,   /**< 无锁模式（SPSC）*/
    RING_BUFFER_TYPE_DISABLE_IRQ,    /**< 关中断模式（裸机）*/
    RING_BUFFER_TYPE_MUTEX,          /**< 互斥锁模式（RTOS）*/
    RING_BUFFER_TYPE_CUSTOM_BASE     /**< 自定义策略起始值 */
} ring_buffer_type_t;

/**
 * @brief 环形缓冲区控制结构
 */
typedef struct {
    uint8_t *buffer;                        /**< 数据缓冲区指针 */
    uint16_t size;                          /**< 缓冲区总大小（字节）*/
    volatile uint16_t head;                 /**< 写指针（生产者）*/
    volatile uint16_t tail;                 /**< 读指针（消费者）*/
    void *lock;                             /**< 锁句柄（互斥锁模式）*/
    const ring_buffer_ops_t *ops;           /**< 操作接口指针 */
    
#if RING_BUFFER_ENABLE_STATISTICS
    uint32_t write_count;                   /**< 写入次数 */
    uint32_t read_count;                    /**< 读取次数 */
    uint32_t overflow_count;                /**< 溢出次数 */
#endif
} ring_buffer_t;

/**
 * @brief 操作接口结构体（策略模式）
 */
typedef struct ring_buffer_ops {
    bool (*write)(ring_buffer_t *rb, uint8_t data);
    bool (*read)(ring_buffer_t *rb, uint8_t *data);
    uint16_t (*write_multi)(ring_buffer_t *rb, const uint8_t *data, uint16_t len);
    uint16_t (*read_multi)(ring_buffer_t *rb, uint8_t *data, uint16_t len);
    uint16_t (*available)(const ring_buffer_t *rb);
    uint16_t (*free_space)(const ring_buffer_t *rb);
    bool (*is_empty)(const ring_buffer_t *rb);
    bool (*is_full)(const ring_buffer_t *rb);
    void (*clear)(ring_buffer_t *rb);
} ring_buffer_ops_t;

/* Exported functions --------------------------------------------------------*/

/**
 * @brief 创建并初始化环形缓冲区（工厂函数）
 * @param rb     缓冲区控制结构指针（用户分配）
 * @param buffer 数据存储空间指针（用户分配）
 * @param size   缓冲区大小（字节，必须 >= 2）
 * @param type   线程安全策略
 * @return true=成功, false=失败（参数错误或不支持的策略）
 * @note 
 * - 完全静态分配，无堆依赖
 * - 实际可用容量 = size - 1
 * - 互斥锁模式会自动创建互斥锁（可能失败）
 * @code
 * static uint8_t uart_rx_buf[256];
 * static ring_buffer_t uart_rx_rb;
 * 
 * if (!ring_buffer_create(&uart_rx_rb, uart_rx_buf, 256, 
 *                         RING_BUFFER_TYPE_LOCKFREE)) {
 *     // 创建失败，检查参数或策略是否启用
 *     Error_Handler();
 * }
 * @endcode
 */
bool ring_buffer_create(
    ring_buffer_t *rb,
    uint8_t *buffer,
    uint16_t size,
    ring_buffer_type_t type
);

/**
 * @brief 销毁环形缓冲区，释放资源
 * @param rb 缓冲区指针
 * @note 互斥锁模式会删除互斥锁
 */
void ring_buffer_destroy(ring_buffer_t *rb);

/**
 * @brief 注册自定义策略
 * @param type 策略类型（>= RING_BUFFER_TYPE_CUSTOM_BASE）
 * @param ops  操作接口指针
 * @return true=成功, false=失败（参数错误或注册表已满）
 */
bool ring_buffer_register_ops(ring_buffer_type_t type, const ring_buffer_ops_t *ops);

/**
 * @brief 获取操作接口指针（性能关键场景）
 * @param rb 缓冲区指针
 * @return 操作接口指针，参数错误返回 NULL
 */
static inline const ring_buffer_ops_t* ring_buffer_get_ops(const ring_buffer_t *rb)
{
    return (rb ? rb->ops : NULL);
}

/**
 * @brief 写入单个字节
 * @param rb   缓冲区指针
 * @param data 待写入的字节
 * @return true=成功, false=缓冲区已满或参数错误
 */
bool ring_buffer_write(ring_buffer_t *rb, uint8_t data);

/**
 * @brief 读取单个字节
 * @param rb   缓冲区指针
 * @param data 读取数据存放地址
 * @return true=成功, false=缓冲区为空或参数错误
 */
bool ring_buffer_read(ring_buffer_t *rb, uint8_t *data);

/**
 * @brief 批量写入数据
 * @param rb   缓冲区指针
 * @param data 待写入的数据指针
 * @param len  待写入的字节数
 * @return 实际写入的字节数（0 表示参数错误或缓冲区已满）
 * @note 
 * - 返回值 < len 表示缓冲区空间不足，部分数据已写入
 * - 如需原子性写入，调用前先检查 ring_buffer_free_space()
 */
uint16_t ring_buffer_write_multi(ring_buffer_t *rb, const uint8_t *data, uint16_t len);

/**
 * @brief 批量读取数据
 * @param rb   缓冲区指针
 * @param data 读取数据存放地址
 * @param len  期望读取的字节数
 * @return 实际读取的字节数（0 表示参数错误或缓冲区为空）
 * @note 返回值 < len 表示缓冲区数据不足，已读取所有可用数据
 */
uint16_t ring_buffer_read_multi(ring_buffer_t *rb, uint8_t *data, uint16_t len);

/**
 * @brief 查询可读数据量
 * @param rb 缓冲区指针
 * @return 可读字节数（参数错误返回 0）
 */
uint16_t ring_buffer_available(const ring_buffer_t *rb);

/**
 * @brief 查询剩余空间
 * @param rb 缓冲区指针
 * @return 剩余可写字节数（参数错误返回 0）
 */
uint16_t ring_buffer_free_space(const ring_buffer_t *rb);

/**
 * @brief 判断缓冲区是否为空
 * @param rb 缓冲区指针
 * @return true=空, false=非空
 */
bool ring_buffer_is_empty(const ring_buffer_t *rb);

/**
 * @brief 判断缓冲区是否已满
 * @param rb 缓冲区指针
 * @return true=满, false=未满
 */
bool ring_buffer_is_full(const ring_buffer_t *rb);

/**
 * @brief 清空缓冲区
 * @param rb 缓冲区指针
 * @note 仅重置读写指针，不清除实际数据
 */
void ring_buffer_clear(ring_buffer_t *rb);

#ifdef __cplusplus
}
#endif

#endif /* __RING_BUFFER_H */
