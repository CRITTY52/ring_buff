/**
 * @file    ring_buffer_config.h
 * @brief   环形缓冲区配置文件 - 统一管理所有编译选项
 * @author  CRITTY.熙影
 * @date    2024-12-27
 * @version 3.0
 * 
 * @details
 * 本文件集中管理所有配置项：
 * 1. 功能启用/禁用（减少代码体积）
 * 2. 性能参数调优
 * 3. 平台适配（RTOS、中断控制）
 * 4. 调试选项
 */

#ifndef __RING_BUFFER_CONFIG_H
#define __RING_BUFFER_CONFIG_H

#ifdef __cplusplus
extern "C" {
#endif

/* ==================== 功能启用开关 ==================== */

/**
 * @brief 启用的线程安全策略
 * 
 * 选择指南：
 * - LOCKFREE: ISR → 主循环（单生产者单消费者，SPSC）
 * - DISABLE_IRQ: 裸机多任务，多个中断源共享缓冲区
 * - MUTEX: FreeRTOS/RT-Thread 等 RTOS 多线程
 */
#define RING_BUFFER_ENABLE_LOCKFREE    1  /**< 无锁模式 */
#define RING_BUFFER_ENABLE_DISABLE_IRQ 0  /**< 关中断模式 */
#define RING_BUFFER_ENABLE_MUTEX       0  /**< 互斥锁模式 */

/**
 * @brief 启用参数检查
 * 
 * 建议：
 * - 开发/调试阶段：启用（1）
 * - 发布版本：禁用（0）以减少代码体积和执行时间
 */
#define RING_BUFFER_ENABLE_PARAM_CHECK  1

/**
 * @brief 启用统计功能
 * 
 * 启用后可统计：读写次数、溢出次数（用于性能分析）
 * RAM 开销：每个缓冲区 +12 字节
 */
#define RING_BUFFER_ENABLE_STATISTICS  0

/**
 * @brief 启用错误码机制
 * 
 * 启用后可通过 ring_buffer_get_errno() 获取详细错误信息
 * ROM 开销：约 +200 字节（错误描述字符串）
 */
#define RING_BUFFER_ENABLE_ERRNO  1

/* ==================== 性能调优参数 ==================== */

/**
 * @brief 最小缓冲区大小（字节）
 * 
 * 注意：实际可用容量 = size - 1
 */
#define RING_BUFFER_MIN_SIZE  2

/**
 * @brief 最大自定义策略数量
 * 
 * RAM 开销：每个策略占用 8 字节（32位系统）
 */
#define RING_BUFFER_MAX_CUSTOM_OPS  4

/* ==================== 平台适配：中断控制 ==================== */

#if RING_BUFFER_ENABLE_DISABLE_IRQ

/**
 * @brief 选择目标平台
 * 
 * 取消注释对应的平台宏，或自定义中断控制
 */

/* Cortex-M3/M4/M7/M33 (STM32, NXP i.MX RT, Nordic nRF等) */
#define PLATFORM_CORTEX_M

/* AVR (Arduino Uno, Mega 等) */
// #define PLATFORM_AVR

/* RISC-V (GD32V, ESP32-C3 等) */
// #define PLATFORM_RISCV

/* 自定义平台 */
// #define PLATFORM_CUSTOM

/* -------------------- Cortex-M 实现 -------------------- */
#ifdef PLATFORM_CORTEX_M
    /* 根据具体芯片选择对应的头文件 */
    #if defined(STM32F4) || defined(STM32F7)
        #include "core_cm4.h"
    #elif defined(STM32H7)
        #include "core_cm7.h"
    #elif defined(STM32L4)
        #include "core_cm4.h"
    #else
        #include "core_cm3.h"  // 默认使用 CM3
    #endif
    
    typedef uint32_t irq_state_t;
    
    #define IRQ_SAVE(state)    do { \
        state = __get_PRIMASK(); \
        __disable_irq(); \
    } while(0)
    
    #define IRQ_RESTORE(state) do { \
        __set_PRIMASK(state); \
    } while(0)

/* -------------------- AVR 实现 -------------------- */
#elif defined(PLATFORM_AVR)
    #include <avr/interrupt.h>
    
    typedef uint8_t irq_state_t;
    
    #define IRQ_SAVE(state)    do { \
        state = SREG; \
        cli(); \
    } while(0)
    
    #define IRQ_RESTORE(state) do { \
        SREG = state; \
    } while(0)

