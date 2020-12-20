/**
 * @file
 * @brief Register SDKs partition table.
 * @author Holger Mueller
 *
 * For ESP8266_NONOS_SDK_v3.0.0 and later versions, it is
 * required to register your partition table in user_pre_init.
 *
 * During compile time define SPI_FLASH_SIZE_MAP accordingly:
 * 0= 512KB( 256KB+ 256KB) not supported
 * 1= ? not supported
 * 2=1024KB( 512KB+ 512KB)
 * 3=2048KB( 512KB+ 512KB)
 * 4=4096KB( 512KB+ 512KB)
 * 5=2048KB(1024KB+1024KB)
 * 6=4096KB(1024KB+1024KB)
 *
 * For ESP8266_NONOS_SDK before v3.0.0, it is
 * required to set the RF cal sector in user_rf_cal_sector_set.
 */

#include <osapi.h>
#include <user_interface.h>
#include <version.h>

#include "common.h"


// global variables
uint32 priv_param_start_sec;


#if ESP_SDK_VERSION_NUMBER >= 0x030000

#ifdef __cplusplus
extern "C" {
#endif
	void user_pre_init(void);
#ifdef __cplusplus
}
#endif


#if ((SPI_FLASH_SIZE_MAP == 0) || (SPI_FLASH_SIZE_MAP == 1))
#error "The flash map is not supported"
#elif (SPI_FLASH_SIZE_MAP == 2)
#define SYSTEM_PARTITION_OTA_SIZE					0x6A000
#define SYSTEM_PARTITION_OTA_2_ADDR					0x81000
#define SYSTEM_PARTITION_RF_CAL_ADDR				0xfb000
#define SYSTEM_PARTITION_PHY_DATA_ADDR				0xfc000
#define SYSTEM_PARTITION_SYSTEM_PARAMETER_ADDR		0xfd000
#define SYSTEM_PARTITION_CUSTOMER_PRIV_PARAM_ADDR	0x7c000
#elif (SPI_FLASH_SIZE_MAP == 3)
#define SYSTEM_PARTITION_OTA_SIZE					0x6A000
#define SYSTEM_PARTITION_OTA_2_ADDR					0x81000
#define SYSTEM_PARTITION_RF_CAL_ADDR				0x1fb000
#define SYSTEM_PARTITION_PHY_DATA_ADDR				0x1fc000
#define SYSTEM_PARTITION_SYSTEM_PARAMETER_ADDR		0x1fd000
#define SYSTEM_PARTITION_CUSTOMER_PRIV_PARAM_ADDR	0x7c000
#elif (SPI_FLASH_SIZE_MAP == 4)
#define SYSTEM_PARTITION_OTA_SIZE					0x6A000
#define SYSTEM_PARTITION_OTA_2_ADDR					0x81000
#define SYSTEM_PARTITION_RF_CAL_ADDR				0x3fb000
#define SYSTEM_PARTITION_PHY_DATA_ADDR				0x3fc000
#define SYSTEM_PARTITION_SYSTEM_PARAMETER_ADDR		0x3fd000
#define SYSTEM_PARTITION_CUSTOMER_PRIV_PARAM_ADDR	0x7c000
#elif (SPI_FLASH_SIZE_MAP == 5)
#define SYSTEM_PARTITION_OTA_SIZE					0x6A000
#define SYSTEM_PARTITION_OTA_2_ADDR					0x101000
#define SYSTEM_PARTITION_RF_CAL_ADDR				0x1fb000
#define SYSTEM_PARTITION_PHY_DATA_ADDR				0x1fc000
#define SYSTEM_PARTITION_SYSTEM_PARAMETER_ADDR		0x1fd000
#define SYSTEM_PARTITION_CUSTOMER_PRIV_PARAM_ADDR	0xfc000
#elif (SPI_FLASH_SIZE_MAP == 6)
#define SYSTEM_PARTITION_OTA_SIZE					0x6A000
#define SYSTEM_PARTITION_OTA_2_ADDR					0x101000
#define SYSTEM_PARTITION_RF_CAL_ADDR				0x3fb000
#define SYSTEM_PARTITION_PHY_DATA_ADDR				0x3fc000
#define SYSTEM_PARTITION_SYSTEM_PARAMETER_ADDR		0x3fd000
#define SYSTEM_PARTITION_CUSTOMER_PRIV_PARAM_ADDR	0xfc000
#else
#error "The flash map is not supported"
#endif

#define SYSTEM_PARTITION_CUSTOMER_PRIV_PARAM		SYSTEM_PARTITION_CUSTOMER_BEGIN

