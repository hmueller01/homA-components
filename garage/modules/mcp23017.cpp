/**
 * @file  mcp23017.cpp
 * @brief Partial implementation of the Arduino Wiring API for the MCP23017.
 *
 * Copyright (c) 2017 Holger Müller
 * Based on Arduino Wiring API and Gordon Henderson wiringPi.
 ***********************************************************************
 * This file is part of mcp23017:
 * https://github.com/hmueller01/mcp23017
 *
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

#include "wiringESP.h"
#include "mcp23017.h"
#include "i2c_master.h"
}

#ifndef ERROR
#define ERROR(format, ...) os_printf(format, ## __VA_ARGS__)
#endif

// Get bit representation of given pin.
#define PIN_TO_BIT(pin) (1 << (pin))
// Check if given pin is supported.
#define CHECK_PIN(pin) (((pin) >= MCP23017_MIN_PIN) && ((pin) <= MCP23017_MAX_PIN))

// addresses of the MCP23017 control register
enum mcp23017_addr {
	IODIRA = 0, // GPIO direction configuration
	IODIRB,
	IPOLA, // input polarity configuration
	IPOLB,
	GPINTENA, // interrupt on change enable
	GPINTENB,
	DEFVALA, // interrupt default compare register
	DEFVALB,
	INTCONA, // interrupt on change control (defval or last)
	INTCONB,
	IOCONA, // I/O expander configuration
	IOCONB, // = IOCONA
	GPPUA, // GPIO pull-up resistor enable
	GPPUB,
	INTFA, // interrupt flag
	INTFB,
	INTCAPA, // interrupt captured value
	INTCAPB,
	GPIOA, // GPIO port
	GPIOB,
	OLATA, // output latch
	OLATB,
};


/**
 * @brief  Sets the mode of a GPIO pin to be input or output.
 * @author Holger Mueller
 * @date   2018-04-24, 2018-05-03
 *
 * @param  pin - Pin number to set the mode.
 * @param  mode - INPUT, INPUT_PULLUP or OUTPUT (see pin_mode),
 *                more is not yet supported.
 * @return true: successful, false: error
 */
bool ICACHE_FLASH_ATTR
Mcp23017::pinMode(uint8_t pin, uint8_t mode)
{
	uint8_t gppua, gppub;
	uint8_t iodira, iodirb;
	uint16_t pin_bit = PIN_TO_BIT(pin);

	if (!CHECK_PIN(pin)) {
		return false;
	}

	switch (mode) {
	case MCP23017_INPUT:
		iodira = m_regs.reg.iodira | pin_bit;
		iodirb = m_regs.reg.iodirb | pin_bit >> 8;
		gppua = m_regs.reg.gppua & ~pin_bit;
		gppub = m_regs.reg.gppub & ~pin_bit >> 8;
		break;
	case MCP23017_INPUT_PULLUP:
		iodira = m_regs.reg.iodira | pin_bit;
		iodirb = m_regs.reg.iodirb | pin_bit >> 8;
		gppua = m_regs.reg.gppua | pin_bit;
		gppub = m_regs.reg.gppub | pin_bit >> 8;
		break;
	case MCP23017_OUTPUT:
		iodira = m_regs.reg.iodira & ~pin_bit;
		iodirb = m_regs.reg.iodirb & ~pin_bit >> 8;
		gppua = m_regs.reg.gppua;
		gppub = m_regs.reg.gppub;
		break;
	default:
		ERROR("%s: Error. Unknown mode.\n", __FUNCTION__);
		return false;
	}

	// write IODIR register to MCP23017
	m_i2c.start();
	m_i2c.sendByte(MCP23017_OP_W);
	if (m_i2c.readAck()) {
		ERROR("%s: op write not ack\n", __FUNCTION__);
		m_i2c.stop();
		return false;
	}
	m_i2c.sendByte(IODIRA);
	if (m_i2c.readAck()) {
		ERROR("%s: addr not ack\n", __FUNCTION__);
		m_i2c.stop();
		return false;
	}
	m_i2c.sendByte(iodira);
	if (m_i2c.readAck()) {
		ERROR("%s: iodira not ack\n", __FUNCTION__);
		m_i2c.stop();
		return false;
	}
	m_regs.reg.iodira = iodira;
	m_i2c.sendByte(iodirb);
	if (m_i2c.readAck()) {
		ERROR("%s: iodirb not ack\n", __FUNCTION__);
		m_i2c.stop();
		return false;
	}
	m_regs.reg.iodirb = iodirb;
	m_i2c.stop();

	// write GPPU register to MCP23017
	m_i2c.start();
	m_i2c.sendByte(MCP23017_OP_W);
	if (m_i2c.readAck()) {
		ERROR("%s: op write not ack\n", __FUNCTION__);
		m_i2c.stop();
		return false;
	}
	m_i2c.sendByte(GPPUA);
	if (m_i2c.readAck()) {
		ERROR("%s: addr not ack\n", __FUNCTION__);
		m_i2c.stop();
		return false;
	}
	m_i2c.sendByte(gppua);
	if (m_i2c.readAck()) {
		ERROR("%s: gppua not ack\n", __FUNCTION__);
		m_i2c.stop();
		return false;
	}
	m_regs.reg.gppua = gppua;
	m_i2c.sendByte(gppub);
	if (m_i2c.readAck()) {
		ERROR("%s: gppub not ack\n", __FUNCTION__);
		m_i2c.stop();
		return false;
	}
	m_regs.reg.gppub = gppub;
	m_i2c.stop();

	return true;
}


