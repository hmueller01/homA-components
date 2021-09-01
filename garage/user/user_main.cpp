/**
 * @file
 * @brief Garage node main application
 *
 * This is the HomA/MQTT garage unit.
 *
 * Params are loaded, WiFi init, MQTT setup and GPIO keys are set.
 * Sensor pulses are counted using an interrupt.
 * Wind speed is calculated and published to broker within a timer.
 *
 * All configuration is done in "user_config.h".
 */

/*
Programming Best Practices
http://www.danielcasner.org/guidelines-for-writing-code-for-the-esp8266/
- Application code should have the ICACHE_FLASH_ATTR decorator unless it is executed very often.
- All interrupt handlers must not have the ICACHE_FLASH_ATTR decorator and any code which executes very often should not have the decorator.
*/

// C++ wrapper
extern "C" {
// put C includes inside here to avoid undefined references by linker.
#include <ets_sys.h>
#include <osapi.h>
#include <gpio.h>
#include <user_interface.h>
#include <mem.h>
#include <time.h>
#include <sntp.h>

#include "user_config.h"
#include "common.h"
#include "ota_upgrade.h"
#include "user_cfg.h"
#include "wifi.h"
#include "dst.h"
#include "mqtt/mqtt.h"
#include "wiringESP/wiringESP.h"
#include "mcp23017.h"
#include "user_telnetd.h"
#include "user_main.h"
}

// global variables
LOCAL Mcp23017 m_mcp23017;
LOCAL MQTT_Client mqttClient;
LOCAL bool mqtt_connected = FALSE;
LOCAL bool send_start_time = FALSE;
LOCAL uint16_t server_version;
uint16_t m_cistern_timeout_time = 0; // [min]
uint16_t m_cistern_timeout_cnt = 0; // [min]
uint8_t m_cistern_level = 0; // cistern level in percent
bool m_cistern_status = OFF; // cistern status ON/OFF
bool m_door_status = OFF; // garage door status OFF=open, ON=closed
LOCAL os_timer_t sntp_timer; // time for NTP service
LOCAL os_timer_t wps_timer; // timeout for WPS key
LOCAL os_timer_t cistern_lvl_timer; // timer to read cistern level
LOCAL os_timer_t m_cistern_timeout_timer; // timer to timeout cistern pump
LOCAL os_timer_t debounce_pin_timer; // timer to read pins debounced
#define MAIN_TASK_QUEUE_LEN   1
LOCAL os_event_t main_task_queue[MAIN_TASK_QUEUE_LEN];
const char *rst_reason_text[] = {
	"normal startup by power on", // REASON_DEFAULT_RST = 0
	"hardware watch dog reset",   // REASON_WDT_RST = 1
	"exception reset",            // REASON_EXCEPTION_RST = 2
	"software watch dog reset",   // REASON_SOFT_WDT_RST = 3
	"sw restart, system_restart()", // REASON_SOFT_RESTART = 4
	"wake up from deep-sleep",    // REASON_DEEP_SLEEP_AWAKE = 5
	"external system reset"       // REASON_EXT_SYS_RST = 6
};


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

	return rf_cal_sec;
}

/**
 ******************************************************************
 * @brief  NTP timer callback.
 * @author Holger Mueller
 * @date   2017-06-11, 2017-07-06, 2017-11-02
 *
 * @param  arg - NULL, not used.
 ******************************************************************
 */
LOCAL void ICACHE_FLASH_ATTR
CheckSntpStamp_Cb(void *arg)
{
	uint32 current_stamp;
	char *time_str;

	current_stamp = applyDST(sntp_get_current_timestamp());
	if (current_stamp != 0) {
		os_timer_disarm(&sntp_timer);
		time_str = sntp_get_real_time(current_stamp);
		if (os_strlen(time_str) > 0)
			time_str[os_strlen(time_str)-1] = 0; // remove tailing \n
		INFO("sntp: %d, %s" CRLF, current_stamp, time_str);
		MQTT_Publish(&mqttClient,
			"/devices/" HOMA_SYSTEM_ID "/controls/Start time",
			time_str, os_strlen(time_str), 1, TRUE);
		send_start_time = TRUE; // do not resend start time until we reboot
	}
}