/* -------------------- RISC-V 实现 -------------------- */
#elif defined(PLATFORM_RISCV)
    #include "riscv_encoding.h"
    
    typedef unsigned long irq_state_t;
    
    #define IRQ_SAVE(state)    do { \
        state = read_csr(mstatus); \
        clear_csr(mstatus, MSTATUS_MIE); \
    } while(0)
    
    #define IRQ_RESTORE(state) do { \
        write_csr(mstatus, state); \
    } while(0)

/* -------------------- 自定义平台 -------------------- */
#elif defined(PLATFORM_CUSTOM)
    #error "请实现自定义平台的中断控制接口"
    
    typedef uint32_t irq_state_t;
    #define IRQ_SAVE(state)    /* TODO: 保存中断状态并关闭中断 */
    #define IRQ_RESTORE(state) /* TODO: 恢复中断状态 */

#else
    #error "未选择目标平台，请定义 PLATFORM_xxx 宏"
#endif

#endif /* RING_BUFFER_ENABLE_DISABLE_IRQ */

/* ==================== 平台适配：RTOS 互斥锁 ==================== */

#if RING_BUFFER_ENABLE_MUTEX

/**
 * @brief 选择 RTOS
 */

/* FreeRTOS */
#define RTOS_FREERTOS

/* RT-Thread */
// #define RTOS_RT_THREAD

/* 自定义 RTOS */
// #define RTOS_CUSTOM

/* -------------------- FreeRTOS 实现 -------------------- */
#ifdef RTOS_FREERTOS
    #include "FreeRTOS.h"
    #include "semphr.h"
    
    typedef SemaphoreHandle_t mutex_t;
    
    #define MUTEX_CREATE()          xSemaphoreCreateMutex()
    #define MUTEX_LOCK(m)           xSemaphoreTake((m), portMAX_DELAY)
    #define MUTEX_UNLOCK(m)         xSemaphoreGive(m)
    #define MUTEX_DELETE(m)         vSemaphoreDelete(m)
    #define MUTEX_IS_VALID(m)       ((m) != NULL)

/* -------------------- RT-Thread 实现 -------------------- */
#elif defined(RTOS_RT_THREAD)
    #include "rtthread.h"
    
    typedef rt_mutex_t mutex_t;
    
    #define MUTEX_CREATE()          rt_mutex_create("rb", RT_IPC_FLAG_PRIO)
    #define MUTEX_LOCK(m)           rt_mutex_take((m), RT_WAITING_FOREVER)
    #define MUTEX_UNLOCK(m)         rt_mutex_release(m)
    #define MUTEX_DELETE(m)         rt_mutex_delete(m)
    #define MUTEX_IS_VALID(m)       ((m) != RT_NULL)

/* -------------------- 自定义 RTOS -------------------- */
#elif defined(RTOS_CUSTOM)
    #error "请实现自定义 RTOS 的互斥锁接口"
    
    typedef void* mutex_t;
    #define MUTEX_CREATE()          /* TODO */
    #define MUTEX_LOCK(m)           /* TODO */
    #define MUTEX_UNLOCK(m)         /* TODO */
    #define MUTEX_DELETE(m)         /* TODO */
    #define MUTEX_IS_VALID(m)       /* TODO */

#else
    #error "未选择 RTOS，请定义 RTOS_xxx 宏"
#endif

#endif /* RING_BUFFER_ENABLE_MUTEX */

/* ==================== 调试选项 ==================== */

/**
 * @brief 启用调试日志
 * 
 * 定义 RING_BUFFER_DEBUG=1 启用
 */
#ifdef RING_BUFFER_DEBUG
    #include <stdio.h>
    #define RB_LOG(fmt, ...) printf("[RingBuf] " fmt "\n", ##__VA_ARGS__)
#else
    #define RB_LOG(fmt, ...) ((void)0)
#endif

/* ==================== 编译时检查 ==================== */

/* 至少启用一种策略 */
#if !RING_BUFFER_ENABLE_LOCKFREE && \
    !RING_BUFFER_ENABLE_DISABLE_IRQ && \
    !RING_BUFFER_ENABLE_MUTEX
    #error "至少启用一种线程安全策略"
#endif

/* 检查最小尺寸 */
#if RING_BUFFER_MIN_SIZE < 2
    #error "RING_BUFFER_MIN_SIZE 必须 >= 2"
#endif

/* 检查自定义策略数量 */
#if RING_BUFFER_MAX_CUSTOM_OPS < 1 || RING_BUFFER_MAX_CUSTOM_OPS > 16
    #error "RING_BUFFER_MAX_CUSTOM_OPS 范围：1-16"
#endif

#ifdef __cplusplus
}
#endif

#endif /* __RING_BUFFER_CONFIG_H */
