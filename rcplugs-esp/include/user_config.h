/**
 * @file
 * @brief Configure IoT device here
 * @author Holger Mueller
 */
#ifndef __USER_CONFIG_H__
#define __USER_CONFIG_H__

#include "mqtt/mqtt.h"

#define APP_VERSION 37

// define DEBUG_ON to show debug output
#define DEBUG_ON
// define if you want MQTT debug info messages
//#define MQTT_DEBUG_ON

// define to enable WPS
#define WPS

// configure FOTA IP or domain name, port and path on server
#define OTA_HOST "update.euhm"
#define OTA_PORT 80
#define OTA_PATH "/esp8266fw/rcplugs/"

// HomA definitions
#define HOMA_SYSTEM_ID "123456-rcplugs"
//#define HOMA_DEVICE "rcplugs"
//#define HOMA_ROOM "rcPlugs"

// MQTT boker host settings
#define MQTT_HOST			"mqtt.euhm" // IP or domain name
#define MQTT_CLIENT_ID		"ESP-%08X"
#define MQTT_SECURITY		SEC_SSL_ONE_WAY_AUTH // security of the connection
#if MQTT_SECURITY == SEC_NONSSL
#define MQTT_PORT			1883
#else
#define MQTT_PORT			8883
#endif
#define MQTT_KEEPALIVE		120		// seconds

// configuration of GPIO pins
#define PIN_433TX		12
#define PIN_WPS			13

#define USE_OPTIMIZE_PRINTF

// Missing ESP8266 SDK function prototypes and defines.
#include "ets_missing.h"

// Secret MQTT and WiFi settings see user_secret.h
#include "user_secret.h"

#endif /* __USER_CONFIG_H__ */