/**
 ******************************************************************
 * @brief  Debounce pin read timer callback. (door, cistern button)
 *         Do keep this in RAM (no ICACHE_FLASH_ATTR), as it is
 *         called very often.
 * @author Holger Mueller
 * @date   2020-05-23
 *
 * @param  arg - NULL, not used.
 ******************************************************************
 */
LOCAL void
DebouncePinTimer_Cb(void *arg)
{
	LOCAL sint8_t door_pin_cnt = 0; // debounce counter for door pin
	LOCAL sint8_t cistern_sw_pin_cnt = 0; // debounce counter for cistern sw pin

	// debounce garage door pin
	if (digitalRead(PIN_DOOR) == OFF) {
		// door pin is OFF, reset ON counts
		if (door_pin_cnt > 0) {
			door_pin_cnt = 0;
		}
		if (door_pin_cnt > -DEBOUNCE_PIN_CNT_MAX) {
			door_pin_cnt--;
			if (door_pin_cnt == -DEBOUNCE_PIN_CNT_MAX) {
				system_os_post(MAIN_TASK_PRIO, SIG_DOOR_CHANGE, OFF);
			}
		}
	} else {
		// door pin is ON, reset OFF counts
		if (door_pin_cnt < 0) {
			door_pin_cnt = 0;
		}
		if (door_pin_cnt < DEBOUNCE_PIN_CNT_MAX) {
			door_pin_cnt++;
			if (door_pin_cnt == DEBOUNCE_PIN_CNT_MAX) {
				system_os_post(MAIN_TASK_PRIO, SIG_DOOR_CHANGE, ON);
			}
		}
	}

	// debounce cistern switch button pin
	if (digitalRead(PIN_CISTERN_SW) == OFF) {
		// cistern switch pin is OFF, reset ON counts
		if (cistern_sw_pin_cnt > 0) {
			cistern_sw_pin_cnt = 0;
		}
		if (cistern_sw_pin_cnt > -DEBOUNCE_PIN_CNT_MAX) {
			cistern_sw_pin_cnt--;
			if (cistern_sw_pin_cnt == -DEBOUNCE_PIN_CNT_MAX) {
				system_os_post(MAIN_TASK_PRIO, SIG_CISTERN, !digitalRead(PIN_CISTERN));
			}
		}
	} else {
		// cistern switch pin is ON, reset OFF counts
		if (cistern_sw_pin_cnt < 0) {
			cistern_sw_pin_cnt = 0;
		}
		// no need to count positive, as ON has no action
	}
}

/**
 ******************************************************************
 * @brief  Cistern pump timeout timer callback.
 * @author Holger Mueller
 * @date   2018-05-22
 *
 * @param  arg - NULL, not used.
 ******************************************************************
 */
LOCAL void ICACHE_FLASH_ATTR
CisternTimeoutTimer_Cb(void *arg)
{
	bool ret;

	if (m_cistern_timeout_cnt != 0) {
		m_cistern_timeout_cnt--;
		if (m_cistern_timeout_cnt == 0) {
			os_timer_disarm(&m_cistern_timeout_timer);
			// switch pump off in main task
			ret = system_os_post(MAIN_TASK_PRIO, SIG_CISTERN, OFF);
			if (!ret) {
				ERROR("%s: system_os_post() failed, retry in 98 ms" CRLF, __FUNCTION__);
				m_cistern_timeout_cnt = 1;
				os_timer_arm(&m_cistern_timeout_timer, 98, FALSE);
			}
		}
	} else {
		DEBUG("%s: Timer with m_cistern_timeout_cnt == 0 elapsed." CRLF, __FUNCTION__);
		os_timer_disarm(&m_cistern_timeout_timer);
	}
}

/**
 ******************************************************************
 * @brief  Cistern level loop timer callback.
 * @author Holger Mueller
 * @date   2018-05-11, 2018-05-26
 *
 * @param  arg - NULL, not used.
 ******************************************************************
 */
