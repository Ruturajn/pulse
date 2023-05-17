/*
 * INA219TwoThread.h
 *
 * Created: 03-05-2023 03:37:04 PM
 */ 


#ifndef INA219TWOTHREAD_H_
#define INA219TWOTHREAD_H_

/******************************************************************************
 * Includes
 ******************************************************************************/
#include "WifiHandlerThread/WifiHandler.h"
#include "I2cDriver/I2cDriver.h"
#include "CurrSensor/INA219Sensor.h"
/******************************************************************************
 * Defines
 ******************************************************************************/
#define INA219_TWO_TASK_SIZE 200  //<Size of stack to assign to the UI thread. In words
#define INA219_TWO_TASK_PRIORITY (configMAX_PRIORITIES - 1)
#define INA219_TWO_VOLTAGE_LIMIT 13
#define EFUSE_2_PIN PIN_PA11

extern struct adc_module adc_instance;
/******************************************************************************
 * Global Function Declaration
 ******************************************************************************/
void vINA219TwoTask(void *pvParameters);

#endif /* INA219TWOTHREAD_H_ */