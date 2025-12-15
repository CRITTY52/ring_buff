/**
 * @file    ring_buffer_test.c
 * @brief   环形缓冲区单元测试实现
 * @author  CRITTY.熙影
 * @date    2024-12-27
 */

#include <stdio.h>
#include <string.h>
#include <assert.h>
#include "ring_buffer.h"
#include "ring_buffer_test.h"

/* ==================== 测试辅助宏 ==================== */
#define TEST_ASSERT(cond, msg) \
    do { \
        if (!(cond)) { \
            printf("? FAILED: %s (line %d)\n", msg, __LINE__); \
            return false; \
        } \
    } while(0)

#define TEST_PASS() do { printf("? PASSED\n"); return true; } while(0)

#define RUN_TEST(test_func) \
    do { \
        printf("Testing: %-40s", #test_func); \
        if (test_func()) { passed++; } \
        else { failed++; } \
        total++; \
    } while(0)

/* ==================== 测试用例实现 ==================== */

/**
 * @brief 测试1：版本信息获取
 */
bool test_version_info(void)
{
    const char *version = ring_buffer_get_version();
    TEST_ASSERT(version != NULL, "Version string is NULL");
    TEST_ASSERT(strcmp(version, RING_BUFFER_VERSION_STRING) == 0, 
                "Version mismatch");
    TEST_PASS();
}

/**
 * @brief 测试2：创建与销毁
 */
bool test_create_destroy(void)
{
    uint8_t buffer[64];
    ring_buffer_t rb;
    
    /* 正常创建 */
    TEST_ASSERT(ring_buffer_create(&rb, buffer, 64, RING_BUFFER_TYPE_LOCKFREE),
                "Create failed");
    TEST_ASSERT(rb.buffer == buffer, "Buffer pointer mismatch");
    TEST_ASSERT(rb.size == 64, "Size mismatch");
    TEST_ASSERT(rb.ops != NULL, "Ops pointer is NULL");
    
    /* 销毁 */
    ring_buffer_destroy(&rb);
    TEST_ASSERT(rb.buffer == NULL, "Buffer not cleared after destroy");
    TEST_ASSERT(rb.ops == NULL, "Ops not cleared after destroy");
    
    /* 参数检查 */
    TEST_ASSERT(!ring_buffer_create(NULL, buffer, 64, RING_BUFFER_TYPE_LOCKFREE),
                "NULL rb should fail");
    TEST_ASSERT(!ring_buffer_create(&rb, NULL, 64, RING_BUFFER_TYPE_LOCKFREE),
                "NULL buffer should fail");
    TEST_ASSERT(!ring_buffer_create(&rb, buffer, 1, RING_BUFFER_TYPE_LOCKFREE),
                "Size < 2 should fail");
    
    TEST_PASS();
}

/**
 * @brief 测试3：单字节读写
 */
bool test_single_byte_write_read(void)
{
    uint8_t buffer[16];
    ring_buffer_t rb;
    
    ring_buffer_create(&rb, buffer, 16, RING_BUFFER_TYPE_LOCKFREE);
    
    /* 写入单字节 */
    TEST_ASSERT(ring_buffer_write(&rb, 0xAA), "Write failed");
    TEST_ASSERT(ring_buffer_available(&rb) == 1, "Available should be 1");
    
    /* 读取单字节 */
    uint8_t data;
    TEST_ASSERT(ring_buffer_read(&rb, &data), "Read failed");
    TEST_ASSERT(data == 0xAA, "Data mismatch");
    TEST_ASSERT(ring_buffer_is_empty(&rb), "Should be empty");
    
    ring_buffer_destroy(&rb);
    TEST_PASS();
}

/**
 * @brief 测试4：批量读写
 */
bool test_multi_byte_write_read(void)
{
    uint8_t buffer[32];
    ring_buffer_t rb;
    
    ring_buffer_create(&rb, buffer, 32, RING_BUFFER_TYPE_LOCKFREE);
    
    /* 批量写入 */
    uint8_t write_data[] = {0x01, 0x02, 0x03, 0x04, 0x05};
    uint16_t written = ring_buffer_write_multi(&rb, write_data, 5);
    TEST_ASSERT(written == 5, "Write_multi failed");
    TEST_ASSERT(ring_buffer_available(&rb) == 5, "Available should be 5");
    
    /* 批量读取 */
    uint8_t read_data[5];
    uint16_t read = ring_buffer_read_multi(&rb, read_data, 5);
    TEST_ASSERT(read == 5, "Read_multi failed");
    TEST_ASSERT(memcmp(write_data, read_data, 5) == 0, "Data mismatch");
    
    ring_buffer_destroy(&rb);
    TEST_PASS();
}

/**
 * @brief 测试5：缓冲区满状态
 */
bool test_buffer_full(void)
{
    uint8_t buffer[8];  /* 实际可用容量 = 7 */
    ring_buffer_t rb;
    
    ring_buffer_create(&rb, buffer, 8, RING_BUFFER_TYPE_LOCKFREE);
    
    /* 填满缓冲区 */
    uint8_t data[7] = {1, 2, 3, 4, 5, 6, 7};
    uint16_t written = ring_buffer_write_multi(&rb, data, 7);
    TEST_ASSERT(written == 7, "Should write 7 bytes");
    TEST_ASSERT(ring_buffer_is_full(&rb), "Should be full");
    TEST_ASSERT(ring_buffer_free_space(&rb) == 0, "Free space should be 0");
    
    /* 尝试再写入 */
    TEST_ASSERT(!ring_buffer_write(&rb, 0xFF), "Write should fail when full");
    
    ring_buffer_destroy(&rb);
    TEST_PASS();
}

/**
 * @brief 测试6：环绕写入
 */
bool test_wrap_around(void)
{
    uint8_t buffer[8];
    ring_buffer_t rb;
    
    ring_buffer_create(&rb, buffer, 8, RING_BUFFER_TYPE_LOCKFREE);
    
    /* 第一次写入 */
    uint8_t data1[] = {1, 2, 3, 4};
    ring_buffer_write_multi(&rb, data1, 4);
    
    /* 第一次读取 */
    uint8_t temp[4];
    ring_buffer_read_multi(&rb, temp, 4);
    
    /* 第二次写入（触发环绕）*/
    uint8_t data2[] = {5, 6, 7, 8, 9};
    uint16_t written = ring_buffer_write_multi(&rb, data2, 5);
    TEST_ASSERT(written == 5, "Wrap-around write failed");
    
    /* 验证数据 */
    uint8_t read_buf[5];
    ring_buffer_read_multi(&rb, read_buf, 5);
    TEST_ASSERT(memcmp(data2, read_buf, 5) == 0, "Wrap-around data mismatch");
    
    ring_buffer_destroy(&rb);
    TEST_PASS();
}

/**
 * @brief 测试7：部分写入
 */
bool test_partial_write(void)
{
    uint8_t buffer[8];  /* 可用 7 字节 */
    ring_buffer_t rb;
    
    ring_buffer_create(&rb, buffer, 8, RING_BUFFER_TYPE_LOCKFREE);
    
    /* 写入 10 字节，实际只能写入 7 字节 */
    uint8_t data[10] = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9};
    uint16_t written = ring_buffer_write_multi(&rb, data, 10);
    TEST_ASSERT(written == 7, "Should write 7 bytes (partial)");
    
    /* 验证前 7 个字节 */
    uint8_t read_buf[7];
    ring_buffer_read_multi(&rb, read_buf, 7);
    TEST_ASSERT(memcmp(data, read_buf, 7) == 0, "Partial write data mismatch");
    
    ring_buffer_destroy(&rb);
    TEST_PASS();
}