static const partition_item_t at_partition_table[] = {
	{ SYSTEM_PARTITION_BOOTLOADER, 			0x0, 										0x1000},
	{ SYSTEM_PARTITION_OTA_1,  				0x1000, 									SYSTEM_PARTITION_OTA_SIZE},
	{ SYSTEM_PARTITION_OTA_2,   			SYSTEM_PARTITION_OTA_2_ADDR, 				SYSTEM_PARTITION_OTA_SIZE},
	{ SYSTEM_PARTITION_RF_CAL,  			SYSTEM_PARTITION_RF_CAL_ADDR,				0x1000},
	{ SYSTEM_PARTITION_PHY_DATA, 			SYSTEM_PARTITION_PHY_DATA_ADDR, 			0x1000},
	{ SYSTEM_PARTITION_SYSTEM_PARAMETER, 	SYSTEM_PARTITION_SYSTEM_PARAMETER_ADDR,		0x3000},
	{ SYSTEM_PARTITION_CUSTOMER_PRIV_PARAM,	SYSTEM_PARTITION_CUSTOMER_PRIV_PARAM_ADDR,	0x1000},
};


/**
 ******************************************************************
 * @brief  Register partition table and read sector for private parameter.
 * @author Holger Mueller
 * @date   2020-06-14
 * For ESP8266_NONOS_SDK_v3.0.0 and later versions, it is
 * required to register your partition table in user_pre_init.
 ******************************************************************
 */
void ICACHE_FLASH_ATTR
user_pre_init(void)
{
	partition_item_t partition_item;

	if (!system_partition_table_regist(at_partition_table,
		sizeof(at_partition_table)/sizeof(at_partition_table[0]), SPI_FLASH_SIZE_MAP)) {
		ERROR("%s: system_partition_table_regist() fail" CRLF, __FUNCTION__);
		while(1);
	}

	if (!system_partition_get_item(SYSTEM_PARTITION_CUSTOMER_PRIV_PARAM, &partition_item)) {
		ERROR("%s: system_partition_get_item() fail" CRLF, __FUNCTION__);
		while(1);
	}
	priv_param_start_sec = partition_item.addr / SPI_FLASH_SEC_SIZE;
}

#else // ESP_SDK_VERSION_NUMBER >= 0x030000

#ifdef __cplusplus
extern "C" {
#endif
	uint32 user_rf_cal_sector_set(void);
#ifdef __cplusplus
}
#endif

#define CFG_LOCATION	0x79	// Please don't change or if you know what you doing

/**
 ******************************************************************
 * @brief  SDK just reversed 4 sectors, used for rf init data and paramters.
 * @author Holger Mueller
 * @date   2017-06-08
 * We add this function to force users to set rf cal sector, since
 * we don't know which sector is free in user's application.
 * Sector map for last several sectors: ABCCC
 * A : rf cal
 * B : rf init data
 * C : sdk parameters
 *
 * @return rf cal sector
 ******************************************************************
 */
uint32 ICACHE_FLASH_ATTR
user_rf_cal_sector_set(void)
{
	enum flash_size_map size_map = system_get_flash_size_map();
	uint32 rf_cal_sec = 0;

	INFO(CRLF);
	switch (size_map) {
	case FLASH_SIZE_4M_MAP_256_256:
		INFO("%s: FLASH_SIZE_4M_MAP_256_256" CRLF, __FUNCTION__);
		rf_cal_sec = 128 - 5;
		break;
	case FLASH_SIZE_8M_MAP_512_512:
		INFO("%s: FLASH_SIZE_8M_MAP_512_512" CRLF, __FUNCTION__);
		rf_cal_sec = 256 - 5;
		break;
	case FLASH_SIZE_16M_MAP_512_512:
	case FLASH_SIZE_16M_MAP_1024_1024:
		INFO("%s: FLASH_SIZE_16M_MAP_512_512" CRLF, __FUNCTION__);
		rf_cal_sec = 512 - 5;
		break;
	case FLASH_SIZE_32M_MAP_512_512:
	case FLASH_SIZE_32M_MAP_1024_1024:
		INFO("%s: FLASH_SIZE_32M_MAP_512_512" CRLF, __FUNCTION__);
		rf_cal_sec = 1024 - 5;
		break;
	case FLASH_SIZE_64M_MAP_1024_1024:
		INFO("%s: FLASH_SIZE_64M_MAP_1024_1024" CRLF, __FUNCTION__);
		rf_cal_sec = 2048 - 5;
		break;
	case FLASH_SIZE_128M_MAP_1024_1024:
		INFO("%s: FLASH_SIZE_128M_MAP_1024_1024" CRLF, __FUNCTION__);
		rf_cal_sec = 4096 - 5;
		break;
	default:
		INFO("%s: default 0?!" CRLF, __FUNCTION__);
		rf_cal_sec = 0;
		break;
	}

	priv_param_start_sec = CFG_LOCATION;

	return rf_cal_sec;
}
#endif // ESP_SDK_VERSION_NUMBER < 0x030000
