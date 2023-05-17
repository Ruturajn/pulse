/*
 * SmokeSensThread.h
 *
 * Created: 03-05-2023 03:37:36 PM
 */ 


#ifndef SMOKESENSTHREAD_H_
#define SMOKESENSTHREAD_H_

/******************************************************************************
 * Includes
 ******************************************************************************/
#include "WifiHandlerThread/WifiHandler.h"
/******************************************************************************
 * Defines
 ******************************************************************************/
#define SMOKE_SENS_TASK_SIZE 200  //<Size of stack to assign to the UI thread. In words
#define SMOKE_SENS_TASK_PRIORITY (configMAX_PRIORITIES - 1)
extern struct adc_module adc_instance;

/******************************************************************************
 * Global Function Declaration
 ******************************************************************************/
void vSmokeSensTask(void *pvParameters);

void get_smoke_data();

#endif /* SMOKESENSTHREAD_H_ */