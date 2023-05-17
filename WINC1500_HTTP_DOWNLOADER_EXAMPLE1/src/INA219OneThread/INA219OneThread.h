/*
 * INA219OneThread.h
 *
 * Created: 03-05-2023 03:36:23 PM
 */ 


#ifndef INA219ONETHREAD_H_
#define INA219ONETHREAD_H_

/******************************************************************************
 * Includes
 ******************************************************************************/
#include "WifiHandlerThread/WifiHandler.h"
#include "I2cDriver/I2cDriver.h"
#include "CurrSensor/INA219Sensor.h"
/******************************************************************************
 * Defines
 ******************************************************************************/
#define INA219_ONE_TASK_SIZE 400  //<Size of stack to assign to the UI thread. In words
#define INA219_ONE_TASK_PRIORITY (configMAX_PRIORITIES - 1)
#define INA219_ONE_VOLTAGE_LIMIT 13
#define EFUSE_1_PIN PIN_PB22

extern struct adc_module adc_instance;
/******************************************************************************
 * Global Function Declaration
 ******************************************************************************/
void vINA219OneTask(void *pvParameters);

#endif /* INA219ONETHREAD_H_ */