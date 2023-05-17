/*
 * INA219Sensor.h
 *
 * Created: 27-04-2023 06:26:49 PM
 */ 


#ifndef INA219SENSOR_H_
#define INA219SENSOR_H_

#include "I2cDriver/I2cDriver.h"

#define INA219_ADDR_1              (uint8_t)0x40
#define INA219_ADDR_2	           (uint8_t)0x44
#define INA219_INIT_CONFIG         (uint16_t)0x399F
#define INA219_REG_SHUNT_VOLTAGE   (uint8_t)0x01
#define INA219_REG_BUS_VOLTAGE     (uint8_t)0x02
#define INA219_CONFIG_REG          (uint8_t)0x00

uint16_t ina219_read_data(uint8_t device_addr, uint8_t read_reg, bool *err_stat);
void float_processing(uint16_t data_store[2], float val);
void init_ina219(uint8_t device_addr, bool *err_stat);

#endif /* INA219SENSOR_H_ */