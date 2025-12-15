/**
 * @file    ring_buffer_test.h
 * @brief   环形缓冲区单元测试接口
 * @author  CRITTY.熙影
 * @date    2024-12-27
 */

#ifndef __RING_BUFFER_TEST_H
#define __RING_BUFFER_TEST_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stdbool.h>

/* ==================== 测试接口 ==================== */

/**
 * @brief 运行所有单元测试
 * @return 0=全部通过, 1=部分失败
 * 
 * @note 会自动执行所有测试用例并输出结果
 */
int ring_buffer_run_all_tests(void);

/**
 * @brief 单独运行某个测试（调试用）
 */
bool test_version_info(void);
bool test_create_destroy(void);
bool test_single_byte_write_read(void);
bool test_multi_byte_write_read(void);
bool test_buffer_full(void);
bool test_wrap_around(void);
bool test_partial_write(void);
bool test_clear(void);
bool test_state_queries(void);
bool test_minimum_size(void);
bool test_concurrent_scenario(void);
bool test_stress(void);
bool test_custom_strategy(void);
bool test_memory_usage(void);

#if RING_BUFFER_ENABLE_STATISTICS
bool test_statistics(void);
#endif

/* ==================== 使用示例 ==================== */
#if 0
/* 在 main.c 中调用 */
#include "ring_buffer_test.h"

int main(void)
{
    /* 方式1：运行所有测试 */
    int result = ring_buffer_run_all_tests();
    if (result == 0) {
        printf("All tests passed!\n");
    }
    
    /* 方式2：单独测试某个功能 */
    if (test_buffer_full()) {
        printf("Buffer full test passed\n");
    }
    
    return 0;
}
#endif

#ifdef __cplusplus
}
#endif

#endif /* __RING_BUFFER_TEST_H */
