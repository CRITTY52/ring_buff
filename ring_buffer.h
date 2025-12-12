/**
 * @file    ring_buffer.h
 * @brief   环形缓冲区接口
 * @author  CRITTY.熙影
 * @date    2024-12-27
 * @version 1.1
 */
 
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
 bool ring_buffer_init(ring_buffer_t *rb, uint8_t *buffer, uint16_t size);
 bool ring_buffer_write(ring_buffer_t *rb, uint8_t data);
 bool ring_buffer_read(ring_buffer_t *rb, uint8_t *data);
 uint16_t ring_buffer_write_multi(ring_buffer_t *rb, const uint8_t *data, uint16_t len);
 uint16_t ring_buffer_read_multi(ring_buffer_t *rb, uint8_t *data, uint16_t len);
 uint16_t ring_buffer_available(const ring_buffer_t *rb);
 uint16_t ring_buffer_free_space(const ring_buffer_t *rb);
 bool ring_buffer_is_empty(const ring_buffer_t *rb);
 bool ring_buffer_is_full(const ring_buffer_t *rb);
 void ring_buffer_clear(ring_buffer_t *rb);
 
 /* ---------------------------------- end of file ------------------------------------- */
 #endif 
 