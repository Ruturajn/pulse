/**
 * \file
 *
 * \brief SSD1306 OLED display controller driver.
 *
 * Copyright (c) 2012-2018 Microchip Technology Inc. and its subsidiaries.
 *
 * \asf_license_start
 *
 * \page License
 *
 * Subject to your compliance with these terms, you may use Microchip
 * software and any derivatives exclusively with Microchip products.
 * It is your responsibility to comply with third party license terms applicable
 * to your use of third party software (including open source software) that
 * may accompany Microchip software.
 *
 * THIS SOFTWARE IS SUPPLIED BY MICROCHIP "AS IS". NO WARRANTIES,
 * WHETHER EXPRESS, IMPLIED OR STATUTORY, APPLY TO THIS SOFTWARE,
 * INCLUDING ANY IMPLIED WARRANTIES OF NON-INFRINGEMENT, MERCHANTABILITY,
 * AND FITNESS FOR A PARTICULAR PURPOSE. IN NO EVENT WILL MICROCHIP BE
 * LIABLE FOR ANY INDIRECT, SPECIAL, PUNITIVE, INCIDENTAL OR CONSEQUENTIAL
 * LOSS, DAMAGE, COST OR EXPENSE OF ANY KIND WHATSOEVER RELATED TO THE
 * SOFTWARE, HOWEVER CAUSED, EVEN IF MICROCHIP HAS BEEN ADVISED OF THE
 * POSSIBILITY OR THE DAMAGES ARE FORESEEABLE.  TO THE FULLEST EXTENT
 * ALLOWED BY LAW, MICROCHIP'S TOTAL LIABILITY ON ALL CLAIMS IN ANY WAY
 * RELATED TO THIS SOFTWARE WILL NOT EXCEED THE AMOUNT OF FEES, IF ANY,
 * THAT YOU HAVE PAID DIRECTLY TO MICROCHIP FOR THIS SOFTWARE.
 *
 * \asf_license_stop
 *
 */
/*
 * Support and FAQ: visit <a href="https://www.microchip.com/support/">Microchip Support</a>
 */
#include "ssd1306.h"
#include "I2cDriver/I2cDriver.h"
#include "SerialConsole/SerialConsole.h"
#include "Font/font.h"

struct spi_module ssd1306_master;
struct spi_slave_inst ssd1306_slave;


I2C_Data i2cOled; ///<Global variable to use for I2C communications with the Seesaw Device
/**
 * \internal
 * \brief Initialize the hardware interface
 *
 * Depending on what interface used for interfacing the OLED controller this
 * function will initialize the necessary hardware.
 */
static void ssd1306_interface_init(void)
{
	struct spi_config config_spi_master;
	struct spi_slave_inst_config slave_dev_config;
		
	/* Configure and initialize software device instance of peripheral slave */
	spi_slave_inst_get_config_defaults(&slave_dev_config);
	slave_dev_config.ss_pin = SSD1306_CS_PIN;
	spi_attach_slave(&ssd1306_slave, &slave_dev_config);
		
	/* Configure, initialize and enable SERCOM SPI module */
	spi_get_config_defaults(&config_spi_master);
	config_spi_master.mux_setting = SSD1306_SPI_PINMUX_SETTING;
	config_spi_master.pinmux_pad0 = SSD1306_SPI_PINMUX_PAD0; //MISO
	config_spi_master.pinmux_pad1 = SSD1306_SPI_PINMUX_PAD1; //SCK
	config_spi_master.pinmux_pad2 = SSD1306_SPI_PINMUX_PAD2;
	config_spi_master.pinmux_pad3 = SSD1306_SPI_PINMUX_PAD3; //MOSI
	config_spi_master.mode_specific.master.baudrate = SSD1306_CLOCK_SPEED;
	
	spi_init(&ssd1306_master, SSD1306_SPI, &config_spi_master);
	spi_enable(&ssd1306_master);
	
	struct port_config config_port_pin;
	port_get_config_defaults(&config_port_pin);
	config_port_pin.direction  = PORT_PIN_DIR_OUTPUT;
	config_port_pin.input_pull = PORT_PIN_PULL_UP;
	port_pin_set_config(SSD1306_DC_PIN, &config_port_pin);
	port_pin_set_config(SSD1306_RES_PIN, &config_port_pin);
}

/**
 * \brief Initialize the OLED controller
 *
 * Call this function to initialize the hardware interface and the OLED
 * controller. When initialization is done the display is turned on and ready
 * to receive data.
 */
