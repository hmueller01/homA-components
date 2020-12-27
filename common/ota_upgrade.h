/**
 * @file
 * @brief User OTA upgrade functions
 * Thanks to Martin
 * https://harizanov.com/2015/06/firmware-over-the-air-fota-for-esp8266-soc/
 *
 * MIT License
 * Copyright (c) 2017 Holger Mueller
 */
#ifndef __USER_UPGRADE_H__
#define __USER_UPGRADE_H__

#include <c_types.h>

#ifdef __cplusplus
extern "C" {
#endif  // __cplusplus

bool OtaUpgrade(uint16_t server_version);

#ifdef __cplusplus
}
#endif  // __cplusplus

#endif /* __COMMON_H__ */
