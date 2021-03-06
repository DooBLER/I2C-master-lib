#ifndef  F_CPU
#define F_CPU 16000000UL
#endif

#include <avr/io.h>
#include <util/twi.h>

#include "i2c_master.h"

#define F_SCL 100000UL // SCL frequency
#define Prescaler 1
#define TWBR_val ((((F_CPU / F_SCL) / Prescaler) - 16 ) / 2)

void i2c_init(void)
{
	TWBR = (uint8_t)TWBR_val;
	TWCR = 0;
}

uint8_t i2c_start(uint8_t address)
{
	// transmit START condition 
	TWCR = (1<<TWINT) | (1<<TWSTA) | (1<<TWEN);
	// wait for end of transmission
	while ( !(TWCR & (1<<TWINT)) );
	
	// check if the (re)start condition was successfully transmitted
	if ( (TW_STATUS != TW_START) && (TW_STATUS != TW_REP_START) ) return 1;
	
	// load slave address into data register
	TWDR = address;
	// start transmission of address
	TWCR = (1<<TWINT) | (1<<TWEN);
	// wait for end of transmission
	while ( !(TWCR & (1<<TWINT)) );
	
	// check if the device has acknowledged the READ / WRITE mode
	if ( (TW_STATUS != TW_MT_SLA_ACK) && (TW_STATUS != TW_MR_SLA_ACK) ) return 1;
	
	return 0;
}

uint8_t i2c_write(uint8_t data)
{
	// load data into data register
	TWDR = data;
	// start transmission of data
	TWCR = (1<<TWINT) | (1<<TWEN);
	// wait for end of transmission
	while ( !(TWCR & (1<<TWINT)) );
	
	if ( TW_STATUS != TW_MT_DATA_ACK ) return 1;
	
	return 0;
}

uint8_t i2c_read_ack(void)
{
	// start TWI module and acknowledge data after reception
	TWCR = (1<<TWINT) | (1<<TWEN) | (1<<TWEA); 
	// wait for end of transmission
	while ( !(TWCR & (1<<TWINT)) );
	// return received data from TWDR
	return TWDR;
}

uint8_t i2c_read_nack(void)
{
	// start receiving without acknowledging reception
	TWCR = (1<<TWINT) | (1<<TWEN);
	// wait for end of transmission
	while ( !(TWCR & (1<<TWINT)) );
	// return received data from TWDR
	return TWDR;
}

uint8_t i2c_transmit(uint8_t address, uint8_t* data, uint16_t length)
{
	if ( i2c_start(address | TW_WRITE) ) return 1;
	
	for ( uint16_t i = 0; i < length; i++ )
	{
		if ( i2c_write(data[i]) ) return 1;
	}
	
	i2c_stop();
	
	return 0;
}

uint8_t i2c_receive(uint8_t address, uint8_t* data, uint16_t length)
{
	if ( i2c_start(address | TW_READ) ) return 1;
	
	for ( uint16_t i = 0; i < (length-1); i++ )
	{
		data[i] = i2c_read_ack();
	}
	data[(length-1)] = i2c_read_nack();
	
	i2c_stop();
	
	return 0;
}

uint8_t i2c_writeReg(uint8_t devaddr, uint8_t regaddr, uint8_t* data,
					 uint16_t length)
{
	if ( i2c_start(devaddr | TW_WRITE) ) return 1;

	i2c_write(regaddr);

	for ( uint16_t i = 0; i < length; i++ )
	{
		if ( i2c_write(data[i]) ) return 1;
	}

	i2c_stop();

	return 0;
}

uint8_t i2c_readReg(uint8_t devaddr, uint8_t regaddr, uint8_t* data,
					uint16_t length)
{
	if ( i2c_start(devaddr) ) return 1;

	i2c_write(regaddr);

	if ( i2c_start(devaddr | TW_READ) ) return 1;

	for ( uint16_t i = 0; i < (length-1); i++ )
	{
		data[i] = i2c_read_ack();
	}
	data[(length-1)] = i2c_read_nack();

	i2c_stop();

	return 0;
}

void i2c_stop(void)
{
	// transmit STOP condition
	TWCR = (1<<TWINT) | (1<<TWEN) | (1<<TWSTO);
}