LOCAL void ICACHE_FLASH_ATTR
CisternLvlTimer_Cb(void *arg)
{
	bool ret;

	// read the level in main task
	ret = system_os_post(MAIN_TASK_PRIO, SIG_CISTERN_LVL, 0);
	if (!ret) {
		ERROR("%s: system_os_post() failed." CRLF, __FUNCTION__);
	}
}

/**
 ******************************************************************
 * @brief  Calculate percentage of bit level value.
 * @author Holger Mueller
 * @date   2018-05-18
 *
 * @param  cistern_lvl - Bit value of cistern level (0x1=10%, 0x3=20%, ...).
 * @return Percentage value (0 .. 100)
 ******************************************************************
 */
//
LOCAL uint8_t ICACHE_FLASH_ATTR
CisternGetPercent(uint16_t cistern_lvl)
{
	uint8_t cistern_percent = 0;

	while (cistern_lvl & 0x01) {
		cistern_percent++;
		cistern_lvl >>= 1;
	}

	return cistern_percent * 10;
}

/**
 ******************************************************************
 * @brief  MQTT callback broker connected.
 * @author Holger Mueller
 * @date   2018-03-15, 2018-05-26, 2019-10-13
 * Subscribes to /sys topics, publishes HomA /devices/ structure.
 *
 * @param  args - MQTT_Client structure pointer.
 ******************************************************************
 */
