/*
 * INA219Sensor.c
 *
 * Created: 27-04-2023 06:26:25 PM
 */ 

#include "CurrSensor/INA219Sensor.h"
#include "I2cDriver/I2cDriver.h"

uint16_t ina219_read_data(uint8_t device_addr, uint8_t read_reg, bool *err_stat)
{
	struct I2C_Data i2c_sensor_data;
/*	uint8_t addr = device_addr; //0x40*/
	
	/* Data sent from the MCU to sensor. */
	uint8_t dataOut[1] = {read_reg};
	
	/* Data sent from the sensor, received on the MCU. */
	uint8_t dataIn[2] = {0, 0};
	
	uint16_t result = 0;

	i2c_sensor_data.address = device_addr;
	i2c_sensor_data.msgIn = (uint8_t*) &dataIn[0];
	i2c_sensor_data.lenOut = 1;
	i2c_sensor_data.msgOut = (const uint8_t*) &dataOut[0];
	i2c_sensor_data.lenIn = 2;
	int32_t com_err = I2cReadDataWait(&i2c_sensor_data, (TickType_t)50, (TickType_t)100);
	if (com_err != ERROR_NONE)
		err_stat = false;
	else
		err_stat = true;
	result = (i2c_sensor_data.msgIn[0] << 8 | i2c_sensor_data.msgIn[1]);
	return result;
}

void float_processing(uint16_t data_store[2], float val)
{
	uint16_t first_part, second_part;
	first_part = (uint16_t)val;
	second_part = (uint16_t)((val - first_part)*1000);
	data_store[0] = first_part;
	data_store[1] = second_part;
}

void init_ina219(uint8_t device_addr, bool *err_stat)
{
	/* Initialize the struct for the IMU. */
	struct I2C_Data i2c_curr_sensor;
	
	/* Define the address for the INA219. */
	uint8_t addr = device_addr;
	uint8_t config_reg_addr = INA219_CONFIG_REG; //0x00
	uint8_t msb_data = (uint8_t)((uint16_t)INA219_INIT_CONFIG >> 8);
	uint8_t lsb_data = (uint8_t)(((uint16_t)INA219_INIT_CONFIG) & 0xFF);
	
	/* Data sent from the MCU to sensor. */
	uint8_t dataOut[3] = {config_reg_addr, msb_data, lsb_data};
	
	/* Data sent from the sensor, received on the MCU. */
	uint8_t dataIn[2] = {0, 0};

	i2c_curr_sensor.address = addr;
	i2c_curr_sensor.msgIn = (uint8_t*) &dataIn[0];
	i2c_curr_sensor.lenOut = 3;
	i2c_curr_sensor.msgOut = (const uint8_t*) &dataOut[0];
	i2c_curr_sensor.lenIn = 2;
	
	int32_t com_err = I2cWriteDataWait(&i2c_curr_sensor, 100);
	if (com_err != ERROR_NONE)
		err_stat = false;
	else
		err_stat = true;
}