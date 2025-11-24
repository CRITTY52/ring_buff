/**
 * @file    ring_buffer_errno.h
 * @brief   环形缓冲区错误码定义
 * @author  CRITTY.熙影
 * @date    2024-12-27
 * @version 3.0
 */

#ifndef __RING_BUFFER_ERRNO_H
#define __RING_BUFFER_ERRNO_H

#ifdef __cplusplus
extern "C" {
#endif

#include "ring_buffer_config.h"

/* Exported types ------------------------------------------------------------*/

/**
 * @brief 错误码枚举
 */
typedef enum {
    /* 成功 */
    RB_OK = 0,                          /**< 操作成功 */
    
    /* 参数错误 (1-19) */
    RB_ERR_NULL_POINTER = 1,            /**< 空指针 */
    RB_ERR_INVALID_SIZE = 2,            /**< 缓冲区大小无效 */
    RB_ERR_INVALID_TYPE = 3,            /**< 策略类型不支持 */
    RB_ERR_INVALID_OPS = 4,             /**< 操作接口无效 */
    
    /* 状态错误 (20-39) */
    RB_ERR_BUFFER_FULL = 20,            /**< 缓冲区已满 */
    RB_ERR_BUFFER_EMPTY = 21,           /**< 缓冲区为空 */
    
    /* 资源错误 (40-59) */
    RB_ERR_MUTEX_CREATE_FAILED = 40,    /**< 互斥锁创建失败 */
    RB_ERR_MUTEX_LOCK_FAILED = 41,      /**< 获取锁失败 */
    
    /* 注册错误 (60-79) */
    RB_ERR_REGISTRY_FULL = 60,          /**< 自定义策略注册表已满 */
    RB_ERR_ALREADY_REGISTERED = 61,     /**< 策略已注册 */
    RB_ERR_CUSTOM_TYPE_INVALID = 62,    /**< 自定义类型值无效 */
    
} ring_buffer_errno_t;

/* Exported functions --------------------------------------------------------*/

#if RING_BUFFER_ENABLE_ERRNO

/**
 * @brief 获取最后一次错误码
 * @return 错误码
 * @note 
 * - 非线程安全（全局变量）
 * - 仅在启用错误码功能时有效
 */
ring_buffer_errno_t ring_buffer_get_errno(void);

/**
 * @brief 获取错误描述字符串
 * @param err 错误码
 * @return 错误描述文本（常量字符串）
 */
const char* ring_buffer_strerror(ring_buffer_errno_t err);

/**
 * @brief 清除错误码
 */
void ring_buffer_clear_errno(void);

#endif /* RING_BUFFER_ENABLE_ERRNO */

/* Internal macros (仅供内部实现使用) ----------------------------------------*/

#if RING_BUFFER_ENABLE_ERRNO
    extern ring_buffer_errno_t g_ring_buffer_errno;
    
    #define RB_SET_ERRNO(err) do { g_ring_buffer_errno = (err); } while(0)
    #define RB_GET_ERRNO()    (g_ring_buffer_errno)
    
    /* 检查条件，失败时设置错误码并返回 */
    #define RB_CHECK_RETURN(cond, err, ret) do { \
        if (!(cond)) { \
            RB_SET_ERRNO(err); \
            RB_LOG("Error: %s", ring_buffer_strerror(err)); \
            return (ret); \
        } \
    } while(0)
    
#else
    /* 禁用错误码时的空实现 */
    #define RB_SET_ERRNO(err)                  ((void)0)
    #define RB_GET_ERRNO()                     (RB_OK)
    #define RB_CHECK_RETURN(cond, err, ret)    do { if (!(cond)) return (ret); } while(0)
    
#endif /* RING_BUFFER_ENABLE_ERRNO */
	
#if RING_BUFFER_ENABLE_ERRNO
    /* 检查条件，失败时设置错误码并返回（用于返回void的函数） */
    #define RB_CHECK_VOID(cond, err) do { \
        if (!(cond)) { \
            RB_SET_ERRNO(err); \
            RB_LOG("Error: %s", ring_buffer_strerror(err)); \
            return; \
        } \
    } while(0)
    
#else
    /* 禁用错误码时的空实现 */
    #define RB_CHECK_VOID(cond, err)    do { if (!(cond)) return; } while(0)
    
#endif /* RING_BUFFER_ENABLE_ERRNO */
	

#ifdef __cplusplus
}
#endif

#endif /* __RING_BUFFER_ERRNO_H */
