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
#define HOMA_SYSTEM_ID	"123456-garage"
#define HOMA_DEVICE		"Garage"
#define HOMA_ROOM		"Garage"
#define HOMA_HOME		"Home"

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

// configuration of GPIO ports (see wiringESP.c)
#define PIN_SDA 0
#define PIN_SCL 2
#define PIN_WPS 14
#define PIN_DOOR 12
#define PIN_CISTERN 13
#define PIN_CISTERN_SW 4

// configuration of MCP23017 GPIO ports
#define CISTERN_LVL_BTN 15

// configuration of cistern level read frequency (10 min.)
#define CISTERN_LVL_TIMER (10 * 60 * 1000)

// configuration of debounced pin read frequency (50 ms)
#define DEBOUNCE_PIN_TIMER (50)

// pin debounce counter (OFF: 0 .. -5, ON: 0 .. 5) -> 250 ms read delay
#define DEBOUNCE_PIN_CNT_MAX 5

#define USE_OPTIMIZE_PRINTF

#endif /* __USER_CONFIG_H__ */
