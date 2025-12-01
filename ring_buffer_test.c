/**
 * @file    ring_buffer_test.c
 * @brief   环形缓冲区单元测试
 * @author  CRITTY.熙影
 * @date    2024-12-27
 * @version 3.1 (移除错误码机制)
 */

#include <stdio.h>
#include <string.h>
#include <assert.h>
#include "ring_buffer.h"

/* Test utilities ------------------------------------------------------------*/

#define TEST_ASSERT(cond) do { \
    if (!(cond)) { \
        printf("? FAILED: %s (line %d)\n", #cond, __LINE__); \
        return false; \
    } \
} while(0)

#define RUN_TEST(test_func) do { \
    printf("Testing: %s ... ", #test_func); \
    if (test_func()) { \
        printf("? PASSED\n"); \
    } else { \
        printf("? FAILED\n"); \
        return 1; \
    } \
} while(0)

/* Test cases ----------------------------------------------------------------*/

bool test_create_destroy(void)
{
    static uint8_t buffer[256];
    ring_buffer_t rb;
    
    /* 正常创建 */
    TEST_ASSERT(ring_buffer_create(&rb, buffer, 256, RING_BUFFER_TYPE_LOCKFREE));
    TEST_ASSERT(rb.buffer == buffer);
    TEST_ASSERT(rb.size == 256);
    TEST_ASSERT(rb.head == 0);
    TEST_ASSERT(rb.tail == 0);
    TEST_ASSERT(rb.ops != NULL);
    
    /* 销毁 */
    ring_buffer_destroy(&rb);
    TEST_ASSERT(rb.buffer == NULL);
    TEST_ASSERT(rb.ops == NULL);
    
    return true;
}

bool test_param_check(void)
{
#if RING_BUFFER_ENABLE_PARAM_CHECK
    static uint8_t buffer[256];
    ring_buffer_t rb;
    
    /* 测试 NULL 指针 */
    TEST_ASSERT(!ring_buffer_create(NULL, buffer, 256, RING_BUFFER_TYPE_LOCKFREE));
    TEST_ASSERT(!ring_buffer_create(&rb, NULL, 256, RING_BUFFER_TYPE_LOCKFREE));
    
    /* 测试无效大小 */
    TEST_ASSERT(!ring_buffer_create(&rb, buffer, 1, RING_BUFFER_TYPE_LOCKFREE));
    
    /* 测试不支持的策略类型 */
    TEST_ASSERT(!ring_buffer_create(&rb, buffer, 256, (ring_buffer_type_t)99));
#endif
    
    return true;
}

bool test_single_byte_rw(void)
{
    static uint8_t buffer[16];
    ring_buffer_t rb;
    
    ring_buffer_create(&rb, buffer, 16, RING_BUFFER_TYPE_LOCKFREE);
    
    /* 写入数据 */
    TEST_ASSERT(ring_buffer_write(&rb, 0xAA));
    TEST_ASSERT(ring_buffer_write(&rb, 0xBB));
    TEST_ASSERT(ring_buffer_write(&rb, 0xCC));
    
    TEST_ASSERT(ring_buffer_available(&rb) == 3);
    TEST_ASSERT(!ring_buffer_is_empty(&rb));
    
    /* 读取数据 */
    uint8_t data;
    TEST_ASSERT(ring_buffer_read(&rb, &data));
    TEST_ASSERT(data == 0xAA);
    
    TEST_ASSERT(ring_buffer_read(&rb, &data));
    TEST_ASSERT(data == 0xBB);
    
    TEST_ASSERT(ring_buffer_available(&rb) == 1);
    
    ring_buffer_destroy(&rb);
    return true;
}

bool test_multi_byte_rw(void)
{
    static uint8_t buffer[32];
    ring_buffer_t rb;
    
    ring_buffer_create(&rb, buffer, 32, RING_BUFFER_TYPE_LOCKFREE);
    
    /* 批量写入 */
    uint8_t write_data[] = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10};
    uint16_t written = ring_buffer_write_multi(&rb, write_data, 10);
    TEST_ASSERT(written == 10);
    
    /* 批量读取 */
    uint8_t read_data[20];
    uint16_t read = ring_buffer_read_multi(&rb, read_data, 20);
    TEST_ASSERT(read == 10);
    TEST_ASSERT(memcmp(read_data, write_data, 10) == 0);
    
    TEST_ASSERT(ring_buffer_is_empty(&rb));
    
    ring_buffer_destroy(&rb);
    return true;
}

