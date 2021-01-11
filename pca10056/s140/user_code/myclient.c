#include "myclient.h"
#include <stdint.h>
#include "nrf_log.h"
#include "nrf_log_ctrl.h"
#include <string.h>
#include "app_timer.h"
#include "app_uart.h"
#include <stdarg.h>
#include "ble_nus_c.h"

#include "app_fstorage.h"

APP_TIMER_DEF(scan_timeout_id);

#define SCAN_RECORD_ADDR      (200 * 4096)

scan_list_info_t scan_list;

static void uart_send_str(char *data);
static void uart_send_log(char *data);
static uint8_t uart_log_buffer[BLE_NUS_MAX_DATA_LEN + 1];

static scan_record_t scan_record_in_flash;

static void uart_send_str(char *data)
{
    size_t len = 0;
    len = strlen(data);
    for (int i = 0; i < len; i++) {
        app_uart_put(data[i]);
    }
}

static void uart_send_log(char *data)
{
    uart_send_str(data);
}

void UART_LOG_INFO(const char * format, ...)
{
    memset(uart_log_buffer,0, BLE_NUS_MAX_DATA_LEN+1);

    va_list arg;
    va_start(arg, format);

    vsnprintf((char *)uart_log_buffer, BLE_NUS_MAX_DATA_LEN, format, arg);
    uint16_t length = strlen((char *)uart_log_buffer);
    if(length > 256)
    {
        length = 256;
    }
    uart_send_log((char *)uart_log_buffer);
    va_end(arg);

}
static uint8_t scan_timeout_start_f = 0;
static void scan_timeout_handler(void *p_context)
{
    scan_list.scan_timeout = 1;
    scan_timeout_start_f = 0;
    UART_LOG_INFO("Please select a device to connect (at+rescan) >> \r\n");
}

static void scan_timeout_start(void)
{
    if (scan_timeout_start_f == 1)
        return ;
    scan_timeout_start_f = 1;
    app_timer_start(scan_timeout_id, APP_TIMER_TICKS(2000), NULL);
}

static void scan_timeout_stop(void)
{
    if (scan_timeout_start_f == 0)
        return;
    scan_timeout_start_f = 0;
    app_timer_stop(scan_timeout_id);
    UART_LOG_INFO("Please select a device to connect >> \r\n");
}

static uint32_t clear_scan_dev_list(void)
{
    memset(&scan_list, 0, SCAN_LIST_SIZE * 6);
    scan_list.match_index = 255;
    return 0;
}

void myclient_init(void)
{
    NRF_LOG_INFO("%s", __func__);
    clear_scan_dev_list();

    app_timer_create(&scan_timeout_id, APP_TIMER_MODE_SINGLE_SHOT, scan_timeout_handler);

    uint8_t flash_read_buff[32];
    myfstorage_read(SCAN_RECORD_ADDR, flash_read_buff, sizeof(scan_record_t));
    NRF_LOG_INFO("scan_record_t size: %d", sizeof(scan_record_t));

    memcpy(&scan_record_in_flash, flash_read_buff, sizeof(scan_record_t));
    if (scan_record_in_flash.valid == 0x55)
    {
        char buff[32] = {0};
        uint8_t *mac_addr = scan_record_in_flash.mac_addr;
        snprintf(buff, 32, "%02X:%02X:%02X:%02X:%02X:%02X", mac_addr[0], mac_addr[1], mac_addr[2],\
                 mac_addr[3], mac_addr[4], mac_addr[5]);
        UART_LOG_INFO("Default connect DEV: %s\r\n", buff);

        scan_list.connected = 1;
        scan_list.match_index = 0;
        memcpy(scan_list.dev_info[scan_list.match_index].mac_addr, scan_record_in_flash.mac_addr, sizeof(uint8_t) * 6);
    }
}

static void get_scan_list_start(void)
{
    clear_scan_dev_list();
    //scan_timeout_start();
}

#define MIN_CONNECTION_INTERVAL     MSEC_TO_UNITS(20, UNIT_1_25_MS)    /**< Determines minimum connection interval in milliseconds. */
#define MAX_CONNECTION_INTERVAL     MSEC_TO_UNITS(75, UNIT_1_25_MS)    /**< Determines maximum connection interval in milliseconds. */
#define SLAVE_LATENCY               0                                   /**< Determines slave latency in terms of connection events. */
#define SUPERVISION_TIMEOUT         MSEC_TO_UNITS(4000, UNIT_10_MS)     /**< Determines supervision time-out in units of 10 milliseconds. */