/**
 * @brief  Control the internal pull-up/down resistors on a GPIO pin.
 * @author Holger Mueller
 * @date   2018-04-24, 2018-05-03
 *
 * Note: The MCP23017 only has pull-ups, so PUD_DOWN is not supported here.
 *
 * @param  pin - Pin number to set the mode.
 * @param  pud - Enable (PUD_UP) or disable (PUD_OFF) pull-up.
 * @return true: successful, false: error
 */
bool ICACHE_FLASH_ATTR
Mcp23017::pullUpDnControl(uint8_t pin, uint8_t pud)
{
	uint8_t gppua, gppub;
	uint16_t pin_bit = PIN_TO_BIT(pin);

	if (!CHECK_PIN(pin)) {
		return false;
	}

	switch (pud) {
	case MCP23017_PUD_OFF:
		gppua = m_regs.reg.gppua & ~pin_bit;
		gppub = m_regs.reg.gppub & ~pin_bit >> 8;
		break;
	case MCP23017_PUD_UP:
		gppua = m_regs.reg.gppua | pin_bit;
		gppub = m_regs.reg.gppub | pin_bit >> 8;
		break;
	default:
		ERROR("%s: Error. Unknown mode.\n", __FUNCTION__);
		return false;
	}

	// write GPPU register to MCP23017
	m_i2c.start();
	m_i2c.sendByte(MCP23017_OP_W);
	if (m_i2c.readAck()) {
		ERROR("%s: op write not ack\n", __FUNCTION__);
		m_i2c.stop();
		return false;
	}
	m_i2c.sendByte(GPPUA);
	if (m_i2c.readAck()) {
		ERROR("%s: addr not ack\n", __FUNCTION__);
		m_i2c.stop();
		return false;
	}
	m_i2c.sendByte(gppua);
	if (m_i2c.readAck()) {
		ERROR("%s: gppua not ack\n", __FUNCTION__);
		m_i2c.stop();
		return false;
	}
	m_regs.reg.gppua = gppua;
	m_i2c.sendByte(gppub);
	if (m_i2c.readAck()) {
		ERROR("%s: gppub not ack\n", __FUNCTION__);
		m_i2c.stop();
		return false;
	}
	m_regs.reg.gppub = gppub;
	m_i2c.stop();

	return true;
}

/**
 * @brief  Read all 16 bit of the I/O ports.
 * @author Holger Mueller
 * @date   2018-05-03
 *
 * @return Value of the I/O ports.
 */
