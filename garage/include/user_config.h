/**
 * @file
 * @brief Configure IoT device here
 * @author hmueller
 */
#ifndef __USER_CONFIG_H__
#define __USER_CONFIG_H__

#define APP_VERSION 13

// define DEBUG_ON to show debug output
#define DEBUG_ON

// define to enable WPS
#define WPS

// configure FOTA IP or domain name, port and path on server
#define OTA_HOST "update.euhm"
#define OTA_PORT 80
#define OTA_PATH "/esp8266fw/garage/"

// HomA definitions
#define HOMA_SYSTEM_ID	"123456-garage"
#define HOMA_DEVICE		"Garage"
#define HOMA_ROOM		"Garage"
#define HOMA_HOME		"Home"

// configuration of GPIO ports (see wiringESP.c)
#define PIN_SDA 0
#define PIN_SCL 2
#define PIN_WPS 14
#define PIN_DOOR 12
#define PIN_CISTERN 13

// configuration of MCP23017 GPIO ports
#define CISTERN_LVL_BTN 15

// configuration of cistern level read frequency (10 min.)
#define CISTERN_TIMER (10 * 60 * 1000)

#define USE_OPTIMIZE_PRINTF

// MQTT settings see mqtt_config.h
#include "mqtt_config.h"

#endif