static ble_gap_conn_params_t const m_connection_param =
{
    (uint16_t)MIN_CONNECTION_INTERVAL,
    (uint16_t)MAX_CONNECTION_INTERVAL,
    (uint16_t)SLAVE_LATENCY,
    (uint16_t)SUPERVISION_TIMEOUT
};
extern void scan_stop(void);
uint32_t get_scan_list(ble_gap_addr_t *peer_addr, ble_gap_scan_params_t *p_scan_param)
{
    if (scan_list.scan_timeout == 1)
    {
        return 0;
    }
    if (scan_list.connected == 1)
    {
        if (memcmp(peer_addr->addr, scan_list.dev_info[scan_list.match_index].mac_addr, 6) == 0)
        {
            NRF_LOG_INFO("Start connecting...");
            ret_code_t ret_code = sd_ble_gap_connect(peer_addr, p_scan_param, &m_connection_param, 1);
            if (ret_code != NRF_SUCCESS)
            {
                NRF_LOG_INFO("connect failed.");
            } else
            {
                UART_LOG_INFO("Connected DEV[%d] success.\r\n", scan_list.match_index);
                //UART_LOG_INFO("%c", 0x0c);
            }
            get_scan_list_start();
        }
        return 0;
    }
    if (scan_list.scan_nums >= SCAN_LIST_SIZE)
    {
        scan_timeout_stop();
        return 0;
    }

    scan_timeout_start();
    for (uint8_t i = 0; i < scan_list.scan_nums; i++)
    {
        if (0 == memcmp(peer_addr->addr, scan_list.dev_info[i].mac_addr, 6))
        {
            return 0;
        }
    }

    memcpy(&scan_list.dev_info[scan_list.scan_nums].mac_addr, peer_addr->addr, 6);
    char buff[32] = {0};
    uint8_t *mac_addr = peer_addr->addr;
    snprintf(buff, 32, "%02X:%02X:%02X:%02X:%02X:%02X", mac_addr[0], mac_addr[1], mac_addr[2],\
             mac_addr[3], mac_addr[4], mac_addr[5]);
    UART_LOG_INFO("DEV [%d]: %s\r\n", scan_list.scan_nums, buff);

    scan_list.scan_nums++;
    return 0;
}

static uint8_t write_mac_addr[6] = {0};
static uint8_t write_flag = 0;

void my_client(void)
{
    if (write_flag == 1)
    {
        NRF_LOG_INFO("write scan record in flash.");
        write_flag = 0;
        memcpy(scan_record_in_flash.mac_addr, write_mac_addr, sizeof(uint8_t) * 6);
        scan_record_in_flash.valid = 0x55;
        myfstorage_write(SCAN_RECORD_ADDR, (void *)&scan_record_in_flash, sizeof(scan_record_t));
    }
}

uint8_t set_connect_index(uint8_t *data)
{
    if (data == NULL)
    {
        NRF_LOG_INFO("[%s] data null", __func__);
        return 2;
    }
    int index = 255;
    sscanf((const char *)data, "%d", &index);
    if (index >= 0 && index < scan_list.scan_nums)
    {
        NRF_LOG_INFO("[%s] index: %d", __func__, index);
        scan_list.match_index = index;
        scan_list.connected = 1;
        scan_list.scan_timeout = 0;

        if ( 0 != memcmp(scan_list.dev_info[scan_list.match_index].mac_addr, scan_record_in_flash.mac_addr, sizeof(uint8_t) * 6))
        {
            NRF_LOG_INFO("scan record update.");
            write_flag = 1;
            memcpy(write_mac_addr, scan_list.dev_info[scan_list.match_index].mac_addr, sizeof(uint8_t) * 6);
        }

        NRF_LOG_INFO("connect index: %02x%02x", scan_list.dev_info[index].mac_addr[0], \
                     scan_list.dev_info[index].mac_addr[1]);
        return 0;
    }
    UART_LOG_INFO("Input is wrong.\r\n");
    return 3;
}

extern uint8_t m_ble_connected;
extern uint16_t m_ble_conn_handle;
extern void scan_start(void);
void at_cmd_process(uint8_t *data, uint16_t size)
{
    NRF_LOG_INFO("[%s]", __func__);
    if (data == NULL)
    {
        NRF_LOG_INFO("[%s] data null", __func__);
        return ;
    }

    NRF_LOG_INFO("cmd: %s", data);
    if ( 0 == strncmp((const char *)(data + 3), "rescan", 6) ||
            0 == strncmp((const char *)(data + 3), "RESCAN", 6) )
    {
        if (m_ble_connected == 0)
        {
			scan_list.scan_timeout = 0;
            get_scan_list_start();
			scan_list.connected = 0;
			NRF_LOG_INFO("get_scan_list_start");
            return ;
        }
        NRF_LOG_INFO("[%s] start scan...", __func__);
        uint32_t ret_code = sd_ble_gap_disconnect(m_ble_conn_handle, BLE_HCI_REMOTE_USER_TERMINATED_CONNECTION);
        if (ret_code != 0)
        {
            NRF_LOG_INFO("disconnect failed.");
            return ;
        }
        scan_start();
    }
    else
    {
        UART_LOG_INFO("AT cmd error.\r\n");
    }
}