/**
 * @brief 测试8：清空缓冲区
 */
bool test_clear(void)
{
    uint8_t buffer[16];
    ring_buffer_t rb;
    
    ring_buffer_create(&rb, buffer, 16, RING_BUFFER_TYPE_LOCKFREE);
    
    /* 写入数据 */
    uint8_t data[] = {1, 2, 3, 4, 5};
    ring_buffer_write_multi(&rb, data, 5);
    TEST_ASSERT(ring_buffer_available(&rb) == 5, "Should have 5 bytes");
    
    /* 清空 */
    ring_buffer_clear(&rb);
    TEST_ASSERT(ring_buffer_is_empty(&rb), "Should be empty after clear");
    TEST_ASSERT(ring_buffer_available(&rb) == 0, "Available should be 0");
    TEST_ASSERT(ring_buffer_free_space(&rb) == 15, "Free space should be 15");
    
    ring_buffer_destroy(&rb);
    TEST_PASS();
}

/**
 * @brief 测试9：状态查询准确性
 */
bool test_state_queries(void)
{
    uint8_t buffer[16];
    ring_buffer_t rb;
    
    ring_buffer_create(&rb, buffer, 16, RING_BUFFER_TYPE_LOCKFREE);
    
    /* 初始状态 */
    TEST_ASSERT(ring_buffer_is_empty(&rb), "Should be empty initially");
    TEST_ASSERT(!ring_buffer_is_full(&rb), "Should not be full initially");
    TEST_ASSERT(ring_buffer_available(&rb) == 0, "Available should be 0");
    TEST_ASSERT(ring_buffer_free_space(&rb) == 15, "Free space should be 15");
    
    /* 写入 5 字节后 */
    uint8_t data[5] = {1, 2, 3, 4, 5};
    ring_buffer_write_multi(&rb, data, 5);
    TEST_ASSERT(!ring_buffer_is_empty(&rb), "Should not be empty");
    TEST_ASSERT(ring_buffer_available(&rb) == 5, "Available should be 5");
    TEST_ASSERT(ring_buffer_free_space(&rb) == 10, "Free space should be 10");
    
    ring_buffer_destroy(&rb);
    TEST_PASS();
}