void ssd1306_init(void)
{
	// Initialize delay routine
	delay_init();

	// Initialize the interface
	ssd1306_interface_init();

	// Do a hard reset of the OLED display controller
	ssd1306_hard_reset();

	// Set the reset pin to the default state
	port_pin_set_output_level(SSD1306_RES_PIN, true);

	// 1/32 Duty (0x0F~0x3F)
	ssd1306_write_command(SSD1306_CMD_SET_MULTIPLEX_RATIO);
	ssd1306_write_command(0x1F);

	// Shift Mapping RAM Counter (0x00~0x3F)
	ssd1306_write_command(SSD1306_CMD_SET_DISPLAY_OFFSET);
	ssd1306_write_command(0x00);

	// Set Mapping RAM Display Start Line (0x00~0x3F)
	ssd1306_write_command(SSD1306_CMD_SET_DISPLAY_START_LINE(0x00));

	// Set Column Address 0 Mapped to SEG0
	ssd1306_write_command(SSD1306_CMD_SET_SEGMENT_RE_MAP_COL127_SEG0);

	// Set COM/Row Scan Scan from COM63 to 0
	ssd1306_write_command(SSD1306_CMD_SET_COM_OUTPUT_SCAN_DOWN);

	// Set COM Pins hardware configuration
	ssd1306_write_command(SSD1306_CMD_SET_COM_PINS);
	ssd1306_write_command(0x02);

	ssd1306_set_contrast(0x8F);

	// Disable Entire display On
	ssd1306_write_command(SSD1306_CMD_ENTIRE_DISPLAY_AND_GDDRAM_ON);

	ssd1306_display_invert_disable();

	// Set Display Clock Divide Ratio / Oscillator Frequency (Default => 0x80)
	ssd1306_write_command(SSD1306_CMD_SET_DISPLAY_CLOCK_DIVIDE_RATIO);
	ssd1306_write_command(0x80);

	// Enable charge pump regulator
	ssd1306_write_command(SSD1306_CMD_SET_CHARGE_PUMP_SETTING);
	ssd1306_write_command(0x14);

	// Set VCOMH Deselect Level
	ssd1306_write_command(SSD1306_CMD_SET_VCOMH_DESELECT_LEVEL);
	ssd1306_write_command(0x40); // Default => 0x20 (0.77*VCC)

	// Set Pre-Charge as 15 Clocks & Discharge as 1 Clock
	ssd1306_write_command(SSD1306_CMD_SET_PRE_CHARGE_PERIOD);
	ssd1306_write_command(0xF1);

	ssd1306_display_on();
}

/**
 * \brief Writes a command to the display controller
 *
 * This functions pull pin D/C# low before writing to the controller. Different
 * data write function is called based on the selected interface.
 *
 * \param command the command to write
 */


void ssd1306_write_command(uint8_t command)
{
	spi_select_slave(&ssd1306_master, &ssd1306_slave, true);
	port_pin_set_output_level(SSD1306_DC_PIN, false);
	enum status_code ret_val = spi_write_buffer_wait(&ssd1306_master, &command, 1);
	if (ret_val != STATUS_OK)
		SerialConsoleWriteString("CMD FAILED!!\r\n");
	spi_select_slave(&ssd1306_master, &ssd1306_slave, false);
}

/**
 * \brief Write data to the display controller
 *
 * This functions sets the pin D/C# before writing to the controller. Different
 * data write function is called based on the selected interface.
 *
 * \param data the data to write
 */
void ssd1306_write_data(uint8_t data)
{
	spi_select_slave(&ssd1306_master, &ssd1306_slave, true);
	port_pin_set_output_level(SSD1306_DC_PIN, true);
	enum status_code ret_val = spi_write_buffer_wait(&ssd1306_master, &data, 1);
	if (ret_val != STATUS_OK)
		SerialConsoleWriteString("DATA FAILED!!\r\n");
	spi_select_slave(&ssd1306_master, &ssd1306_slave, false);
}

/**
 * \brief Display Character on the display
 *
 * This function will display the char on the display, passed as
 * an argument.
 *
 * \param data character to display
 */
int8_t ssd1306_draw_char(char data)
{
	// Greater than equal to 48 and less than 57 -> Number
	// Greater than equal to 65 and less than 90 -> Letter
	// if 61 -> =
	// if 33 -> !
	uint8_t check_input = (int)data;
	uint8_t final_indx = 0;
	if (data == 33)
		final_indx = EXCLAIM_INDEX;
	else if (data == 61)
		final_indx = EQUAL_INDEX;
	else if (data == 32)
		final_indx = SPACE_INDEX;
	else if (data == 58)
		final_indx = COLON_INDEX;
	else if (data >= 65 && data <= 90)
		final_indx = A_START_INDEX + (data - 'A');
	else if (data >= 48 && data <= 57)
		final_indx = ZERO_START_INDEX + (data - '0');
	else
		return -1;
	
	for (uint8_t i = 0; i < 6; i++)
		ssd1306_write_data(ssd1306oled_font[final_indx][i]);
		
	return 0;
}

void ssd1306_write_string(char *input)
{
	uint8_t idx = 0;
	while (input[idx] != '\0')
	{
		ssd1306_draw_char(input[idx]);
		idx++;
	}
}

void ssd1306_clear_screen()
{
	uint8_t page_address = 0;
	uint8_t column_address = 0;
	
	for (page_address = 0; page_address < 4; page_address++)
	{
		ssd1306_set_page_address(page_address);
		for (column_address = 0; column_address < 128; column_address++)
		{
			ssd1306_set_column_address(column_address);
			ssd1306_write_data(0x00);	
		}
			
	}
}