LOCAL void ICACHE_FLASH_ATTR
MqttConnected_Cb(uint32_t *args)
{
	char tmp_str20[20];
	char *rst_reason;
	MQTT_Client *client = (MQTT_Client *) args;

	INFO("MQTT: Connected" CRLF);
	mqtt_connected = TRUE;

	MQTT_Subscribe(client, "/sys/" HOMA_SYSTEM_ID "/#", 1);

	// setup HomA device topics
	//MQTT_Publish(*client, topic, data, data_length, qos, retain)
	MQTT_Publish(client, "/devices/" HOMA_SYSTEM_ID "/meta/room", HOMA_ROOM, os_strlen(HOMA_ROOM), 1, TRUE);
	MQTT_Publish(client, "/devices/" HOMA_SYSTEM_ID "/meta/name", HOMA_DEVICE, os_strlen(HOMA_DEVICE), 1, TRUE);

	MQTT_Publish(client, "/devices/" HOMA_SYSTEM_ID "/controls/Garage door/meta/type", "text", 4, 1, TRUE);
	MQTT_Publish(client, "/devices/" HOMA_SYSTEM_ID "/controls/Garage door/meta/unit", "", 0, 1, TRUE);
	MQTT_Publish(client, "/devices/" HOMA_SYSTEM_ID "/controls/Garage door/meta/room", HOMA_HOME, os_strlen(HOMA_HOME), 1, TRUE);
	MQTT_Publish(client, "/devices/" HOMA_SYSTEM_ID "/controls/Cistern/meta/type", "switch", 6, 1, TRUE);
	MQTT_Publish(client, "/devices/" HOMA_SYSTEM_ID "/controls/Cistern/meta/room", HOMA_HOME, os_strlen(HOMA_HOME), 1, TRUE);
	MQTT_Subscribe(client, "/devices/" HOMA_SYSTEM_ID "/controls/Cistern/on", 1);
	MQTT_Publish(client, "/devices/" HOMA_SYSTEM_ID "/controls/Cistern",
			digitalRead(PIN_CISTERN) == OFF ? "0" : "1", 1, 1, TRUE);
	MQTT_Publish(client, "/devices/" HOMA_SYSTEM_ID "/controls/Cistern level/meta/type", "text", 4, 1, TRUE);
	MQTT_Publish(client, "/devices/" HOMA_SYSTEM_ID "/controls/Cistern level/meta/unit", " %%", 2, 1, TRUE);
	MQTT_Publish(client, "/devices/" HOMA_SYSTEM_ID "/controls/Cistern level/meta/room", HOMA_HOME, os_strlen(HOMA_HOME), 1, TRUE);

	MQTT_Publish(client, "/devices/" HOMA_SYSTEM_ID "/controls/Garage door/meta/order", "1", 1, 1, TRUE);
	MQTT_Publish(client, "/devices/" HOMA_SYSTEM_ID "/controls/Cistern/meta/order", "2", 1, 1, TRUE);
	MQTT_Publish(client, "/devices/" HOMA_SYSTEM_ID "/controls/Cistern level/meta/order", "3", 1, 1, TRUE);
	MQTT_Publish(client, "/devices/" HOMA_SYSTEM_ID "/controls/Reset reason/meta/order", "4", 1, 1, TRUE);
	MQTT_Publish(client, "/devices/" HOMA_SYSTEM_ID "/controls/Device id/meta/order", "5", 1, 1, TRUE);
	MQTT_Publish(client, "/devices/" HOMA_SYSTEM_ID "/controls/Device IP/meta/order", "6", 1, 1, TRUE);
	MQTT_Publish(client, "/devices/" HOMA_SYSTEM_ID "/controls/Version/meta/order", "7", 1, 1, TRUE);
	MQTT_Publish(client, "/devices/" HOMA_SYSTEM_ID "/controls/Start time/meta/order", "8", 1, 1, TRUE);
	MQTT_Publish(client, "/devices/" HOMA_SYSTEM_ID "/controls/State/meta/order", "9", 1, 1, TRUE);

	MQTT_Publish(client, "/devices/" HOMA_SYSTEM_ID "/controls/Device id",
		sysCfg.device_id, os_strlen(sysCfg.device_id), 1, TRUE);

	struct ip_info ip_config;
	wifi_get_ip_info(STATION_IF, &ip_config);
	os_sprintf(tmp_str20, IPSTR, IP2STR(&ip_config.ip.addr));
	MQTT_Publish(client, "/devices/" HOMA_SYSTEM_ID "/controls/Device IP",
		tmp_str20, os_strlen(tmp_str20), 1, TRUE);

	itoa(tmp_str20, APP_VERSION);
	MQTT_Publish(client, "/devices/" HOMA_SYSTEM_ID "/controls/Version",
		tmp_str20, os_strlen(tmp_str20), 1, TRUE);
	rst_reason = (char *) rst_reason_text[system_get_rst_info()->reason];
	MQTT_Publish(client, "/devices/" HOMA_SYSTEM_ID "/controls/Reset reason",
		rst_reason, os_strlen(rst_reason), 1, TRUE);
	// set LWT message, that we are alive
	MQTT_Publish(client, "/devices/" HOMA_SYSTEM_ID "/controls/State",
		"online", 6, 1, TRUE);

	// do only resend start time if we reboot,
	// do not if we got a Wifi reconnect ...
	if (!send_start_time) {
		os_timer_disarm(&sntp_timer);
		os_timer_setfn(&sntp_timer, (os_timer_func_t *)CheckSntpStamp_Cb, NULL);
		os_timer_arm(&sntp_timer, 100, TRUE);
	}
}

/**
 ******************************************************************
 * @brief  MQTT callback broker disconnected.
 * @author Holger Mueller
 * @date   2017-06-08
 *
 * @param  args - MQTT_Client structure pointer.
 ******************************************************************
 */
LOCAL void ICACHE_FLASH_ATTR
MqttDisconnected_Cb(uint32_t * args)
{
	//MQTT_Client *client = (MQTT_Client *) args;
	INFO("MQTT: Disconnected" CRLF);
	mqtt_connected = FALSE;
}

/**
 ******************************************************************
 * @brief  MQTT callback message/topic published.
 * @author Holger Mueller
 * @date   2017-06-06
 *
 * @param  args - MQTT_Client structure pointer.
 ******************************************************************
 */
LOCAL void ICACHE_FLASH_ATTR
MqttPublished_Cb(uint32_t * args)
{
	//MQTT_Client *client = (MQTT_Client *) args;
	//INFO("MQTT: Published" CRLF);
}

