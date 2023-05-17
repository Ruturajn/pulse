/*
 * SmokeSensThread.c
 *
 * Created: 03-05-2023 03:37:20 PM
 */ 

#include "SmokeSensThread/SmokeSensThread.h"

void vSmokeSensTask(void *pvParameters)
{
	while(1)
	{
		get_smoke_data();	
		//vTaskDelay(100);
	}
}

void get_smoke_data()
{
	// Display testing
		
	//! the page address to write to
	uint8_t page_address;
	//! the column address, or the X pixel.
	uint8_t column_address;
	//! store the LCD controller start draw line
	uint8_t start_line_address = 0;
		
	char complete_string[50];
		
	uint16_t result = 0;
		
	bool smoke_res = false;
		
	adc_start_conversion(&adc_instance);
	do {
		/* Wait for conversion to be done and read out result */
	} while (adc_read(&adc_instance, &result) == STATUS_BUSY);
			
	smoke_res = (result >= 170) ? true : false;
	WifiAddSmokeDataToQueue(&smoke_res);
			
	ssd1306_clear_screen();
			
	// set addresses at beginning of display
	ssd1306_set_page_address(0);

	column_address = 0;
	page_address = 0;
	ssd1306_set_page_address(0);
	ssd1306_set_column_address(96);

	snprintf(complete_string, 50, "SMOKE VAL : %d\r\n", result);
	//SerialConsoleWriteString(complete_string);
	ssd1306_write_string(complete_string);
}