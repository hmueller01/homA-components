/*
/* user_cfg.c
*
* Copyright (c) 2014-2015, Tuan PM <tuanpm at live dot com>
* All rights reserved.
*
* Redistribution and use in source and binary forms, with or without
* modification, are permitted provided that the following conditions are met:
*
* * Redistributions of source code must retain the above copyright notice,
* this list of conditions and the following disclaimer.
* * Redistributions in binary form must reproduce the above copyright
* notice, this list of conditions and the following disclaimer in the
* documentation and/or other materials provided with the distribution.
* * Neither the name of Redis nor the names of its contributors may be used
* to endorse or promote products derived from this software without
* specific prior written permission.
*
* THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
* AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
* IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
* ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
* LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
* CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
* SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
* INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
* CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
* ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
* POSSIBILITY OF SUCH DAMAGE.
*/
#include <ets_sys.h>
#include <os_type.h>
#include <mem.h>
#include <osapi.h>
#include <user_interface.h>

#include "user_config.h"
#include "user_partition.h"
#include "user_cfg.h"
#include "common.h"

SYSCFG sysCfg;
SAVE_FLAG saveFlag;

void ICACHE_FLASH_ATTR
CFG_Save(void)
{
	spi_flash_read((priv_param_start_sec + 3) * SPI_FLASH_SEC_SIZE,
			(uint32 *)&saveFlag, sizeof(SAVE_FLAG));

	if (saveFlag.flag == 0) {
		spi_flash_erase_sector(priv_param_start_sec + 1);
		spi_flash_write((priv_param_start_sec + 1) * SPI_FLASH_SEC_SIZE,
				(uint32 *)&sysCfg, sizeof(SYSCFG));
		saveFlag.flag = 1;
	} else {
		spi_flash_erase_sector(priv_param_start_sec + 0);
		spi_flash_write((priv_param_start_sec + 0) * SPI_FLASH_SEC_SIZE,
				(uint32 *)&sysCfg, sizeof(SYSCFG));
		saveFlag.flag = 0;
	}

	spi_flash_erase_sector(priv_param_start_sec + 3);
	spi_flash_write((priv_param_start_sec + 3) * SPI_FLASH_SEC_SIZE,
			(uint32 *)&saveFlag, sizeof(SAVE_FLAG));
}

void ICACHE_FLASH_ATTR
CFG_Load(void)
{
	INFO("%s: loading from sector 0x%02x" CRLF, __FUNCTION__, priv_param_start_sec);
	spi_flash_read((priv_param_start_sec + 3) * SPI_FLASH_SEC_SIZE,
			(uint32 *)&saveFlag, sizeof(SAVE_FLAG));
	if (saveFlag.flag == 0) {
		spi_flash_read((priv_param_start_sec + 0) * SPI_FLASH_SEC_SIZE,
				(uint32 *)&sysCfg, sizeof(SYSCFG));
	} else {
		spi_flash_read((priv_param_start_sec + 1) * SPI_FLASH_SEC_SIZE,
				(uint32 *)&sysCfg, sizeof(SYSCFG));
	}
	if (sysCfg.cfg_holder != CFG_HOLDER) {
		INFO("%s: setting default configuration" CRLF, __FUNCTION__);
		os_memset(&sysCfg, 0x00, sizeof sysCfg);

		sysCfg.cfg_holder = CFG_HOLDER;

		os_sprintf(sysCfg.device_id, MQTT_CLIENT_ID, system_get_chip_id());
		sysCfg.device_id[sizeof(sysCfg.device_id) - 1] = '\0';
		os_strncpy(sysCfg.sta_ssid, STA_SSID, sizeof(sysCfg.sta_ssid) - 1);
		os_strncpy(sysCfg.sta_pwd, STA_PASS, sizeof(sysCfg.sta_pwd) - 1);

		os_strncpy(sysCfg.mqtt_host, MQTT_HOST, sizeof(sysCfg.mqtt_host) - 1);
		sysCfg.mqtt_port = MQTT_PORT;
		os_strncpy(sysCfg.mqtt_user, MQTT_USER, sizeof(sysCfg.mqtt_user) - 1);
		os_strncpy(sysCfg.mqtt_pass, MQTT_PASS, sizeof(sysCfg.mqtt_pass) - 1);

		sysCfg.security = MQTT_SECURITY;	// default non ssl

		sysCfg.mqtt_keepalive = MQTT_KEEPALIVE;

		CFG_Save();
	}
}