/**
 ******************************************************************
 * @brief  MQTT callback message/topic received.
 * @author Holger Mueller
 * @date   2017-06-08, 2018-03-15, 2018-05-22
 *
 * @param  args - MQTT_Client structure pointer.
 ******************************************************************
 */
LOCAL void ICACHE_FLASH_ATTR
MqttData_Cb(uint32_t *args, const char *topic_raw, uint32_t topic_len, const char *data_raw, uint32_t data_len)
{
	char versionBuf[20];
	char *topic = (char *)os_zalloc(topic_len + 1);
	char *data = (char *)os_zalloc(data_len + 1);

	MQTT_Client *client = (MQTT_Client *) args;

	os_memcpy(topic, topic_raw, topic_len);
	topic[topic_len] = 0;

	os_memcpy(data, data_raw, data_len);
	data[data_len] = 0;

	INFO("%s: Receive topic: %s, data: %s" CRLF, __FUNCTION__, topic, data);

	if (strcmp(topic, "/sys/" HOMA_SYSTEM_ID "/server_version") == 0) {
		server_version = atoi(data);
		INFO("Received server version %d" CRLF, server_version);
		if (server_version <= APP_VERSION) {
			INFO("%s: No upgrade. Server version=%d, local version=%d" CRLF,
				__FUNCTION__, server_version, APP_VERSION);
			server_version = 0; // reset server version
		} else {
			// tell user that upgrade is in progress
			os_sprintf(versionBuf, "upgrading to %d", server_version);
			MQTT_Publish(client, "/devices/" HOMA_SYSTEM_ID "/controls/Version",
				versionBuf, os_strlen(versionBuf), 1, TRUE);
			// do the main work in a Task, not here in the callback
			system_os_post(MAIN_TASK_PRIO, SIG_UPGRADE, 0);
		}
	} else if (strcmp(topic, "/sys/" HOMA_SYSTEM_ID "/cistern_time") == 0) {
		m_cistern_timeout_time = atoi(data);
		INFO("Received cistern_time %d" CRLF, m_cistern_timeout_time);
	} else if (os_strncmp(topic, "/devices/" HOMA_SYSTEM_ID "/controls/Cistern/on",
			os_strlen("/devices/" HOMA_SYSTEM_ID "/controls/Cistern/on")) == 0) {
		// the cistern pump should be switched on or off
		system_os_post(MAIN_TASK_PRIO, SIG_CISTERN, data[0] == '1' ? ON : OFF);
	} else {
		ERROR("%s: Error. Unknown topic: %s" CRLF,
			__FUNCTION__, topic);
	}

	os_free(topic);
	os_free(data);
}

/**
 ******************************************************************
 * @brief  Do all the stuff that need network after we got an ip.
 * @author Holger Mueller
 * @date   2017-06-08, 2017-10-31
 ******************************************************************
 */
LOCAL void ICACHE_FLASH_ATTR
WifiGotIp(void)
{
	MQTT_Connect(&mqttClient);

	// setup NTP
	sntp_setservername(0, (char*) "de.pool.ntp.org"); // set server 0 by domain name
	sntp_setservername(1, (char*) "europe.pool.ntp.org"); // set server 1 by domain name
	sntp_setservername(2, (char*) "time.nist.gov"); // set server 2 by domain name
	sntp_set_timezone(1); // set Berlin timezone (GMT+1)
	sntp_init();

	// establish the telnetd configuration terminal
	Telnetd_Init(23);
}

#ifdef WPS
/**
 ******************************************************************
 * @brief  WiFi WPS status callback.
 * @author Holger Mueller
 * @date   2017-06-06, 2018-03-15
 *
 * @param  status - WPS status. See SDK documentation.
 ******************************************************************
 */
LOCAL void ICACHE_FLASH_ATTR
UserWpsStatus_Cb(int status)
{
	switch (status) {
	case WPS_CB_ST_SUCCESS:
		INFO("%s: WPS_CB_ST_SUCCESS" CRLF, __FUNCTION__);
		wifi_wps_disable();
		wifi_station_connect();
		break;
	case WPS_CB_ST_FAILED:
	case WPS_CB_ST_TIMEOUT:
		ERROR("%s: WPS_CB_ST_FAILED or WPS_CB_ST_TIMEOUT" CRLF, __FUNCTION__);
		wifi_wps_disable();
		break;
	}
}

