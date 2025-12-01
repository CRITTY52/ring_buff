/**
 * @file    ring_buffer_config.h
 * @brief   环形缓冲区配置文件
 * @author  CRITTY.熙影
 * @date    2024-12-27
 * @version 2.1
 */

#ifndef __RING_BUFFER_CONFIG_H
#define __RING_BUFFER_CONFIG_H

#ifdef __cplusplus
extern "C" {
#endif

/* =============================== 功能启用开关 =============================== */

/**
 * @brief 启用的线程安全策略
 */
#define RING_BUFFER_ENABLE_LOCKFREE    1  /**< 无锁模式 */
#define RING_BUFFER_ENABLE_DISABLE_IRQ 0  /**< 关中断模式 */
#define RING_BUFFER_ENABLE_MUTEX       0  /**< 互斥锁模式 */

/**
 * @brief 启用统计功能
 * RAM 开销：每个缓冲区 +12 字节
 */
#define RING_BUFFER_ENABLE_STATISTICS  0


/* ============================== 性能调优参数 =============================== */

/**
 * @brief 最小缓冲区大小（字节）
 */
#define RING_BUFFER_MIN_SIZE  2

/**
 * @brief 最大自定义策略数量
 */
#define RING_BUFFER_MAX_CUSTOM_OPS  4

/* =============================== 编译时检查 =============================== */

#if !RING_BUFFER_ENABLE_LOCKFREE && \
    !RING_BUFFER_ENABLE_DISABLE_IRQ && \
    !RING_BUFFER_ENABLE_MUTEX
    #error "至少启用一种线程安全策略"
#endif

#if RING_BUFFER_MIN_SIZE < 2
    #error "RING_BUFFER_MIN_SIZE 必须 >= 2"
#endif

/* =========================== 平台适配：中断控制 =========================== */

#if RING_BUFFER_ENABLE_DISABLE_IRQ

/* Cortex-M3/M4/M7/M33 */
#define PLATFORM_CORTEX_M

#ifdef PLATFORM_CORTEX_M
    #if defined(STM32F4) || defined(STM32F7)
        #include "core_cm4.h"
    #elif defined(STM32H7)
        #include "core_cm7.h"
    #elif defined(STM32L4)
        #include "core_cm4.h"
    #else
        #include "core_cm3.h"
    #endif
    
    typedef uint32_t irq_state_t;
    
    #define IRQ_SAVE(state)    do { \
        state = __get_PRIMASK(); \
        __disable_irq(); \
    } while(0)
    
    #define IRQ_RESTORE(state) do { \
        __set_PRIMASK(state); \
    } while(0)

#else
    #error "未选择目标平台，请定义 PLATFORM_xxx 宏"
#endif

#endif /* RING_BUFFER_ENABLE_DISABLE_IRQ */


/* ========================== 平台适配：RTOS 互斥锁 ========================= */

#if RING_BUFFER_ENABLE_MUTEX

/* FreeRTOS */
#define RTOS_FREERTOS

#ifdef RTOS_FREERTOS
    #include "FreeRTOS.h"
    #include "semphr.h"
    
    typedef SemaphoreHandle_t mutex_t;
    
    #define MUTEX_CREATE()          xSemaphoreCreateMutex()
    #define MUTEX_LOCK(m)           xSemaphoreTake((m), portMAX_DELAY)
    #define MUTEX_UNLOCK(m)         xSemaphoreGive(m)
    #define MUTEX_DELETE(m)         vSemaphoreDelete(m)
    #define MUTEX_IS_VALID(m)       ((m) != NULL)

#else
    #error "未选择 RTOS，请定义 RTOS_xxx 宏"
#endif

#endif /* RING_BUFFER_ENABLE_MUTEX */

/* ================================ 调试选项 ================================ */

//#define RING_BUFFER_DEBUG
#ifdef RING_BUFFER_DEBUG
    #include <stdio.h>
    #define RB_LOG(fmt, ...) printf("[RingBuf] " fmt "\n", ##__VA_ARGS__)
#else
    #define RB_LOG(fmt, ...) ((void)0)
#endif

#ifdef __cplusplus
}
#endif

#endif /* __RING_BUFFER_CONFIG_H */
