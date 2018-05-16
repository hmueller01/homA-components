/**
 * @file  mcp23017.h
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

#ifndef __MCP23017_H__
#define __MCP23017_H__

#include <gpio.h>
#include <osapi.h>
#include <user_interface.h>

#include "i2c_master.h"

#define MCP23017_MIN_PIN 0
#define MCP23017_MAX_PIN 15
#define MCP23017_HW_ADDR 0x00 // HW address coded by pins A0, A1, A2
#define MCP23017_OP_W (0x40 | MCP23017_HW_ADDR << 1)
#define MCP23017_OP_R (MCP23017_OP_W | 0x01)

// Pin levels
#ifndef LOW
#define LOW 0
#endif
#ifndef HIGH
#define HIGH 1
#endif

// Pin modes
#define MCP23017_INPUT 0
#define MCP23017_OUTPUT 1
#define MCP23017_INPUT_PULLUP 2

// Pull up/none
#define MCP23017_PUD_OFF 0
//#define MCP23017_PUD_DOWN 1 // not possible with MCP23017
#define MCP23017_PUD_UP 2

class mcp23017 {
  public:
	mcp23017(); // constructor
	mcp23017(uint8_t pin_sda, uint8_t pin_scl, uint8_t iodira = 0xFF, uint8_t iodirb = 0xFF, uint8_t gppua = 0, uint8_t gppub = 0, uint8_t ipola = 0, uint8_t ipolb = 0);

	// Core wiringMCP23017 functions
	bool pinMode(uint8_t pin, uint8_t mode);
	bool pullUpDnControl(uint8_t pin, uint8_t pud);
	uint16_t digitalRead16(void);
	uint8_t digitalRead(uint8_t pin);
	bool digitalWrite16(uint16_t value);
	bool digitalWrite(uint8_t pin, uint8_t value);
	bool dumpRegs(void);

  private:
	i2c_master m_i2c; // I2C interface class

	// internal copy of the MCP23017 control register
	// align bytes of union to 1 byte boundary (instead of compiler default)
	#pragma pack(push)
	#pragma pack(1)
	union {
		uint8_t raw[22];
		struct {
			uint8_t iodira; // I/O direction configuration
			uint8_t iodirb;
			uint8_t ipola; // input polarity configuration
			uint8_t ipolb;
			uint8_t gpintena; // interrupt on change enable
			uint8_t gpintenb;
			uint8_t defvala; // interrupt default compare register
			uint8_t defvalb;
			uint8_t intcona; // interrupt on change control (defval or last)
			uint8_t intconb;
			uint8_t iocona; // I/O expander configuration
			uint8_t ioconb; // = iocona
			uint8_t gppua; // GPIO pull-up resistor enable
			uint8_t gppub;
			uint8_t intfa; // interrupt flag
			uint8_t intfb;
			uint8_t intcapa; // interrupt captured value
			uint8_t intcapb;
			uint8_t gpioa; // general purpose I/O port
			uint8_t gpiob;
			uint8_t olata; // output latch
			uint8_t olatb;
		} reg;
	} m_regs;
	#pragma pack(pop)
};

#endif // __MCP23017_H__
