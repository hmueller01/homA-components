/**
 * @file
 * @brief Configure IoT device here.
 *        This is a template. Real configuration shall NOT go to version control!
 *        Copy this file to user_config.h before editing!
 * @author Holger Mueller
 */
#ifndef __USER_CONFIG_H__
#define __USER_CONFIG_H__

#include "mqtt/mqtt.h"

#define APP_VERSION 1

// define DEBUG_ON to show debug output or use make FLAVOR=debug
#ifndef DEBUG_ON
//#define DEBUG_ON
#endif // DEBUG_ON

// define to enable WPS
#define WPS

// WiFi ssid and password, if WPS is not used.
#define STA_SSID	""
#define STA_PASS	""

// configure FOTA IP or domain name, port and path on server
#define OTA_HOST	"UPDATE.YOURDOMAIN"
#define OTA_PORT	80
#define OTA_PATH	"/esp8266fw/rcplugs/"

// HomA definitions
#define HOMA_SYSTEM_ID "123456-rcplugs"

// MQTT broker host settings
#define MQTT_HOST		"MQTT.YOURDOMAIN" // MQTT IP or domain name
#define MQTT_USER		"" // MQTT username, set NULL if anonymous is allowed
#define MQTT_PASS		"" // MQTT password, set NULL if anonymous is allowed
#define MQTT_CLIENT_ID	"ESP-%08X"
#define MQTT_SECURITY	SEC_SSL_ONE_WAY_AUTH // security of the connection
#if MQTT_SECURITY == SEC_NONSSL
#define MQTT_PORT		1883
#else
#define MQTT_PORT		8883
#endif
#define MQTT_KEEPALIVE	120 // seconds

// configuration of GPIO pins
#define PIN_433TX		12
#define PIN_WPS			13

#define USE_OPTIMIZE_PRINTF

#endif /* __USER_CONFIG_H__ */
