/**
 * @file
 * @brief Telnet server (telnetd)
 * @author Holger Mueller
 */
#include <osapi.h>
#include <mem.h>
#include <user_interface.h>
#include <espconn.h>

#include "common.h"
#include "user_main.h"
#include "user_telnetd.h"
#include "wiringESP/wiringESP.h"


// global variables
LOCAL char *recvbuffer_p = NULL;
LOCAL uint32 recv_length = 0;
LOCAL uint16 cnt_connected = 0; // if cnt_connected > 0 system must not sleep!
LOCAL bool restart_flag = false; // if true restart system after telnetd disconnect
LOCAL struct espconn m_esp_conn;
LOCAL esp_tcp m_esp_tcp;


/**
 ******************************************************************
 * @brief  Save data locally until we get a CR.
 * @author Holger Mueller
 * @date   2015-09-26
 *
 * @param  *precv - The received data.
 * @param  length - The length of received data.
 * @return True, if we received CR.
 ******************************************************************
 */
LOCAL bool ICACHE_FLASH_ATTR
SaveData(char *recv_p, uint16 length)
{
	char *temp_p = NULL;

	if (recvbuffer_p == NULL) {
		recvbuffer_p = (char *) os_zalloc(REC_BUF_LEN);
		recv_length = 0;
	}

	if (recv_length + length < REC_BUF_LEN) {
		// append received data to saved data
		os_memcpy(recvbuffer_p + recv_length, recv_p, length);
		recv_length += length;
	} else {
		// buffer overflow, reset buffer!!!
		os_bzero(recvbuffer_p, REC_BUF_LEN);
		recv_length = 0;
	}

	temp_p = (char *) os_strstr(recvbuffer_p, CR);
	if (temp_p != NULL) {
		// replace CR by \0 to make strxxx functions work
		temp_p[0] = 0x00;
		return true;
	} else {
		return false;
	}
}

/**
 ******************************************************************
 * @brief  Processing the received data from the server.
 * @author Holger Mueller
 * @date   2015-09-26, 2015-10-18, 2018-03-27, 2018-04-16
 *
 * @param  *arg - espconn structure passed to the callback function
 * @param  usrdata_p - The received data (or NULL when the connection has been closed!).
 * @param  length - The length of received data.
 ******************************************************************
 */
