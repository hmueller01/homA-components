 /**
  * @file
  * @brief WiFi functions
  * Created on: Dec 30, 2014 by Minh (ESP8266_NONOS_SDK example)
  *
  * MIT License
  * Copyright (c) 2017 Holger Mueller
  */
#ifndef USER_WIFI_H_
#define USER_WIFI_H_

#include <os_type.h>


typedef void (*WifiCallback)(uint8_t);

#ifdef __cplusplus
extern "C" {
#endif // __cplusplus

void ICACHE_FLASH_ATTR WIFI_Connect(uint8_t* ssid, uint8_t* pass, WifiCallback cb);

#ifdef __cplusplus
}
#endif // __cplusplus

#endif /* USER_WIFI_H_ */