/**
 ******************************************************************
 * @brief  WiFi event handler if WPS is enabled.
 * @author Holger Mueller
 * @date   2017-06-07
 *
 * @param  *evt - WiFi system event pointer. See SDK documentation.
 ******************************************************************
 */
LOCAL void ICACHE_FLASH_ATTR
WifiWpsHandleEvent_Cb(System_Event_t *evt_p)
{
	//INFO("%s: %s" CRLF, __FUNCTION__, wifi_event[evt->event]);
	switch (evt_p->event) {
	case EVENT_STAMODE_DISCONNECTED:
		INFO("%s: disconnect from ssid %s, reason %d" CRLF, __FUNCTION__,
				evt_p->event_info.disconnected.ssid,
				evt_p->event_info.disconnected.reason);
		MQTT_Disconnect(&mqttClient);
		break;
	case EVENT_STAMODE_GOT_IP:
		INFO("%s: WiFi connected." CRLF, __FUNCTION__);
		/*INFO("ip: " IPSTR ", mask: " IPSTR ", gw: " IPSTR CRLF,
				IP2STR(&evt_p->event_info.got_ip.ip),
				IP2STR(&evt_p->event_info.got_ip.mask),
				IP2STR(&evt_p->event_info.got_ip.gw));*/
		// start tasks that need network
		WifiGotIp();
		break;
	default:
		break;
	}
}

#else // WPS

/**
 ******************************************************************
 * @brief  WiFi event handler if WPS is disabled.
 * @author Holger Mueller
 * @date   2017-06-06
 *
 * @param  status - WiFi status. See wifi.c and
 *                  wifi_station_get_connect_status()
 ******************************************************************
 */
LOCAL void ICACHE_FLASH_ATTR
WifiConnect_Cb(uint8_t status)
{
	if (status == STATION_GOT_IP) {
		WifiGotIp();
	} else {
		MQTT_Disconnect(&mqttClient);
	}
}
#endif // WPS

/**
 ******************************************************************
 * @brief  WPS key's long press function, needed to be installed.
 *         Starts WPS push button function.
 * @author Holger Mueller
 * @date   2017-06-07
 *
 * @param  arg - NULL, not used.
 ******************************************************************
 */
LOCAL void ICACHE_FLASH_ATTR
WpsLongPress_Cb(void *arg)
{
#ifdef WPS
	INFO("%s: starting WPS push button ..." CRLF, __FUNCTION__);
	wifi_wps_disable();
	wifi_wps_enable(WPS_TYPE_PBC);
	wifi_set_wps_cb(UserWpsStatus_Cb);
	wifi_wps_start();
#endif // WPS
}

/**
 ******************************************************************
 * @brief  WPS key's key change call back. Starts/stops timer to
 *         detect long (5s) press.
 * @author Holger Mueller
 * @date   2018-03-15
 ******************************************************************
 */
LOCAL void ICACHE_FLASH_ATTR
WpsPinChange_Cb(void)
{
	// Keep the Interrupt Service Routine (ISR) / callback short.
	// Do not use “serial print” commands in an ISR. These can hang the system.
	if (0 == digitalRead(PIN_WPS)) {
		// key is pressed, wait 5s if key is still pressed
		os_timer_disarm(&wps_timer);
		os_timer_setfn(&wps_timer, (os_timer_func_t *)WpsLongPress_Cb, NULL);
		os_timer_arm(&wps_timer, 5000, FALSE);
		INFO("%s: key press detected, arming timer ..." CRLF, __FUNCTION__);
	} else {
		// key released, stop timer
		os_timer_disarm(&wps_timer);
		INFO("%s: key release detected, disarming timer ..." CRLF, __FUNCTION__);
	}
}