LOCAL void ICACHE_FLASH_ATTR
RecvCb_Listen(void *arg, char *usrdata_p, unsigned short length)
{
	struct espconn *esp_conn_p = (struct espconn *)arg;
	bool parse_flag = false;
	/*
	char *tmp_data;
	tmp_data = (char *)os_zalloc(length + 1);
	os_memcpy(tmp_data, usrdata_p, length);
	os_printf("len: %u\n", length);
	os_printf("data: %s\n", tmp_data);
	os_printf("hex data:");
	for (int i = 0; i <= length; i++) os_printf(" %02X", (unsigned char) tmp_data[i]);
	os_printf(CRLF);
	os_free(tmp_data);
	*/

	if ((length > 1) && ((uint8_t) usrdata_p[0] == 0xFF)) {
		// Telnet Negotiation, see
		// http://mud-dev.wikidot.com/telnet:negotiation
		// https://users.cs.cf.ac.uk/Dave.Marshall/Internet/node139.html
		// IAC,<type of operation>,<option>
		// IAC (255, 0xFF) - Interpret as command.
		// <type of operation>
		// WILL (251, 0xFB) - Sender wants to do something.
		// DO (252, 0xFD) - Sender wants the other end to do something.

		// currently just ignore this ...
		return;
	}

	parse_flag = SaveData(usrdata_p, length);
	if (parse_flag) {
		if (!os_strcmp(recvbuffer_p, "help")) {
			char send_data[] = "Possible commands:" CRLF
					"info - print info and current settings" CRLF
					"get - get current cistern values" CRLF
					"set <param>=<value> - enter 'help set' for more information" CRLF
					"restart - restart ESP8266 module" CRLF
					"exit - exit connection" CRLF
					PROMPT;
			espconn_send(esp_conn_p, (uint8_t *) send_data, strlen(send_data));
		} else if (!os_strcmp(recvbuffer_p, "help set")) {
			char send_data[] = "set <param>=<value> help" CRLF
					"Possible parameter <param>:" CRLF
					"cistern - switch cistern pump \"on\" or \"off\"" CRLF
					PROMPT;
			espconn_send(esp_conn_p, (uint8_t *) send_data, strlen(send_data));
		} else if (!os_strcmp(recvbuffer_p, "close") ||
				!os_strcmp(recvbuffer_p, "exit") ||
				!os_strcmp(recvbuffer_p, "quit") ||
				!os_strcmp(recvbuffer_p, "restart")) {
			char send_data[] = "Bye bye ..." CRLF;
			espconn_send(esp_conn_p, (uint8_t *) send_data, strlen(send_data));
			espconn_disconnect(esp_conn_p);
			if (!os_strcmp(recvbuffer_p, "restart")) {
				// set restart flag, to restart after disconnect
				restart_flag = true;
			}
		} else if (!os_strcmp(recvbuffer_p, "info")) {
			char send_data[256];
			struct ip_info info;
			uint8_t hwaddr[6];

			wifi_get_ip_info(STATION_IF, &info);
			wifi_get_macaddr(STATION_IF, hwaddr);
			os_sprintf(send_data,
					"SDK version: %s" CRLF
					"App version: %d" CRLF
					"Hostname: %s" CRLF
					"IP: " IPSTR " / " IPSTR ", GW: " IPSTR CRLF
					"MAC: " MACSTR CRLF
					"Sensor ID: %06x" CRLF
					"Garage door status: %s" CRLF
					"Cistern status: %s" CRLF
					"Cistern level: %d%%" CRLF
					"Cistern time: %d/%d min" CRLF
					PROMPT,
					system_get_sdk_version(),
					APP_VERSION,
					wifi_station_get_hostname(),
					IP2STR(&info.ip.addr), IP2STR(&info.netmask.addr), IP2STR(&info.gw.addr),
					MAC2STR(hwaddr),
					system_get_chip_id(),
					m_door_status == OFF ? "OPEN" : "CLOSED",
					m_cistern_status == OFF ? "OFF" : "ON",
					m_cistern_level,
					m_cistern_timeout_cnt, m_cistern_timeout_time);
			espconn_send(esp_conn_p, (uint8_t *) send_data, strlen(send_data));
		} else if (!os_strcmp(recvbuffer_p, "get")) {
			char send_data[256] = {0};

			// post signal to UserMainTask to read cistern level
			system_os_post(MAIN_TASK_PRIO, SIG_CISTERN_LVL, 0);
			os_sprintf(send_data, "Reading cistern level in main task ..." CRLF PROMPT);
			espconn_send(esp_conn_p, (uint8_t *) send_data, strlen(send_data));
		} else if (!os_strncmp(recvbuffer_p, "set ", 4)) {
			char send_data[256] = {0};
			char *param_p;
			char *value_p;

			param_p = recvbuffer_p + 4; // skip "set "
			value_p = (char *) os_strstr(param_p, "=");
			if (value_p != NULL) {
				// terminate param with \0 (replace the "=")
				value_p[0] = 0x00;
				// set value pointer after "=" now \0
				value_p++;
				if (!os_strcmp(param_p, "cistern")) {
					if (!os_strcmp(value_p, "on")) {
						os_sprintf(send_data, "Switch cistern pump on.");
						// post signal to UserMainTask to switch cistern pump
						system_os_post(MAIN_TASK_PRIO, SIG_CISTERN, ON);
					} else {
						os_sprintf(send_data, "Switch cistern pump off.");
						// post signal to UserMainTask to switch cistern pump
						system_os_post(MAIN_TASK_PRIO, SIG_CISTERN, OFF);
					}
				} else {
					os_sprintf(send_data, "Unknown param '%s'.", param_p);
				}
			} else {
				os_sprintf(send_data, "No value found (missing '=') for param '%s'.", param_p);
			}
			DEBUG("%s" CRLF, send_data);
			os_strcat(send_data, CRLF PROMPT);
			espconn_send(esp_conn_p, (uint8_t *) send_data, strlen(send_data));
		} else if (os_strlen(recvbuffer_p) != 0) {
			char send_data[256];
			os_sprintf(send_data, "Unknown command '%s'. Type help!" CRLF PROMPT, recvbuffer_p);
			espconn_send(esp_conn_p, (uint8_t *) send_data, strlen(send_data));
		} else {
			// only enter pressed, so just send the prompt
			char send_data[] = PROMPT;
			espconn_send(esp_conn_p, (uint8_t *) send_data, strlen(send_data));
		}
		// TODO: does not disconnect telnet client
		// Does not work even work in DisconCb_Listen()
		// restart system, if flag is set (because command was send)
		if (restart_flag) {
			system_restart();
		}
		// free local receive buffer
		if (recvbuffer_p != NULL) {
			os_free(recvbuffer_p);
			recvbuffer_p = NULL;
		}
	}
}

/**
 ******************************************************************
 * @brief  The connection has an error, reconnection.
 * @author Holger Mueller
 * @date   2015-09-24
 *
 * @param  *arg - espconn structure passed to the callback function
 ******************************************************************
 */
