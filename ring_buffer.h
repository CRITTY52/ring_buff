/**
 * @file    ring_buffer.h
 * @brief   环形缓冲区接口
 * @author  CRITTY.熙影
 * @date    2024-12-21
 * @version 2.0.0
 */
 
 /* ===============================================================================
   版本历史 (Version History)
   ===============================================================================
   
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
   [新增] + Feature         新功能，文档等
   [优化] * Improved        性能/代码优化
   [修复] - Bugfix          缺陷修复
   [重构] * Refactor        代码重构
   [文档] # Docs            文档更新
   [破坏性变更] ! Breaking  不兼容的API变更
   ============================================================================= */
   
   #ifndef __RING_BUFFER_H
   #define __RING_BUFFER_H
   
   /* Includes -----------------------------------------------------------------------------*/
   #include <stdint.h>
   #include <stdbool.h>
   
   /* Exported types -----------------------------------------------------------------------*/
   typedef struct {
       uint8_t *buffer;        // 缓冲区指针 
       uint16_t size;          // 缓冲区大小 
       volatile uint16_t head; // 写指针 (write_ptr) 
       volatile uint16_t tail; // 读指针 (read_ptr) 
   } ring_buffer_t;
   
   /* Exported functions -------------------------------------------------------------------*/
   /* 兼容原有接口 */
   typedef struct {
       bool (*init)(ring_buffer_t *rb, uint8_t *buffer, uint16_t size);
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
   
   /* 获取环形缓冲区操作接口函数 */
   const ring_buffer_ops_t *ring_buffer_get_ops(void);
   
   /* ---------------------------------- end of file ------------------------------------- */
   #endif 
  