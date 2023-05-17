/*
 * INA219OneThread.c
 *
 * Created: 03-05-2023 03:36:03 PM
 */ 

#include "INA219OneThread/INA219OneThread.h"
#include "SmokeSensThread/SmokeSensThread.h"

char buffer[100];

bool efuse_stat_1 = true;
extern bool up_firmware;

void vINA219OneTask(void *pvParameters)
{
	// Init the current sensor once.
	// Call the initialization function defined in the INA219 driver
	// written by us.
	bool comm_stat = true;
	
	init_ina219(INA219_ADDR_1, &comm_stat);
	
	if (comm_stat == false)
	{
		SerialConsoleWriteString("[ * ]Could not Initialize Current Sensor 1!!\r\n");
		SerialConsoleWriteString("[ * ]FATAL ERROR !! MANUAL INTERVENTION REQUIRED!!\r\n");
	}
	
	// Define variables for current and power values.
	float _current = 0;
	float _power = 0;
		
	// Define variables for capturing raw values from the
	// slave registers.
	uint16_t read_reg_voltage = 0;
	uint16_t read_reg_bus_voltage = 0;
		
	// Store the values for shunt and bus voltage.
	float shunt_voltage = 0;
	float bus_voltage = 0;
	
	// Define arrays for breaking down the float values
	// into integers before the decimal point and after
	// the decimal point.
	uint16_t data_current[2] = {0, 0};
	uint16_t data_power[2] = {0, 0};
	uint16_t data_bus_volt[2] = {0, 0};
		
	uint16_t result = 0;
	
	bool smoke_res = false;
	
	char complete_string[50];
		
	while(1)
	{

		// Call the ina219_read_data function to read the data from the SHUNT voltage
		// register.
		read_reg_voltage = ina219_read_data(INA219_ADDR_1, INA219_REG_SHUNT_VOLTAGE, &comm_stat);
		
		if (comm_stat == false)
		{
			SerialConsoleWriteString("[ * ]Could not Communicate with Current Sensor 1!!\r\n");
			SerialConsoleWriteString("[ * ]DISABLING SUPPLY...\r\n");
			port_pin_set_output_level(EFUSE_1_PIN, false);
			SerialConsoleWriteString("[ * ]DISABLED SUPPLY...\r\n");
			continue;
		}
		
		// Call the ina219_read_data function to read the data from the BUS voltage
		// register.
		read_reg_bus_voltage = ina219_read_data(INA219_ADDR_1, INA219_REG_BUS_VOLTAGE, &comm_stat);
		
		if (comm_stat == false)
		{
			SerialConsoleWriteString("[ * ]Could not Communicate with Current Sensor 1!!\r\n");
			SerialConsoleWriteString("[ * ]DISABLING SUPPLY...\r\n");
			port_pin_set_output_level(EFUSE_1_PIN, false);
			SerialConsoleWriteString("[ * ]DISABLED SUPPLY...\r\n");
			continue;
		}

		// Check if overflow took place, and account for negative voltages.
		if (read_reg_voltage & 0x8000) {
			shunt_voltage = (int16_t)((~read_reg_voltage) + 1) * -1;
			} else {
			shunt_voltage = read_reg_voltage;
		}
		
		// Divide the shunt voltage value by 100, for calibration.
		shunt_voltage = shunt_voltage / 100;
		
		// Calibrate the bus voltage value.
		bus_voltage = (read_reg_bus_voltage >> 3) * 4;
		bus_voltage = bus_voltage / 1000;

		// Calculate the current and power, using the shunt resistor
		// which has a value of 0.1 ohms.
		_current = ((float)(shunt_voltage) / 0.1);
		_power = (float)(bus_voltage) * _current;

		// Perform some post processing for printing out the float values.
		float_processing(&data_current, _current);
		float_processing(&data_power, _power);
		float_processing(&data_bus_volt, bus_voltage);		if (bus_voltage > INA219_ONE_VOLTAGE_LIMIT)
		{
			port_pin_set_output_level(EFUSE_1_PIN, false);
			efuse_stat_1 = false;
		}
		
		// Add data to queue.
		struct INA219Sens1 ina_one;
		ina_one.current = _current;
		ina_one.power = _power;
		ina_one.voltage = bus_voltage;
		ina_one.comm_status = comm_stat;
		ina_one.efuse_1_stat = efuse_stat_1;
		WifiAddINA219OneDataToQueue(&ina_one);
		
		// Get Data from the smoke sensor.
		get_smoke_data();
		
		if (up_firmware)
		{
			update_firmware();
			up_firmware = false;
		}
		
		vTaskDelay(100);
	}
}