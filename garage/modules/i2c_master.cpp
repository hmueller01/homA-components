/**
 * @file  i2c_master.cpp
 * @brief I2C TWI (Two Wire Interface) class for ESP8266.
 *
 * MIT License
 * Copyright (c) 2018 Holger Müller
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
I2c_master::clock(uint8_t sda, uint8_t scl)
{
	this->m_last_sda = sda;
	this->m_last_scl = scl;

	digitalWrite(this->m_pin_sda, sda);
	digitalWrite(this->m_pin_scl, scl);
	delayMicroseconds(this->m_clock);
}

/**
 * @brief  Send I2C start condition.
 * @author Holger Mueller
 * @date   2018-05-15
 */
void ICACHE_FLASH_ATTR
I2c_master::start(void)
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
I2c_master::stop(void)
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
I2c_master::sendAck(uint8_t level)
{
	this->clock(this->m_last_sda, 0);
	this->clock(level, 0);
	this->clock(level, 1);
	this->clock(level, 0);
	this->clock(1, 0);
}

/**
 * @brief  Send I2C ACK sequence.
 * @author Holger Mueller
 * @date   2018-05-15
 */
void ICACHE_FLASH_ATTR
I2c_master::sendAck(void)
{
	this->sendAck(LOW);
}

/**
 * @brief  Send I2C NACK sequence.
 * @author Holger Mueller
 * @date   2018-05-15
 */
void ICACHE_FLASH_ATTR
I2c_master::sendNack(void)
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
I2c_master::readAck(void)
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
I2c_master::readByte(void)
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
I2c_master::sendByte(uint8_t data)
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
 * @brief  Initiate the I2c_master library.
 *         This shall be called before any other function.
 *         Configs SDA and SCL GPIO pin to open-drain output mode,
 *         clock speed and sends init sequence.
 * @author Holger Mueller
 * @date   2018-05-22
 *
 * @param  pin_sda - wiringESP SDA pin.
 * @param  pin_scl - wiringESP SCL pin.
 * @param  clock - Clock time of half cycle [µs] (optional).
 *                 5 = 100kHz (default)
 *                 1 = 500kHz
 *                 2 = 250kHz
 */
void ICACHE_FLASH_ATTR
I2c_master::begin(uint8_t pin_sda, uint8_t pin_scl, uint8_t clock)
{
	this->m_pin_sda = pin_sda;
	this->m_pin_scl = pin_scl;
	this->m_clock = clock;
	pinMode(this->m_pin_sda, OPENDRAIN);
	pinMode(this->m_pin_scl, OPENDRAIN);
	this->clock(HIGH, HIGH);
}

ICACHE_FLASH_ATTR
I2c_master::I2c_master()
{
}