uint16_t ICACHE_FLASH_ATTR
Mcp23017::digitalRead16(void)
{
	// write GPIOA address to MCP23017
	m_i2c.start();
	m_i2c.sendByte(MCP23017_OP_W);
	if (m_i2c.readAck()) {
		ERROR("%s: op write not ack\n", __FUNCTION__);
		m_i2c.stop();
		return false;
	}
	m_i2c.sendByte(GPIOA);
	if (m_i2c.readAck()) {
		ERROR("%s: addr not ack\n", __FUNCTION__);
		m_i2c.stop();
		return false;
	}

	// read gpio value
	m_i2c.start();
	m_i2c.sendByte(MCP23017_OP_R);
	if (m_i2c.readAck()) {
		ERROR("%s: op read not ack\n", __FUNCTION__);
		m_i2c.stop();
		return false;
	}
	m_regs.reg.gpioa = m_i2c.readByte();
	m_i2c.sendAck();
	m_regs.reg.gpiob = m_i2c.readByte();
	m_i2c.stop();

	return ((((uint16_t) m_regs.reg.gpiob) << 8) |
			m_regs.reg.gpioa);
}

/**
 * @brief  Read the value of a given pin, returning HIGH or LOW.
 * @author Holger Mueller
 * @date   2018-04-24, 2018-05-03
 *
 * @param  pin - Pin number to read from.
 * @return Status of pin (LOW or HIGH).
 */
uint8_t ICACHE_FLASH_ATTR
Mcp23017::digitalRead(uint8_t pin)
{
	if (!CHECK_PIN(pin)) {
		return false;
	}

	return (digitalRead16() >> pin) & 0x01;
}

/**
 * @brief  Write all 16 bit of the I/O ports.
 * @author Holger Mueller
 * @date   2018-05-16
 *
 * @param  value - Value to write.
 * @return true: successful, false: error
 */
bool ICACHE_FLASH_ATTR
Mcp23017::digitalWrite16(uint16_t value)
{
	// write GPIOA address to MCP23017
	m_i2c.start();
	m_i2c.sendByte(MCP23017_OP_W);
	if (m_i2c.readAck()) {
		ERROR("%s: op write not ack\n", __FUNCTION__);
		m_i2c.stop();
		return false;
	}
	m_i2c.sendByte(GPIOA);
	if (m_i2c.readAck()) {
		ERROR("%s: addr not ack\n", __FUNCTION__);
		m_i2c.stop();
		return false;
	}
	m_i2c.sendByte((uint8_t) value);
	if (m_i2c.readAck()) {
		ERROR("%s: gpioa not ack\n", __FUNCTION__);
		m_i2c.stop();
		return false;
	}
	// save in global register copy, as we have written it successfully
	m_regs.reg.gpioa = (uint8_t) value;

	m_i2c.sendByte((uint8_t) (value >> 8));
	if (m_i2c.readAck()) {
		ERROR("%s: gpiob not ack\n", __FUNCTION__);
		m_i2c.stop();
		return false;
	}
	// save in global register copy, as we have written it successfully
	m_regs.reg.gpiob = (uint8_t) value >> 8;
	m_i2c.stop();

	return true;
}

/**
 * @brief  Set an output pin (LOW or HIGH).
 * @author Holger Mueller
 * @date   2018-05-02, 2018-05-16
 *
 * @param  pin - Pin number to set.
 * @param  value - Level of pin (LOW or HIGH).
 * @return true: successful, false: error
 */
bool ICACHE_FLASH_ATTR
Mcp23017::digitalWrite(uint8_t pin, uint8_t value)
{
	uint8_t gpioa, gpiob;
	uint16_t pin_bit = PIN_TO_BIT(pin);

	if (!CHECK_PIN(pin)) {
		return false;
	}

	switch (value) {
	case LOW:
		gpioa = m_regs.reg.gpioa & ~pin_bit;
		gpiob = m_regs.reg.gpiob & ~pin_bit >> 8;
		break;
	case HIGH:
		gpioa = m_regs.reg.gpioa | pin_bit;
		gpiob = m_regs.reg.gpiob | pin_bit >> 8;
		break;
	default:
		ERROR("%s: Error. Unknown value.\n", __FUNCTION__);
		return false;
	}

	return digitalWrite16((((uint16_t) gpiob) << 8) | gpioa);
}

