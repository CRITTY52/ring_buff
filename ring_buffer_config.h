/**
 * @file    ring_buffer_config.h
 * @brief   环形缓冲区配置文件
 * @author  CRITTY.熙影
 * @date    2024-12-27
 * @version V3.0.0 
 * @attention
 * <h2><center>&copy; Copyright (c) 2024 CRITTY.</center></h2>
 * All rights reserved.
 * This software is licensed under terms that can be found in the LICENSE file
 * in the root directory of this software component.
 */
 
/* ===============================================================================
   版本历史 (Version History)
   ===============================================================================
   
   V3.0.0 / 2024-12-27
   -------------------
   [重构] * 采用工厂策略模式(无锁、关中断、互斥锁)
   [新增] + 创建、销毁、自定义注册等函数
   [新增] + 各类宏开关及配置选项
   [优化] * 保留ops接口添加便携API
   
   V2.0.0 / 2024-12-21
   -------------------
   [破坏性变更] ! API 不兼容 v1.x
   [重构] * 不直接调用函数改用ops接口
   [文档] # 增加readme文档
   [新增] + 增加测试文件
   [优化] * 版本号优化 
   
   V1.0.1 / 2024-12-20
   -------------------
   [优化] * 移除部分volatile变量
   [优化] * 增加const修饰
   [优化] * 增加边界检查，防御性编程
   
   V1.0.0 / 2024-12-01
   -------------------
   [初始版本] * 基础环形缓冲区实现
   
   ==============================================================================
   变更标记说明:
   [新增] + Feature         新功能
   [优化] * Improved        性能/代码优化
   [修复] - Bugfix          缺陷修复
   [重构] * Refactor        代码重构
   [文档] # Docs            文档更新
   [破坏性变更] ! Breaking  不兼容的API变更
   ============================================================================= */
   
#ifndef __RING_BUFFER_CONFIG_H
#define __RING_BUFFER_CONFIG_H

#ifdef __cplusplus
extern "C" {
#endif

/* ================================= 版本信息 ================================== */

#define RING_BUFFER_VERSION_MAJOR    3        /**< 主版本号：不兼容的API修改 */
#define RING_BUFFER_VERSION_MINOR    0        /**< 次版本号：向下兼容的功能新增 */
#define RING_BUFFER_VERSION_PATCH    0        /**< 修订号：向下兼容的问题修正 */
#define RING_BUFFER_VERSION_STRING   "3.0.0"  /**< 版本字符串 */
	  
	  
/* =============================== 功能启用开关 ================================ */

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


/* =============================== 性能调优参数 =============================== */

/**
 * @brief 最小缓冲区大小（字节）
 */
#define RING_BUFFER_MIN_SIZE  2
/**
 * @brief 最大自定义策略数量
 */
#define RING_BUFFER_MAX_CUSTOM_OPS  4


/* ================================ 编译时检查 ================================ */

#if !RING_BUFFER_ENABLE_LOCKFREE && \
    !RING_BUFFER_ENABLE_DISABLE_IRQ && \
    !RING_BUFFER_ENABLE_MUTEX
    #error "至少启用一种线程安全策略"
#endif

#if RING_BUFFER_MIN_SIZE < 2
    #error "RING_BUFFER_MIN_SIZE 必须 >= 2"
#endif


/* ============================ 平台适配：中断控制 ============================ */

#if RING_BUFFER_ENABLE_DISABLE_IRQ

/* 选择目标平台 */
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

/* 选择 RTOS 类型 */
// #define RTOS_FREERTOS
// #define RTOS_RTTHREAD

#ifdef RTOS_FREERTOS
    #include "FreeRTOS.h"
    #include "semphr.h"
    
    typedef SemaphoreHandle_t mutex_t;
    
    #define MUTEX_CREATE()          xSemaphoreCreateMutex()
    #define MUTEX_LOCK(m)           xSemaphoreTake((m), portMAX_DELAY)
    #define MUTEX_UNLOCK(m)         xSemaphoreGive(m)
    #define MUTEX_DELETE(m)         vSemaphoreDelete(m)
    #define MUTEX_IS_VALID(m)       ((m) != NULL)

#elif defined(RTOS_RTTHREAD)
    #include "rtthread.h"
    
    typedef rt_mutex_t mutex_t;
    
    #define MUTEX_CREATE()          rt_mutex_create("rb_mtx", RT_IPC_FLAG_PRIO)
    #define MUTEX_LOCK(m)           rt_mutex_take((m), RT_WAITING_FOREVER)
    #define MUTEX_UNLOCK(m)         rt_mutex_release(m)
    #define MUTEX_DELETE(m)         rt_mutex_delete(m)
    #define MUTEX_IS_VALID(m)       ((m) != RT_NULL)

#else
    #error "未选择 RTOS，请定义 RTOS_FREERTOS 或 RTOS_RTTHREAD 宏"
#endif

#endif /* RING_BUFFER_ENABLE_MUTEX */

/* ================================= 调试选项 ================================= */

//#define RING_BUFFER_DEBUG

#ifdef RING_BUFFER_DEBUG
    #include <stdio.h>
    
    /* 错误级别：详细信息（文件名、行号、函数名） */
    #define RB_LOG_ERROR(fmt, ...) \
        printf("[RingBuf ERROR][%s:%d %s()] " fmt "\r\n", \
               __FILE__, __LINE__, __FUNCTION__, ##__VA_ARGS__)
    
    /* 警告级别：包含函数名 */
    #define RB_LOG_WARN(fmt, ...) \
        printf("[RingBuf WARN][%s()] " fmt "\r\n", \
               __FUNCTION__, ##__VA_ARGS__)
    
    /* 信息级别：简洁输出 */
    #define RB_LOG_INFO(fmt, ...) \
        printf("[RingBuf INFO] " fmt "\r\n", ##__VA_ARGS__)
#else
    #define RB_LOG_ERROR(fmt, ...) ((void)0)
    #define RB_LOG_WARN(fmt, ...)  ((void)0)
    #define RB_LOG_INFO(fmt, ...)  ((void)0)
#endif

#ifdef __cplusplus
}
#endif

#endif /* __RING_BUFFER_CONFIG_H */
