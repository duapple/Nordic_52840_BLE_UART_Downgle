#ifndef __MYCLIENT_H
#define __MYCLIENT_H

#include <stdint.h>
#include "ble_nus_c.h"

#define SCAN_LIST_SIZE          (20)

typedef struct {
    uint8_t dev_name[32];
    uint8_t mac_addr[6];
} scan_dev_info_t;

typedef struct {
    uint8_t scan_nums;
    uint8_t scan_timeout;
    uint8_t match_index;
	uint8_t connected;
    scan_dev_info_t dev_info[SCAN_LIST_SIZE];
} scan_list_info_t;

typedef struct {
	uint8_t valid; // 0x55ÎªÓÐÐ§
	uint8_t reserve_byte;
	uint8_t mac_addr[6];
	uint8_t dev_name[32];
} scan_record_t;

void my_client(void);
uint32_t get_scan_list(ble_gap_evt_adv_report_t const *p_not_found, ble_gap_scan_params_t *p_scan_param);
void myclient_init(void);
uint8_t set_connect_index(uint8_t *data);

void UART_LOG_INFO(const char * format, ...);

void at_cmd_process(uint8_t *data, uint16_t size);

#endif /* __MYCLIENT_H */
