#ifndef __APP_FSTORAGE_H
#define __APP_FSTORAGE_H

#include <stdint.h>

void app_fstorage_init(void);
uint32_t myfstorage_read(uint32_t read_addr, void *p_data, uint16_t data_len);
uint32_t myfstorage_write(uint32_t write_addr, void *p_data, uint16_t data_len);

#endif /* __APP_FSTORAGE_H */