LOCAL ICACHE_FLASH_ATTR
void ReconCb_Listen(void *arg, sint8 err)
{
	struct espconn *esp_conn_p = arg;

	DEBUG("telnetd: client " IPSTR ":%d error %d reconnect" CRLF,
			IP2STR(&esp_conn_p->proto.tcp->remote_ip),
			esp_conn_p->proto.tcp->remote_port, err);
}

/**
 ******************************************************************
 * @brief  The connection has been ended.
 * @author Holger Mueller
 * @date   2015-09-24, 2015-10-06
 *
 * @param  *arg - espconn structure passed to the callback function
 ******************************************************************
 */
LOCAL ICACHE_FLASH_ATTR
void DisconCb_Listen(void *arg)
{
	struct espconn *esp_conn_p = arg;

	DEBUG("telnetd: client " IPSTR ":%d disconnected" CRLF,
			IP2STR(&esp_conn_p->proto.tcp->remote_ip),
			esp_conn_p->proto.tcp->remote_port);
	// remove connection from counter
	if (cnt_connected > 0)
		cnt_connected--;
}

/**
 ******************************************************************
 * @brief  Server listened a connection successfully.
 * @author Holger Mueller
 * @date   2015-09-26, 2015-10-06, 2018-11-05, 2020-11-28
 *
 * @param  *arg - espconn structure passed to the callback function
 ******************************************************************
 */
LOCAL void ICACHE_FLASH_ATTR
ConnectCb_Listen(void *arg)
{
	struct espconn *esp_conn_p = arg;
	sint8 ret;

	DEBUG("telnetd: client " IPSTR ":%d connected" CRLF,
			IP2STR(&esp_conn_p->proto.tcp->remote_ip),
			esp_conn_p->proto.tcp->remote_port);

	// add new connection to counter
	cnt_connected++;

	// enable write buffer without delay
	ret = espconn_set_opt(esp_conn_p, ESPCONN_COPY | ESPCONN_NODELAY);
	if (ret) {
		DEBUG("telnetd: espconn_set_opt() failed. (%d)" CRLF, ret);
	}

	espconn_regist_recvcb(esp_conn_p, RecvCb_Listen);
	espconn_regist_reconcb(esp_conn_p, ReconCb_Listen);
	espconn_regist_disconcb(esp_conn_p, DisconCb_Listen);

	char send_data[] = "Welcome to ESP8266." CRLF
			"Enter 'help' to get help." CRLF PROMPT;
	espconn_send(esp_conn_p, (uint8_t *) send_data, strlen(send_data));
}

/**
 ******************************************************************
 * @brief  Log data to serial line and if available the telnet
 *         session.
 * @author Holger Mueller
 * @date   2018-11-10, 2020-11-28
 *
 * @param  *data - output data string, starts with LF+"D"/"E"/"I"+" "
 ******************************************************************
 */
void ICACHE_FLASH_ATTR
Log__(const char *data)
{
	os_printf("%s", data+1); // ignore LF on serial line
	if (cnt_connected > 0) {
		sint8 ret = espconn_send(&m_esp_conn, (uint8 *) data, os_strlen(data));
		if (ret) {
			DEBUG("%s: espconn_send() failed. (%d)" CRLF, __FUNCTION__, ret);
		}
	}
}

/**
 ******************************************************************
 * @brief  Check if a telnet session is active.
 * @author Holger Mueller
 * @date   2015-10-06
 *
 * @return True, if there is a active connection.
 ******************************************************************
 */
bool ICACHE_FLASH_ATTR
TelnetdActive(void)
{
	if (cnt_connected == 0)
		return false;
	else
		return true;
}

/**
 ******************************************************************
 * @brief  Initialize TCP telnet server (telnetd)
 * @author Holger Mueller
 * @date   2015-09-26, 2015-10-06
 *
 * @param  port - telnetd port
 ******************************************************************
 */
void ICACHE_FLASH_ATTR
Telnetd_Init(int port)
{
	sint8 ret;

	m_esp_conn.type = ESPCONN_TCP;
	m_esp_conn.state = ESPCONN_NONE;
	m_esp_conn.proto.tcp = &m_esp_tcp;
	m_esp_conn.proto.tcp->local_port = port;
	espconn_regist_connectcb(&m_esp_conn, ConnectCb_Listen);

#ifdef SERVER_SSL_ENABLE
	ret = espconn_secure_accept(&m_esp_conn);
#else
	ret = espconn_accept(&m_esp_conn);
#endif
	if (ret == 0) {
		espconn_regist_time(&m_esp_conn, TELNETD_TIMEOUT, 0); // set timeout
		INFO("Setup telnetd at port %d." CRLF,
				m_esp_conn.proto.udp->local_port);
	} else {
		ERROR("Error %d creating telnetd at port %d." CRLF,
				ret, m_esp_conn.proto.udp->local_port);
	}
}
