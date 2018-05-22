/**
 * @file  i2c_master.h
 * @brief I2C TWI class header for ESP8266.
 *
 * Copyright (c) 2018 Holger Müller
 ***********************************************************************
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, see <http://www.gnu.org/licenses/>.
 ***********************************************************************
 */

#ifndef __I2C_MASTER_H__
#define __I2C_MASTER_H__

#include <c_types.h>

class I2c_master {
  public:
	I2c_master(); // constructor

	void begin(uint8_t pin_sda, uint8_t pin_scl, uint8_t clock = 5);
	void start(void);
	void stop(void);
	void sendAck(uint8_t level);
	void sendAck(void);
	void sendNack(void);
	uint8_t readAck(void);
	uint8_t readByte(void);
	void sendByte(uint8_t data);

  private:
	uint8_t m_pin_sda = 0;
	uint8_t m_pin_scl = 0;
	uint8_t m_last_sda = 0;
	uint8_t m_last_scl = 0;
	uint8_t m_clock = 5; // 5µs half cycle = 100kHz clock

	void clock(uint8_t sda, uint8_t scl);
};

#endif // __I2C_MASTER_H__
