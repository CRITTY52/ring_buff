/**
 * @file    ring_buffer_test.c
 * @brief   环形缓冲区单元测试
 * @author  CRITTY.熙影
 * @date    2024-12-27
 * @version 3.0
 */

#include <stdio.h>
#include <string.h>
#include <assert.h>
#include "ring_buffer.h"
#include "ring_buffer_errno.h"

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

bool test_error_handling(void)
{
#if RING_BUFFER_ENABLE_ERRNO
    static uint8_t buffer[256];
    ring_buffer_t rb;
    
    /* 测试 NULL 指针 */
    TEST_ASSERT(!ring_buffer_create(NULL, buffer, 256, RING_BUFFER_TYPE_LOCKFREE));
    TEST_ASSERT(ring_buffer_get_errno() == RB_ERR_NULL_POINTER);
    
    /* 测试无效大小 */
    TEST_ASSERT(!ring_buffer_create(&rb, buffer, 1, RING_BUFFER_TYPE_LOCKFREE));
    TEST_ASSERT(ring_buffer_get_errno() == RB_ERR_INVALID_SIZE);
    
    /* 测试错误描述 */
    const char *msg = ring_buffer_strerror(RB_ERR_BUFFER_FULL);
    TEST_ASSERT(strcmp(msg, "Buffer is full") == 0);
    
    /* 清除错误 */
    ring_buffer_clear_errno();
    TEST_ASSERT(ring_buffer_get_errno() == RB_OK);
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

bool test_wrap_around(void)
{
    static uint8_t buffer[8];
    ring_buffer_t rb;
    
    ring_buffer_create(&rb, buffer, 8, RING_BUFFER_TYPE_LOCKFREE);
    
    /* 填满缓冲区（7个字节）*/
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
    
    /* 写入5个（测试环绕）*/
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

//int main(void)
//{
//    printf("\n========== Ring Buffer Unit Tests ==========\n\n");
//    
//    RUN_TEST(test_create_destroy);
//    RUN_TEST(test_error_handling);
//    RUN_TEST(test_single_byte_rw);
//    RUN_TEST(test_multi_byte_rw);
//    RUN_TEST(test_wrap_around);
//    RUN_TEST(test_full_condition);
//    RUN_TEST(test_clear);
//    
//    printf("\n========== All Tests Passed! ==========\n\n");
//    
//    return 0;
//}