/**
 ******************************************************************
 * @brief  Main task for publishing data.
 * @author Holger Mueller
 * @date   2018-03-15, 2018-05-18, 2018-05-22
 *
 * @param  *event_p - message queue pointer set by system_os_post().
 ******************************************************************
 */
LOCAL void ICACHE_FLASH_ATTR
Main_Task(os_event_t *event_p)
{
	uint16_t cistern_lvl;
	char cistern_str[10];

	switch (event_p->sig) {
	case SIG_CISTERN:
		INFO("%s: Got signal 'SIG_CISTERN'. par=%d" CRLF, __FUNCTION__, event_p->par);
		m_cistern_status = (bool) event_p->par;
		MQTT_Publish(&mqttClient, "/devices/" HOMA_SYSTEM_ID "/controls/Cistern",
				event_p->par == ON ? "1" : "0", 1, 1, TRUE);
		digitalWrite(PIN_CISTERN, event_p->par);
		// set timeout of cistern pump (auto switch off)
		if (event_p->par == ON) {
			os_timer_disarm(&m_cistern_timeout_timer);
			m_cistern_timeout_cnt = m_cistern_timeout_time;
			os_timer_setfn(&m_cistern_timeout_timer, (os_timer_func_t *)CisternTimeoutTimer_Cb, NULL);
			os_timer_arm(&m_cistern_timeout_timer, 60 * 1000, TRUE); // 1 min timer
		} else {
			os_timer_disarm(&m_cistern_timeout_timer);
			m_cistern_timeout_cnt = 0;
		}
		break;
	case SIG_CISTERN_LVL:
		INFO("%s: Got signal 'SIG_CISTERN_LVL'." CRLF, __FUNCTION__);
		m_mcp23017.digitalWrite(CISTERN_LVL_BTN, LOW); // start measurement
		delay(50); // wait 50 ms to settle measurement
		cistern_lvl = m_mcp23017.digitalRead16() & 0x03FF;
		m_mcp23017.digitalWrite(CISTERN_LVL_BTN, HIGH); // stop measurement (BTN off)
		m_cistern_level = CisternGetPercent(cistern_lvl);
		itoa(cistern_str, m_cistern_level);
		INFO("%s: MCP23017 cistern_lvl=0x%X, cistern_percent=%s" CRLF, __FUNCTION__, cistern_lvl, cistern_str);
		MQTT_Publish(&mqttClient, "/devices/" HOMA_SYSTEM_ID "/controls/Cistern level",
			cistern_str, os_strlen(cistern_str), 1, TRUE);
		break;
	case SIG_DOOR_CHANGE:
		INFO("%s: Got signal 'SIG_DOOR_CHANGE'. par=%d" CRLF, __FUNCTION__, event_p->par);
		m_door_status = (bool) event_p->par;
		if (OFF == event_p->par) {
			MQTT_Publish(&mqttClient, "/devices/" HOMA_SYSTEM_ID "/controls/Garage door",
					"open", 4, 1, TRUE);
		} else {
			MQTT_Publish(&mqttClient, "/devices/" HOMA_SYSTEM_ID "/controls/Garage door",
					"closed", 6, 1, TRUE);
		}
		break;
	case SIG_UPGRADE:
		INFO("%s: Got signal 'SIG_UPGRADE'." CRLF, __FUNCTION__);
		if (!OtaUpgrade(server_version)) {
			// No upgrade will be done
		}
		server_version = 0; // reset server version
		break;
	default:
		ERROR("%s: Unknown signal %d." CRLF, __FUNCTION__, event_p->sig);
		break;
	}
} // Main_Task


/**
 ******************************************************************
 * @brief  Main user init function.
 * @author Holger Mueller
 * @date   2018-03-15, ..., 2020-12-28
 ******************************************************************
 */
