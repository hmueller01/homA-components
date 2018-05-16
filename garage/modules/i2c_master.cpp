/**
 * @file  i2c_master.cpp
 * @brief I2C TWI class for ESP8266.
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

// C++ wrapper
extern "C" {
// put C includes inside here to avoid undefined references by linker.
#include <ets_sys.h>
#include <osapi.h>
#include <gpio.h>

#include "i2c_master.h"
#include "wiringESP.h"
}

#ifndef ERROR
#define ERROR(format, ...) os_printf(format, ## __VA_ARGS__)
#endif


/**
 * @brief  Private function to set I2C SDA and SCL (data/clock) bits
 *         and wait the half clock cycle.
 * @author Holger Mueller
 * @date   2018-05-15
 *
 * @param  sda - SDA level (LOW/HIGH).
 * @param  scl - SCL level (LOW/HIGH).
 */
void ICACHE_FLASH_ATTR
i2c_master::clock(uint8_t sda, uint8_t scl)
{
	this->m_last_sda = sda;
	this->m_last_scl = scl;

	digitalWrite(this->m_pin_sda, sda);
	digitalWrite(this->m_pin_scl, scl);
	delayMicroseconds(this->m_clock);
}

/**
 * @brief  Set the I2C clock speed.
 * @author Holger Mueller
 * @date   2018-05-15
 *
 * @param  clock - Clock time of half cycle [µs].
 *                 5 = 100kHz (default)
 *                 1 = 500kHz
 *                 2 = 250kHz
 */
void ICACHE_FLASH_ATTR
i2c_master::setClock(uint8_t clock)
{
	this->m_clock = clock;
}

/**
 * @brief  Initilize I2C bus.
 * @author Holger Mueller
 * @date   2018-05-15
 */
 /* XXX
void ICACHE_FLASH_ATTR
i2c_master::init(void)
{
	uint8_t i;

	this->clock(1, 0);

	// when SCL = 0, toggle SDA to clear up
	this->clock(0, 0);
	this->clock(1, 0);
	// set data_cnt to max value
	for (i = 0; i < 28; i++) {
		this->clock(1, 0);
		this->clock(1, 1);
	}

	// reset all
	this->stop();
	return;
}*/

/**
 * @brief  Send I2C start condition.
 * @author Holger Mueller
 * @date   2018-05-15
 */
void ICACHE_FLASH_ATTR
i2c_master::start(void)
{
	// SDA 1 -> 0 while SCL = 1
	this->clock(1, this->m_last_scl);
	this->clock(1, 1);
	this->clock(0, 1);
}

/**
 * @brief  Send I2C stop condition.
 * @author Holger Mueller
 * @date   2018-05-15
 */
void ICACHE_FLASH_ATTR
i2c_master::stop(void)
{
	// SDA 0 -> 1 while SCL = 1
	this->clock(0, this->m_last_scl);
	this->clock(0, 1);
	this->clock(1, 1);
}

/**
 * @brief  Send I2C acknowledge (ACK/NACK) sequence.
 * @author Holger Mueller
 * @date   2018-05-15
 *
 * @param  level - ACK level (LOW = ACK, HIGH = NACK).
 */
void ICACHE_FLASH_ATTR
i2c_master::sendAck(uint8_t level)
{
	this->clock(this->m_last_sda, 0);
	this->clock(level, 0);
	this->clock(level, 1);
	//delayMicroseconds(3);	// XXX why this extra delay?
	this->clock(level, 0);
	this->clock(1, 0);
}

/**
 * @brief  Send I2C ACK sequence.
 * @author Holger Mueller
 * @date   2018-05-15
 */
void ICACHE_FLASH_ATTR
i2c_master::sendAck(void)
{
	this->sendAck(LOW);
}

/**
 * @brief  Send I2C NACK sequence.
 * @author Holger Mueller
 * @date   2018-05-15
 */
void ICACHE_FLASH_ATTR
i2c_master::sendNack(void)
{
	this->sendAck(HIGH);
}

/**
 * @brief  Read I2C acknowledge (ACK/NACK).
 * @author Holger Mueller
 * @date   2018-05-15
 *
 * @return ACK level (LOW = ACK, HIGH = NACK).
 */
uint8_t ICACHE_FLASH_ATTR
i2c_master::readAck(void)
{
	uint8_t ret;

	this->clock(this->m_last_sda, 0);
	this->clock(1, 0);
	this->clock(1, 1);
	ret = digitalRead(this->m_pin_sda);
	this->clock(1, 0);

	return ret;
}

/**
 * @brief  Read byte from I2C bus.
 * @author Holger Mueller
 * @date   2018-05-15
 *
 * @return Read value (byte).
 */
uint8_t ICACHE_FLASH_ATTR
i2c_master::readByte(void)
{
	uint8_t ret = 0;
	uint8_t sda;
	uint8_t i;

	this->clock(m_last_sda, 0);
	for (i = 0; i < 8; i++) {
		this->clock(1, 0);
		this->clock(1, 1);
		sda = digitalRead(this->m_pin_sda);
		ret = (ret << 1) | sda;
	}
	this->clock(1, 0);

	return ret;
}

/**
 * @brief  Send byte to I2C bus.
 * @author Holger Mueller
 * @date   2018-05-15
 *
 * @param  data - Value (byte) to send.
 */
void ICACHE_FLASH_ATTR
i2c_master::sendByte(uint8_t data)
{
	uint8_t sda;
	sint8_t i;

	this->clock(this->m_last_sda, 0);
	for (i = 7; i >= 0; i--) {
		sda = (data >> i) & 0x01;
		this->clock(sda, 0);
		this->clock(sda, 1);
		this->clock(sda, 0);
	}
}

/**
 * @brief  i2c_master class constructor.
 *         Config SDA and SCL GPIO pin to open-drain output mode.
 *         Send init sequence.
 * @author Holger Mueller
 * @date   2018-05-15
 *
 * @param  pin_sda - wiringESP SDA pin.
 * @param  pin_scl - wiringESP SCL pin.
 */
ICACHE_FLASH_ATTR
i2c_master::i2c_master(uint8_t pin_sda, uint8_t pin_scl) :
m_pin_sda(pin_sda), m_pin_scl(pin_scl)
{
	ERROR("i2c_master(%d, %d) constructor called\n",
		this->m_pin_sda, this->m_pin_scl);

	pinMode(this->m_pin_sda, OPENDRAIN);
	pinMode(this->m_pin_scl, OPENDRAIN);
	this->clock(HIGH, HIGH);
	// XXX this->init();
}

ICACHE_FLASH_ATTR
i2c_master::i2c_master() :
m_pin_sda(0), m_pin_scl(0)
{
	ERROR("i2c_master() constructor w/o parameter called\n");
}

