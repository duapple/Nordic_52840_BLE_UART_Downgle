#include "app_fstorage.h"

#include "nrf_log.h"
#include "nrf_log_ctrl.h"
#include "nrf_fstorage.h"
#include "nrf_fstorage_sd.h"
#include "nrf_pwr_mgmt.h"
#include "app_error.h"
#include "nrf_delay.h"

#define ERROR_CHECK(error) \
	do{ \
		if (error) { \
			NRF_LOG_INFO("LINE[%d] error code: %d\r\n", __LINE__, error); \
		} \
	} while(0);


static void fstorage_evt_handler(nrf_fstorage_evt_t * p_evt)
{
    if (p_evt->result != NRF_SUCCESS)
    {
        NRF_LOG_INFO("--> Event received: ERROR while executing an fstorage operation.");
        return;
    }

    switch (p_evt->id)
    {
    case NRF_FSTORAGE_EVT_WRITE_RESULT:
    {
        NRF_LOG_INFO("--> Event received: wrote %d bytes at address 0x%x.",
                     p_evt->len, p_evt->addr);
    }
    break;

    case NRF_FSTORAGE_EVT_ERASE_RESULT:
    {
        NRF_LOG_INFO("--> Event received: erased %d page from address 0x%x.",
                     p_evt->len, p_evt->addr);
    }
    break;

    default:
        break;
    }
}

NRF_FSTORAGE_DEF(nrf_fstorage_t fstorage) =
{
    .evt_handler = fstorage_evt_handler,
    .start_addr = 200 * 4096,
    .end_addr = 255 * 4096,
};

static void print_flash_info(nrf_fstorage_t * p_fstorage)
{
    NRF_LOG_INFO("========| flash info |========");
    NRF_LOG_INFO("erase unit: \t%d bytes",      p_fstorage->p_flash_info->erase_unit);
    NRF_LOG_INFO("program unit: \t%d bytes",    p_fstorage->p_flash_info->program_unit);
    NRF_LOG_INFO("==============================");
}

static uint32_t nrf5_flash_end_addr_get()
{
    uint32_t const bootloader_addr = NRF_UICR->NRFFW[0];
    uint32_t const page_sz         = NRF_FICR->CODEPAGESIZE;
    uint32_t const code_sz         = NRF_FICR->CODESIZE;

    return (bootloader_addr != 0xFFFFFFFF ?
            bootloader_addr : (code_sz * page_sz));
}

void wait_for_flash_ready(nrf_fstorage_t const * p_fstorage)
{
    /* While fstorage is busy, sleep and wait for an event. */
    while (nrf_fstorage_is_busy(p_fstorage))
    {
        nrf_pwr_mgmt_run();
    }
}


void app_fstorage_init(void)
{
	ret_code_t rc;
	
//	uint32_t m_data = 0xBADC0FFE;
//	char m_hello_world[] = "hello world";
//	
//	uint8_t read_bf_u32[32] = {0};
//	uint8_t read_bf_chr[32] = {0};
//	
//	uint32_t addr_1 = 125*4096;
//	uint32_t addr_2 = 126*4096;
	
	nrf_fstorage_api_t *p_fs_api;
	p_fs_api = &nrf_fstorage_sd;
	
	rc = nrf_fstorage_init(&fstorage, p_fs_api, NULL);
	APP_ERROR_CHECK(rc);
	
	print_flash_info(&fstorage);
	
	(void) nrf5_flash_end_addr_get();
	
//	rc = nrf_fstorage_erase(&fstorage, addr_1, 2, NULL);
//	APP_ERROR_CHECK(rc);
//	wait_for_flash_ready(&fstorage);
//	UART_LOG_INFO("erase ok.\r\n");

//	
//	UART_LOG_INFO("Writing \"%x\" to flash.", m_data);
//	rc = nrf_fstorage_write(&fstorage, addr_1, &m_data, sizeof(m_data), NULL);
//	APP_ERROR_CHECK(rc);
//	
//	wait_for_flash_ready(&fstorage);
//	UART_LOG_INFO("write Done.\r\n");

//	
//	rc = nrf_fstorage_erase(&fstorage, addr_1, 1, NULL);
//	APP_ERROR_CHECK(rc);
//	wait_for_flash_ready(&fstorage);
//	UART_LOG_INFO("erase ok.\r\n");	

//	
//	m_data = 0xDEADBEEF;
//	UART_LOG_INFO("Writing \"%x\" to flash.\r\n", m_data);
//	rc = nrf_fstorage_write(&fstorage, addr_1, &m_data, sizeof(m_data), NULL);
//	APP_ERROR_CHECK(rc);
//	
//	wait_for_flash_ready(&fstorage);
//	UART_LOG_INFO("write Done.\r\n");
//	
//	memset(read_bf_u32, 0, 4);
//	rc = nrf_fstorage_read(&fstorage, addr_1, read_bf_u32, sizeof(m_data));
//	APP_ERROR_CHECK(rc);
//	UART_LOG_INFO("read m_data: %2x%2x%2x%2x\r\n", read_bf_u32[0], read_bf_u32[1], read_bf_u32[2], read_bf_u32[3]);

//	UART_LOG_INFO("Writing \"%s\" to flash.\r\n", m_hello_world);
//	rc = nrf_fstorage_write(&fstorage, addr_2, m_hello_world, sizeof(m_hello_world), NULL);
//	APP_ERROR_CHECK(rc);
//	
//	wait_for_flash_ready(&fstorage);
//	UART_LOG_INFO("Write Done.\r\n");
//	
//	rc = nrf_fstorage_read(&fstorage, addr_2, read_bf_chr, sizeof(m_hello_world));
//	APP_ERROR_CHECK(rc);
//	wait_for_flash_ready(&fstorage);
//	UART_LOG_INFO("Read Done.\r\n");
//	UART_LOG_INFO("read m_hello_world: %s\r\n", read_bf_chr);
}

uint32_t myfstorage_read(uint32_t read_addr, void *p_data, uint16_t data_len)
{
	if (!p_data)
	{
		return 1;
	}

	uint32_t ret_code = nrf_fstorage_read(&fstorage, read_addr, p_data, data_len);
	APP_ERROR_CHECK(ret_code);
	wait_for_flash_ready(&fstorage);
	return ret_code;
}

uint32_t myfstorage_write(uint32_t write_addr, void *p_data, uint16_t data_len)
{
	if (!p_data)
	{
		return 1;
	}

	uint32_t ret_code = nrf_fstorage_erase(&fstorage, write_addr, 1, NULL);
	APP_ERROR_CHECK(ret_code);
	wait_for_flash_ready(&fstorage);

	ret_code = nrf_fstorage_write(&fstorage, write_addr, p_data, data_len, NULL);
	APP_ERROR_CHECK(ret_code);
	wait_for_flash_ready(&fstorage);
	return ret_code;
}

