/**
 * @file    ring_buffer_errno.c
 * @brief   环形缓冲区错误码实现
 * @author  CRITTY.熙影
 * @date    2024-12-27
 * @version 3.0
 */

#include "ring_buffer_errno.h"

#if RING_BUFFER_ENABLE_ERRNO

/* Private variables ---------------------------------------------------------*/

/**
 * @brief 全局错误码（非线程安全）
 */
ring_buffer_errno_t g_ring_buffer_errno = RB_OK;

/* Private constants ---------------------------------------------------------*/

/**
 * @brief 错误描述字符串表
 */
static const char* const error_strings[] = {
    [RB_OK]                         = "Success",
    [RB_ERR_NULL_POINTER]           = "Null pointer",
    [RB_ERR_INVALID_SIZE]           = "Invalid buffer size",
    [RB_ERR_INVALID_TYPE]           = "Unsupported strategy type",
    [RB_ERR_INVALID_OPS]            = "Invalid operations interface",
    [RB_ERR_BUFFER_FULL]            = "Buffer is full",
    [RB_ERR_BUFFER_EMPTY]           = "Buffer is empty",
    [RB_ERR_MUTEX_CREATE_FAILED]    = "Mutex creation failed",
    [RB_ERR_MUTEX_LOCK_FAILED]      = "Mutex lock failed",
    [RB_ERR_REGISTRY_FULL]          = "Custom strategy registry full",
    [RB_ERR_ALREADY_REGISTERED]     = "Strategy already registered",
    [RB_ERR_CUSTOM_TYPE_INVALID]    = "Invalid custom type value",
};

#define ERROR_STRINGS_COUNT (sizeof(error_strings) / sizeof(error_strings[0]))

/* Exported functions --------------------------------------------------------*/

ring_buffer_errno_t ring_buffer_get_errno(void)
{
    return g_ring_buffer_errno;
}

const char* ring_buffer_strerror(ring_buffer_errno_t err)
{
    if ( err < ERROR_STRINGS_COUNT && error_strings[err]) {
        return error_strings[err];
    }
    return "Unknown error";
}

void ring_buffer_clear_errno(void)
{
    g_ring_buffer_errno = RB_OK;
}

#endif /* RING_BUFFER_ENABLE_ERRNO */