#if RING_BUFFER_ENABLE_STATISTICS
/**
 * @brief 测试10：统计功能
 */
bool test_statistics(void)
{
    uint8_t buffer[16];
    ring_buffer_t rb;
    
    ring_buffer_create(&rb, buffer, 16, RING_BUFFER_TYPE_LOCKFREE);
    
    /* 写入操作 */
    ring_buffer_write(&rb, 0xAA);
    ring_buffer_write(&rb, 0xBB);
    
    /* 读取操作 */
    uint8_t data;
    ring_buffer_read(&rb, &data);
    
    /* 获取统计 */
    ring_buffer_stats_t stats;
    ring_buffer_get_stats(&rb, &stats);
    TEST_ASSERT(stats.write_count == 2, "Write count should be 2");
    TEST_ASSERT(stats.read_count == 1, "Read count should be 1");
    
    /* 重置统计 */
    ring_buffer_reset_stats(&rb);
    ring_buffer_get_stats(&rb, &stats);
    TEST_ASSERT(stats.write_count == 0, "Write count should be 0 after reset");
    TEST_ASSERT(stats.read_count == 0, "Read count should be 0 after reset");
    
    ring_buffer_destroy(&rb);
    TEST_PASS();
}
#endif

/**
 * @brief 测试11：最小尺寸边界
 */
bool test_minimum_size(void)
{
    uint8_t buffer[2];
    ring_buffer_t rb;
    
    /* size=2 应该成功 */
    TEST_ASSERT(ring_buffer_create(&rb, buffer, 2, RING_BUFFER_TYPE_LOCKFREE),
                "Size=2 should succeed");
    TEST_ASSERT(ring_buffer_free_space(&rb) == 1, "Size=2 should have 1 byte capacity");
    
    /* 写入 1 字节 */
    TEST_ASSERT(ring_buffer_write(&rb, 0xAA), "Write should succeed");
    TEST_ASSERT(ring_buffer_is_full(&rb), "Should be full");
    
    /* 读取 */
    uint8_t data;
    TEST_ASSERT(ring_buffer_read(&rb, &data), "Read should succeed");
    TEST_ASSERT(data == 0xAA, "Data should be 0xAA");
    
    ring_buffer_destroy(&rb);
    TEST_PASS();
}

/**
 * @brief 测试12：并发场景模拟（无锁模式）
 */
bool test_concurrent_scenario(void)
{
    uint8_t buffer[256];
    ring_buffer_t rb;
    
    ring_buffer_create(&rb, buffer, 256, RING_BUFFER_TYPE_LOCKFREE);
    
    /* 模拟生产者-消费者模式 */
    for (int round = 0; round < 100; round++) {
        /* 生产者：写入 10 字节 */
        uint8_t write_data[10];
        for (int i = 0; i < 10; i++) {
            write_data[i] = (uint8_t)((round * 10 + i) & 0xFF);
        }
        ring_buffer_write_multi(&rb, write_data, 10);
        
        /* 消费者：读取 5 字节 */
        uint8_t read_data[5];
        ring_buffer_read_multi(&rb, read_data, 5);
    }
    
    /* 验证最终状态 */
    TEST_ASSERT(ring_buffer_available(&rb) == 500, "Should have 500 bytes");
    
    ring_buffer_destroy(&rb);
    TEST_PASS();
}

/**
 * @brief 测试13：压力测试
 */