bool test_partial_write_read(void)
{
    static uint8_t buffer[8];
    ring_buffer_t rb;
    
    ring_buffer_create(&rb, buffer, 8, RING_BUFFER_TYPE_LOCKFREE);
    
    /* 写满缓冲区（7字节，size-1） */
    uint8_t data1[10] = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10};
    uint16_t written = ring_buffer_write_multi(&rb, data1, 10);
    TEST_ASSERT(written == 7);  // 只能写入7个
    TEST_ASSERT(ring_buffer_is_full(&rb));
    
    /* 读取3个，腾出空间 */
    uint8_t read_buf[10];
    uint16_t read = ring_buffer_read_multi(&rb, read_buf, 3);
    TEST_ASSERT(read == 3);
    TEST_ASSERT(read_buf[0] == 1 && read_buf[1] == 2 && read_buf[2] == 3);
    
    /* 再写入5个，但只能写入3个 */
    uint8_t data2[5] = {11, 12, 13, 14, 15};
    written = ring_buffer_write_multi(&rb, data2, 5);
    TEST_ASSERT(written == 3);
    
    ring_buffer_destroy(&rb);
    return true;
}

bool test_wrap_around(void)
{
    static uint8_t buffer[8];
    ring_buffer_t rb;
    
    ring_buffer_create(&rb, buffer, 8, RING_BUFFER_TYPE_LOCKFREE);
    
    /* 填充接近满（7个字节）*/
    for (int i = 0; i < 7; i++) {
        TEST_ASSERT(ring_buffer_write(&rb, i));
    }
    TEST_ASSERT(ring_buffer_is_full(&rb));
    
    /* 读取3个 */
    uint8_t data;
    for (int i = 0; i < 3; i++) {
        TEST_ASSERT(ring_buffer_read(&rb, &data));
        TEST_ASSERT(data == i);
    }
    
    /* 写入5个，测试回绕 */
    for (int i = 100; i < 105; i++) {
        TEST_ASSERT(ring_buffer_write(&rb, i));
    }
    
    /* 验证数据 */
    TEST_ASSERT(ring_buffer_available(&rb) == 9);  // 4 + 5
    
    ring_buffer_destroy(&rb);
    return true;
}

bool test_full_condition(void)
{
    static uint8_t buffer[8];
    ring_buffer_t rb;
    
    ring_buffer_create(&rb, buffer, 8, RING_BUFFER_TYPE_LOCKFREE);
    
    /* 填满 */
    for (int i = 0; i < 7; i++) {
        TEST_ASSERT(ring_buffer_write(&rb, i));
    }
    
    /* 再写入应该失败 */
    TEST_ASSERT(!ring_buffer_write(&rb, 0xFF));
    TEST_ASSERT(ring_buffer_free_space(&rb) == 0);
    
#if RING_BUFFER_ENABLE_STATISTICS
    TEST_ASSERT(rb.overflow_count == 1);
#endif
    
    ring_buffer_destroy(&rb);
    return true;
}

bool test_empty_condition(void)
{
    static uint8_t buffer[16];
    ring_buffer_t rb;
    
    ring_buffer_create(&rb, buffer, 16, RING_BUFFER_TYPE_LOCKFREE);
    
    /* 空缓冲区读取应该失败 */
    uint8_t data;
    TEST_ASSERT(!ring_buffer_read(&rb, &data));
    TEST_ASSERT(ring_buffer_is_empty(&rb));
    
    /* 批量读取空缓冲区 */
    uint8_t buf[10];
    TEST_ASSERT(ring_buffer_read_multi(&rb, buf, 10) == 0);
    
    ring_buffer_destroy(&rb);
    return true;
}

bool test_clear(void)
{
    static uint8_t buffer[16];
    ring_buffer_t rb;
    
    ring_buffer_create(&rb, buffer, 16, RING_BUFFER_TYPE_LOCKFREE);
    
    /* 写入数据 */
    for (int i = 0; i < 10; i++) {
        ring_buffer_write(&rb, i);
    }
    
    /* 清空 */
    ring_buffer_clear(&rb);
    
    TEST_ASSERT(ring_buffer_is_empty(&rb));
    TEST_ASSERT(ring_buffer_available(&rb) == 0);
    TEST_ASSERT(ring_buffer_free_space(&rb) == 15);
    
    ring_buffer_destroy(&rb);
    return true;
}

/* Main ----------------------------------------------------------------------*/

int main(void)
{
    printf("\n========== Ring Buffer Unit Tests ==========\n\n");
    
    RUN_TEST(test_create_destroy);
    RUN_TEST(test_param_check);
    RUN_TEST(test_single_byte_rw);
    RUN_TEST(test_multi_byte_rw);
    RUN_TEST(test_partial_write_read);
    RUN_TEST(test_wrap_around);
    RUN_TEST(test_full_condition);
    RUN_TEST(test_empty_condition);
    RUN_TEST(test_clear);
    
    printf("\n========== All Tests Passed! ==========\n\n");
    
    return 0;
}
