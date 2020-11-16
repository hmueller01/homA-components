/**
 * @file  i2c_master.h
 * @brief I2C TWI (Two Wire Interface) class header for ESP8266.
 *
 * MIT License
 * Copyright (c) 2018 Holger Müller
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