/**
 * @brief  Read and dump all register from MCP23017.
 * @author Holger Mueller
 * @date   2018-05-14
 *
 * @return true: successful, false: error
 */
bool ICACHE_FLASH_ATTR
Mcp23017::dumpRegs(void)
{
	uint8_t i;
	uint8_t value;

	// write first address (IODIRA) to MCP23017
	m_i2c.start();
	m_i2c.sendByte(MCP23017_OP_W);
	if (m_i2c.readAck()) {
		ERROR("%s: op write not ack\n", __FUNCTION__);
		m_i2c.stop();
		return false;
	}
	m_i2c.sendByte(IODIRA);
	if (m_i2c.readAck()) {
		ERROR("%s: addr not ack\n", __FUNCTION__);
		m_i2c.stop();
		return false;
	}

	// read values from here
	m_i2c.start();
	m_i2c.sendByte(MCP23017_OP_R);
	if (m_i2c.readAck()) {
		ERROR("%s: op read not ack\n", __FUNCTION__);
		m_i2c.stop();
		return false;
	}
	for (i = 0; i < sizeof(m_regs.raw); i++) {
		if (i) {
			m_i2c.sendAck();
		}
		value = m_i2c.readByte();
		ERROR("MCP23017 register 0x%02X=0x%02X\n", i, value);
	}
	m_i2c.stop();

	return true;
}

/**
 * @brief  Initiate the mcp23017 library.
 *         This shall be called before any other function.
 *         Configures I2C, writes default config values to MCP23017.
 * @author Holger Mueller
 * @date   2018-05-05, 2018-05-15, 2018-05-22
 *
 * @param  pin_sda - wiringESP SDA pin.
 * @param  pin_scl - wiringESP SCL pin.
 * @param  iodira - direction configuration port A (optional, default 0xFF).
 * @param  iodirb - direction configuration port B (optional, default 0xFF).
 * @param  gppua - pull-up resistor enable port A (optional, default 0x00).
 * @param  gppub - pull-up resistor enable port B (optional, default 0x00).
 * @param  ipola - input polarity configuration port A (optional, default 0x00).
 * @param  ipolb - input polarity configuration port B (optional, default 0x00).
 */
bool ICACHE_FLASH_ATTR
Mcp23017::begin(uint8_t pin_sda, uint8_t pin_scl,
		uint8_t iodira, uint8_t iodirb,
		uint8_t gppua, uint8_t gppub,
		uint8_t ipola, uint8_t ipolb)
{
	uint8_t i;
	uint8_t reg_size;

	// init MCP23017 register with default values
	reg_size = sizeof(m_regs.raw);
	os_bzero(m_regs.raw, reg_size);
	m_regs.reg.iodira = iodira;
	m_regs.reg.iodirb = iodirb;
	m_regs.reg.gppua = gppua;
	m_regs.reg.gppub = gppub;
	m_regs.reg.ipola = ipola;
	m_regs.reg.ipolb = ipolb;

	// init I2C GPIO class and read data
	m_i2c.begin(pin_sda, pin_scl);

	// write first address (IODIRA) to MCP23017
	m_i2c.start();
	m_i2c.sendByte(MCP23017_OP_W);
	if (m_i2c.readAck()) {
		ERROR("%s: op write not ack\n", __FUNCTION__);
		m_i2c.stop();
		return false;
	}
	m_i2c.sendByte(IODIRA);
	if (m_i2c.readAck()) {
		ERROR("%s: addr not ack\n", __FUNCTION__);
		m_i2c.stop();
		return false;
	}
	// write init values to MCP23017
	for (i = 0; i < reg_size; i++) {
		//ERROR("MCP23017 register %2x set to: %02x\n", i, mcp23017_reg.raw[i]);
		m_i2c.sendByte(m_regs.raw[i]);
		if (m_i2c.readAck()) {
			ERROR("%s: write byte %d not ack\n", __FUNCTION__, i);
			m_i2c.stop();
			return false;
		}
	}
	m_i2c.stop();

	return true;
}

/**
 * @brief  Constructor of class
 * @author Holger Mueller
 * @date   2018-05-14
 */
ICACHE_FLASH_ATTR
Mcp23017::Mcp23017()
{
}