void ICACHE_FLASH_ATTR
user_init(void)
{
	// if you do not set the uart, ESP8266 will start with 74880 baud :-(
	//uart_div_modify(0, UART_CLK_FREQ / 115200);
	//INFO("SDK version: %s" CRLF, system_get_sdk_version());
	INFO("Garage version: %d" CRLF, APP_VERSION);
	INFO("Reset reason: %s" CRLF, rst_reason_text[system_get_rst_info()->reason]);
	INFO("HomA device ID: %s" CRLF, HOMA_SYSTEM_ID);
	INFO("Chip ID: %08X" CRLF, system_get_chip_id());
	INFO("Memory info:" CRLF);
	system_print_meminfo();

	mqtt_connected = FALSE;
	send_start_time = FALSE;
	CFG_Load();

#ifdef WPS
	INFO("WiFi WPS setup ..." CRLF);
    wifi_set_event_handler_cb(WifiWpsHandleEvent_Cb);
	wifi_set_opmode(STATION_MODE);
	wifi_station_set_hostname(sysCfg.device_id);
#else // WPS
	WIFI_Connect(sysCfg.sta_ssid, sysCfg.sta_pwd, WifiConnect_Cb);
	wifi_station_set_hostname(sysCfg.device_id);
#endif // WPS

	// setup Main_Task (for doing time-consuming things)
	system_os_task(Main_Task, MAIN_TASK_PRIO, main_task_queue, MAIN_TASK_QUEUE_LEN);

	// setup needed GPIO pins
	attachInterrupt(PIN_WPS, WpsPinChange_Cb, CHANGE);
	pinMode(PIN_DOOR, INPUT_PULLUP);
	pinMode(PIN_CISTERN_SW, INPUT_PULLUP);
	pinMode(PIN_CISTERN, OUTPUT);
	digitalWrite(PIN_CISTERN, OFF);

	// setup cyclic decounced pin reading
	os_timer_disarm(&debounce_pin_timer);
	os_timer_setfn(&debounce_pin_timer, (os_timer_func_t *)DebouncePinTimer_Cb, NULL);
	os_timer_arm(&debounce_pin_timer, DEBOUNCE_PIN_TIMER, TRUE);

	// setup cyclic sending of cistern level
	os_timer_disarm(&cistern_lvl_timer);
	os_timer_setfn(&cistern_lvl_timer, (os_timer_func_t *)CisternLvlTimer_Cb, NULL);
	os_timer_arm(&cistern_lvl_timer, CISTERN_LVL_TIMER, TRUE);

	// setup MQTT
	//MQTT_InitConnection(&mqttClient, sysCfg.mqtt_host, sysCfg.mqtt_port, sysCfg.security);
	MQTT_InitConnection(&mqttClient, MQTT_HOST, MQTT_PORT, MQTT_SECURITY);
	//MQTT_InitClient(&mqttClient, sysCfg.device_id, sysCfg.mqtt_user, sysCfg.mqtt_pass, sysCfg.mqtt_keepalive, 1);
	MQTT_InitClient(&mqttClient, sysCfg.device_id, MQTT_USER, MQTT_PASS, MQTT_KEEPALIVE, FALSE);
	MQTT_InitLWT(&mqttClient, "/devices/" HOMA_SYSTEM_ID "/controls/State", "offline", 1, TRUE);
	MQTT_OnConnected(&mqttClient, MqttConnected_Cb);
	MQTT_OnDisconnected(&mqttClient, MqttDisconnected_Cb);
	MQTT_OnPublished(&mqttClient, MqttPublished_Cb);
	MQTT_OnData(&mqttClient, MqttData_Cb);

	// setup MCP23017 GPIO expander
	INFO("MCP23017 setup ..." CRLF);
	m_mcp23017.begin(PIN_SDA, PIN_SCL, 0xFF, 0xFF, 0xFF, 0x03, 0xFF, 0x03);
	m_mcp23017.pinMode(CISTERN_LVL_BTN, MCP23017_OUTPUT);
	m_mcp23017.digitalWrite(CISTERN_LVL_BTN, HIGH); // HIGH is BTN not pressed
	m_mcp23017.dumpRegs();

	// read cistern level at once (not first time in 10 min.)
	system_os_post(MAIN_TASK_PRIO, SIG_CISTERN_LVL, 0);

	INFO("System started." CRLF CRLF);
}