bool test_stress(void)
{
    uint8_t buffer[1024];
    ring_buffer_t rb;
    
    ring_buffer_create(&rb, buffer, 1024, RING_BUFFER_TYPE_LOCKFREE);
    
    /* 1000 轮高频读写 */
    for (int i = 0; i < 1000; i++) {
        uint8_t data[32];
        for (int j = 0; j < 32; j++) {
            data[j] = (uint8_t)((i + j) & 0xFF);
        }
        
        /* 写入 */
        uint16_t written = ring_buffer_write_multi(&rb, data, 32);
        TEST_ASSERT(written > 0, "Write should not fail");
        
        /* 读取部分 */
        uint8_t read_buf[16];
        ring_buffer_read_multi(&rb, read_buf, 16);
    }
    
    ring_buffer_destroy(&rb);
    TEST_PASS();
}

/**
 * @brief 测试14：自定义策略注册
 */
static bool custom_write_called = false;

static bool custom_write(ring_buffer_t *rb, uint8_t data)
{
    custom_write_called = true;
    /* 复用无锁实现 */
    extern const ring_buffer_ops_t ring_buffer_lockfree_ops;
    return ring_buffer_lockfree_ops.write(rb, data);
}

static const ring_buffer_ops_t custom_ops = {
    .write = custom_write,
    .read = NULL,
    .write_multi = NULL,
    .read_multi = NULL,
    .available = NULL,
    .free_space = NULL,
    .is_empty = NULL,
    .is_full = NULL,
    .clear = NULL,
};

bool test_custom_strategy(void)
{
    /* 修复警告：显式转换枚举类型 */
    ring_buffer_type_t custom_type = (ring_buffer_type_t)(RING_BUFFER_TYPE_CUSTOM_BASE + 0);
    
    /* 注册自定义策略 */
    TEST_ASSERT(ring_buffer_register_ops(custom_type, &custom_ops),
                "Register custom ops failed");
    
    /* 使用自定义策略 */
    uint8_t buffer[16];
    ring_buffer_t rb;
    TEST_ASSERT(ring_buffer_create(&rb, buffer, 16, custom_type),
                "Create with custom strategy failed");
    
    /* 验证自定义函数被调用 */
    custom_write_called = false;
    ring_buffer_write(&rb, 0xAA);
    TEST_ASSERT(custom_write_called, "Custom write not called");
    
    ring_buffer_destroy(&rb);
    TEST_PASS();
}

/**
 * @brief 测试15：内存占用报告
 */
bool test_memory_usage(void)
{
    printf("\n========================================\n");
    printf("  Memory Usage Report\n");
    printf("========================================\n");
    printf("ring_buffer_t:      %zu bytes\n", sizeof(ring_buffer_t));
    printf("ring_buffer_ops_t:  %zu bytes\n", sizeof(ring_buffer_ops_t));
    
    /* 示例实例 */
    uint8_t buffer[256];
    ring_buffer_t rb;
    (void)buffer;  /* 避免未使用警告 */
    printf("Example instance:   %zu bytes (256B buffer + %zu B struct)\n",
           sizeof(buffer) + sizeof(rb), sizeof(rb));
    printf("========================================\n\n");
    
    TEST_PASS();
}

/* ==================== 主测试函数 ==================== */

/**
 * @brief 运行所有单元测试
 */
int ring_buffer_run_all_tests(void)
{
    int total = 0, passed = 0, failed = 0;
    
    printf("\n========================================\n");
    printf("  Ring Buffer Unit Tests\n");
    printf("  Component Version: %s\n", ring_buffer_get_version());
    printf("========================================\n\n");
    
    /* 运行所有测试 */
    RUN_TEST(test_version_info);
    RUN_TEST(test_create_destroy);
    RUN_TEST(test_single_byte_write_read);
    RUN_TEST(test_multi_byte_write_read);
    RUN_TEST(test_buffer_full);
    RUN_TEST(test_wrap_around);
    RUN_TEST(test_partial_write);
    RUN_TEST(test_clear);
    RUN_TEST(test_state_queries);
    
#if RING_BUFFER_ENABLE_STATISTICS
    RUN_TEST(test_statistics);
#endif
    
    RUN_TEST(test_minimum_size);
    RUN_TEST(test_concurrent_scenario);
    RUN_TEST(test_stress);
    RUN_TEST(test_custom_strategy);
    RUN_TEST(test_memory_usage);
    
    /* 输出汇总 */
    printf("\n========================================\n");
    printf("  Test Summary\n");
    printf("========================================\n");
    printf("Total:  %d\n", total);
    printf("Passed: %d\n", passed);
    printf("Failed: %d\n", failed);
    printf("========================================\n\n");
    
    if (failed == 0) {
        printf("? All tests passed!\n\n");
        return 0;
    } else {
        printf("? Some tests failed!\n\n");
        return 1;
    }
}
