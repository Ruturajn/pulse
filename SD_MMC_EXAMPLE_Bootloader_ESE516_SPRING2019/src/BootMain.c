/**************************************************************************//**
* @file      BootMain.c
* @brief     Main file for the ESE516 bootloader. Handles updating the main application
* @details   Main file for the ESE516 bootloader. Handles updating the main application
* @author    Eduardo Garcia
* @date      2020-02-15
* @version   2.0
* @copyright Copyright University of Pennsylvania
******************************************************************************/


/******************************************************************************
* Includes
******************************************************************************/
#include <asf.h>
#include "conf_example.h"
#include <string.h>
#include "sd_mmc_spi.h"

#include "SD Card/SdCard.h"
#include "Systick/Systick.h"
#include "SerialConsole/SerialConsole.h"
#include "ASF/sam0/drivers/dsu/crc32/crc32.h"





/******************************************************************************
* Defines
******************************************************************************/
#define APP_START_ADDRESS  ((uint32_t)0x12000) ///<Start of main application. Must be address of start of main application
#define APP_START_RESET_VEC_ADDRESS (APP_START_ADDRESS+(uint32_t)0x04) ///< Main application reset vector address
//#define MEM_EXAMPLE 1 //COMMENT ME TO REMOVE THE MEMORY WRITE EXAMPLE BELOW

/******************************************************************************
* Structures and Enumerations
******************************************************************************/

struct usart_module cdc_uart_module; ///< Structure for UART module connected to EDBG (used for unit test output)

/******************************************************************************
* Local Function Declaration
******************************************************************************/
static void jumpToApplication(void);
static bool StartFilesystemAndTest(void);
static void configure_nvm(void);
static void configure_port_pins(void);
static void reset_binary(char* readBuffer);
static bool crcCheck(char* readBuffer, int k);
static void bootloader_init(char* readBuffer);

/******************************************************************************
* Global Variables
******************************************************************************/
//INITIALIZE VARIABLES
char test_file_name[] = "0:sd_mmc_test.txt";	///<Test TEXT File name
char test_bin_file[] = "0:sd_binary.bin";	///<Test BINARY File name
char TestA_bin_file[] = "0:ESE516_MAIN_FW.bin"; // Test A Binary
char TestB_bin_file[] = "0:TestB.bin"; // Test B Binary
char testA_file_name[] = "0:FlagA.txt";	///<Test TEXT File name for TestA binary
char testB_file_name[] = "0:FlagB.txt";	///<Test TEXT File name for TestB binary
char bootloader_file_init[] = "0:bootloader.txt";///<Test TEXT File name for bootloader initialisation
Ctrl_status status; ///<Holds the status of a system initialization
FRESULT res; //Holds the result of the FATFS functions done on the SD CARD TEST
FRESULT resA; //Holds the result of the FATFS functions done on the SD CARD TEST
FRESULT resB; //Holds the result of the FATFS functions done on the SD CARD TEST
FRESULT res_BOOT;// Holds the result of the FATFS functions done on the SD CARD TEST
FATFS fs; //Holds the File System of the SD CARD
FIL file_object; //FILE OBJECT used on main for the SD Card Test
FIL boot_object;


/******************************************************************************
* Global Functions
******************************************************************************/


/**************************************************************************//**
* @fn		static void configure_port_pins(void)
* @brief	Function to configure the SW0 pin.
* @return	
* @note		The SW0 pin is initialized here.
*****************************************************************************/
static void configure_port_pins(void)
{
	struct port_config config_port_pin;
	port_get_config_defaults(&config_port_pin);
	config_port_pin.direction  = PORT_PIN_DIR_INPUT;
	config_port_pin.input_pull = PORT_PIN_PULL_UP;
	port_pin_set_config(BUTTON_0_PIN, &config_port_pin);
}

/**************************************************************************//**
* @fn			static void reset_binary(char* readBuffer)
* @brief		Write binary files from SD card into MCU memory.
* @param[in]	Pointer to the readBuffer array.
* @return		None
* @note			This function erases the current memory in the MCU
* 				and writes to it, the binary read from the SD card.
*****************************************************************************/
static void reset_binary(char* readBuffer)
{
	// Counter variables to keep track of successful reads and writes.
	int counter = 0;
	int crc = 0;
	
	// Loop through, and read 256 bytes (one row) from the SD card, and write
	// it to the MCU memory. Since, the binary files are approximately 32KB,
	// the loop bound is set according to that.
	for(int k =0 ; k<176000 ; k=k+256)
	{
		// Erase a row from the MCU memory, using the `APP_START_ADDRESS` as
		// the base, and k as the offset.
		enum status_code nvmError = nvm_erase_row(APP_START_ADDRESS + k);
		if(nvmError != STATUS_OK)
		{
			SerialConsoleWriteString("Erase error");
		}

		
		//Make sure it got erased - we read the page. Erasure in NVM is an 0xFF
		for(int iter = 0; iter < 256; iter++)
		{
			char *a = (char *)(APP_START_ADDRESS + k + iter); //Pointer pointing to address APP_START_ADDRESS
			if(*a != 0xFF)
			{
				SerialConsoleWriteString("Error - test page is not erased!");
				break;
			}
		}
		
		// Define the number of bytes that are left to
		// be read.
		int numBytesLeft = 256;
		
		// Define number of bytes read.
		uint32_t numBytesRead = 0;
		
		// Define total number of bytes.
		int numberBytesTotal = 0;
		
		// Use `f_lseek` to increment the file_object
		// by `k`. Since, we are advancing by one row
		// in every iteration.
		res = f_lseek(&file_object, k);
		
		// Loop until we have read all the bytes from the SD card.
		while(numBytesLeft  != 0)
		{
			// Call f_read to read from the file, and store into the readBuffer. The
			// numBytesRead parameter stores the number of bytes read from the file.
			res = f_read(&file_object, &readBuffer[numberBytesTotal], numBytesLeft, &numBytesRead); //Question to students: What is numBytesRead? What are we doing here?
			numBytesLeft -= numBytesLeft;
			numberBytesTotal += numBytesRead;
		}
		
		// Now, we write the data that is read from the SD card into the 
		// microcontroller's memory.
		res = nvm_write_buffer (APP_START_ADDRESS + k, &readBuffer[0], 64);
		res = nvm_write_buffer (APP_START_ADDRESS + k + 64, &readBuffer[64], 64);
		res = nvm_write_buffer (APP_START_ADDRESS + k + 128, &readBuffer[128], 64);
		res = nvm_write_buffer (APP_START_ADDRESS + k + 192, &readBuffer[192], 64);

		// Check the result from the write operation, and based on that increment
		// the counter.
		if (res != FR_OK)
			SerialConsoleWriteString("Test write to NVM failed!\r\n");
		else
			counter++;
		
		// Now, check the CRC for the whole row operation.
		if(crcCheck(readBuffer,k))
			crc++;
		
		// If the counters is equal to the number of iterations, it implies
		// that all the checks passed in every iteration and we can
		// print out that the operation was successfull.
		if(counter == 128)
			SerialConsoleWriteString("Test write to NVM succeeded!\r\n");
		if(crc == 128)
			SerialConsoleWriteString("CRC succeeded!\r\n");
	}
}


/**************************************************************************//**
* @fn			static bool crcCheck (char *readBuffer, int k)
* @brief		Performs CRC check.
* @param[in]	Pointer to the readBuffer array.
* @param[in]	Address offset.
* @return		Returns true if CRC passed else returns false.
* @note			This function computes the CRC for both the MCU memory
*				and the SD card operation. Compares them and returns
*				the result.			
*****************************************************************************/
static bool crcCheck (char *readBuffer, int k)
{
	// Initialize a help string, into which we will store
	// the formatted string to be printed.	
	char helpStr[64];
	
	// Initialize a variable to store the CRC result
	// for the SD card.
	uint32_t resultCrcSd = 0;
	*((volatile unsigned int*) 0x41007058) &= ~0x30000UL;

	// CRC of SD Card.
	// The Third parameter is used for storing the CRC result.
	enum status_code crcres = dsu_crc32_cal	(readBuffer	,256, &resultCrcSd); //Instructor note: Was it the third parameter used for? Please check how you can use the third parameter to do the CRC of a long data stream in chunks - you will need it!
	
	//Errata Part 2 - To be done after RAM CRC
	*((volatile unsigned int*) 0x41007058) |= 0x20000UL;
	
	
	//CRC of memory (NVM)
	uint32_t resultCrcNvm = 0;
	crcres |= dsu_crc32_cal	(APP_START_ADDRESS +k,256, &resultCrcNvm);
	
	// If the CRC failed, print out the error message. Else, return
	// true if it passed.
	if (crcres != STATUS_OK)
		SerialConsoleWriteString("Could not calculate CRC!!\r\n");
	else
	{
		if(resultCrcNvm == resultCrcSd)
			return true;
	}
	jumpToApplication();
	return false;
}

static void bootloader_init(char* readBuffer){
	f_unlink(testA_file_name);
	f_unlink(testB_file_name);
	
	bootloader_file_init[0] = LUN_ID_SD_MMC_0_MEM + '0';
	res = f_open(&boot_object, (char const *)bootloader_file_init, FA_CREATE_NEW);
	
	
	//Read SD Card File.
	TestA_bin_file[0] = LUN_ID_SD_MMC_0_MEM + '0';
	res = f_open(&file_object, (char const *)TestA_bin_file, FA_READ);
	
	// If there was a problem print out the error message.
	if (res != FR_OK)
	{
		SerialConsoleWriteString("Could not open test file!\r\n");
	}
	
	
	// Call the `reset_binary` function, which will load TestA.bin into
	// memory by writing it into the MCU memory.
	reset_binary(readBuffer);
}
/**************************************************************************//**
* @fn		int main(void)
* @brief	Main function for ESE516 Bootloader Application

* @return	Unused (ANSI-C compatibility).
* @note		Bootloader code initiates here.
*****************************************************************************/

int main(void)
{

	/*1.) INIT SYSTEM PERIPHERALS INITIALIZATION*/
	system_init();
	delay_init();
	InitializeSerialConsole();
	system_interrupt_enable_global();
	configure_port_pins(); // Initialize SW0 Pin.
	/* Initialize SD MMC stack */
	sd_mmc_init();

	//Initialize the NVM driver
	configure_nvm();

	irq_initialize_vectors();
	cpu_irq_enable();

	//Configure CRC32
	dsu_crc32_init();

	SerialConsoleWriteString("ESE516 - ENTER BOOTLOADER");	//Order to add string to TX Buffer

	/*END SYSTEM PERIPHERALS INITIALIZATION*/


	/*2.) STARTS SIMPLE SD CARD MOUNTING AND TEST!*/

	//EXAMPLE CODE ON MOUNTING THE SD CARD AND WRITING TO A FILE
	//See function inside to see how to open a file
	SerialConsoleWriteString("\x0C\n\r-- SD/MMC Card Example on FatFs --\n\r");

	if(StartFilesystemAndTest() == false)
	{
		SerialConsoleWriteString("SD CARD failed! Check your connections. System will restart in 5 seconds...");
		delay_cycles_ms(5000);
		system_reset();
	}
	else
	{
		SerialConsoleWriteString("SD CARD mount success! Filesystem also mounted. \r\n");
	}

	/*END SIMPLE SD CARD MOUNTING AND TEST!*/


	/*3.) STARTS BOOTLOADER HERE!*/
	
	/*************************************************** BOOTLOADER ***************************************************/
	
	// Initialize the `nvm_parameters` struct.
	struct nvm_parameters parameters;
	
	char helpStr[64]; //Used to help print values
	
	nvm_get_parameters (&parameters); //Get NVM parameters
	snprintf(helpStr, 63,"NVM Info: Number of Pages %d. Size of a page: %d bytes. \r\n", parameters.nvm_number_of_pages, parameters.page_size);
	SerialConsoleWriteString(helpStr);
	
	
	uint8_t readBuffer[256]; //Buffer the size of one row
	
	bootloader_file_init[0] = LUN_ID_SD_MMC_0_MEM + '0';
	res_BOOT = f_open(&file_object, (char const *)bootloader_file_init, FA_READ);
	
	if(res_BOOT!= FR_OK)
	{
		
		bootloader_init(readBuffer);
		//jumpToApplication();
	}
	
	
	// Store the current status of the button the in the `pin_state` variable.
	bool pin_state = port_pin_get_input_level(BUTTON_0_PIN);
	
	// Try to open the `FlagA.txt` file, and store the result in `resA`.
	testA_file_name[0] = LUN_ID_SD_MMC_0_MEM + '0';
	resA = f_open(&file_object, (char const *)testA_file_name, FA_READ);
		
	// Try to open the `FlagB.txt` file, and store the result in `resB`.
	testB_file_name[0] = LUN_ID_SD_MMC_0_MEM + '0';
	resB = f_open(&file_object, (char const *)testB_file_name, FA_READ);
	
	// If the button is pressed and we found FlagA.txt, load the `TestA.bin` file
	// into memory. Also handle the default condition when no flags are present.
	if ((!pin_state && resA == FR_OK)/* || (resA != FR_OK && resB != FR_OK && pin_state)*/)
	{
		// Now that, we found `FlagA.txt` we can delete it.
		f_unlink(testA_file_name);
		f_unlink(bootloader_file_init);
		
		//Read SD Card File.
		TestA_bin_file[0] = LUN_ID_SD_MMC_0_MEM + '0';
		res = f_open(&file_object, (char const *)TestA_bin_file, FA_READ);
		
		// If there was a problem print out the error message.
		if (res != FR_OK)
			SerialConsoleWriteString("Could not open test file!\r\n");
		
		// Call the `reset_binary` function, which will load TestA.bin into
		// memory by writing it into the MCU memory.
		reset_binary(readBuffer);
	}
	else if (!pin_state && resB == FR_OK)
	{
		// Now that, we found `FlagB.txt` we can delete it.
		f_unlink(testB_file_name);
		f_unlink(bootloader_file_init);
		
		//Read SD Card File
		TestB_bin_file[0] = LUN_ID_SD_MMC_0_MEM + '0';
		res = f_open(&file_object, (char const *)TestB_bin_file, FA_READ);

		// If there was a problem print out the error message.
		if (res != FR_OK)
			SerialConsoleWriteString("Could not open test file!\r\n");
		
		// Call the `reset_binary` function, which will load TestA.bin into
		// memory by writing it into the MCU memory.
		reset_binary(readBuffer);
	}

	/*************************************************** BOOTLOADER ***************************************************/
		
	// PERFORM BOOTLOADER HERE!

	// #define MEM_EXAMPLE
// #ifdef MEM_EXAMPLE
// 	//Example
// 	//Check out the NVM API at http://asf.atmel.com/docs/latest/samd21/html/group__asfdoc__sam0__nvm__group.html#asfdoc_sam0_nvm_examples . It provides important information too!
// 
// 	//We will ask the NVM driver for information on the MCU (SAMD21)
// 	struct nvm_parameters parameters;
// 	char helpStr[64]; //Used to help print values
// 	nvm_get_parameters (&parameters); //Get NVM parameters
// 	snprintf(helpStr, 63,"NVM Info: Number of Pages %d. Size of a page: %d bytes. \r\n", parameters.nvm_number_of_pages, parameters.page_size);
// 	SerialConsoleWriteString(helpStr);
// 	//The SAMD21 NVM (and most if not all NVMs) can only erase and write by certain chunks of data size.
// 	//The SAMD21 can WRITE on pages. However erase are done by ROW. One row is conformed by four (4) pages.
// 	//An erase is mandatory before writing to a page.
// 	
// 
// 	//We will do the following in this example:
// 	//Erase the first row of the application data
// 	//Read a page of data from the SD Card. The SD CARD initialization writes a file with this data as its test for initialization
// 	//Write it to the first row.
// 	//CheckCRC32 of both files.
// 	
// 	uint8_t readBuffer[256]; //Buffer the size of one row
// 	uint32_t numBytesRead = 0;
// 
// 	//Erase first page of Main FW
// 	//This page starts at APP_START_ADDRESS
// 	enum status_code nvmError = nvm_erase_row(APP_START_ADDRESS);
// 	if(nvmError != STATUS_OK)
// 	{
// 		SerialConsoleWriteString("Erase error");
// 	}
// 	
// 	//Make sure it got erased - we read the page. Erasure in NVM is an 0xFF
// 	for(int iter = 0; iter < 256; iter++)
// 	{
// 		char *a = (char *)(APP_START_ADDRESS + iter); //Pointer pointing to address APP_START_ADDRESS
// 		if(*a != 0xFF)
// 		{
// 			SerialConsoleWriteString("Error - test page is not erased!");
// 			break;	
// 		}
// 	}	
// 
// 	//Read SD Card File
// 	test_bin_file[0] = LUN_ID_SD_MMC_0_MEM + '0';
// 	res = f_open(&file_object, (char const *)test_bin_file, FA_READ);
// 		
// 	if (res != FR_OK)
// 	{
// 		SerialConsoleWriteString("Could not open test file!\r\n");
// 	}
// 
// 	int numBytesLeft = 256;
// 	numBytesRead = 0;
// 	int numberBytesTotal = 0;
// 	while(numBytesLeft  != 0) 
// 	{
// 		
// 		res = f_read(&file_object, &readBuffer[numberBytesTotal], numBytesLeft, &numBytesRead); //Question to students: What is numBytesRead? What are we doing here?
// 		numBytesLeft -= numBytesLeft; 
// 		numberBytesTotal += numBytesRead;
// 	}
// 	
// 
// 
// 	//Write data to first row. Writes are per page, so we need four writes to write a complete row
// 	res = nvm_write_buffer (APP_START_ADDRESS, &readBuffer[0], 64);
// 	res = nvm_write_buffer (APP_START_ADDRESS + 64, &readBuffer[64], 64);
// 	res = nvm_write_buffer (APP_START_ADDRESS + 128, &readBuffer[128], 64);
// 	res = nvm_write_buffer (APP_START_ADDRESS + 192, &readBuffer[192], 64);
// 
// 		if (res != FR_OK)
// 		{
// 			SerialConsoleWriteString("Test write to NVM failed!\r\n");
// 		}
// 		else
// 		{
// 			SerialConsoleWriteString("Test write to NVM succeeded!\r\n");
// 		}
// 
// 	//CRC32 Calculation example: Please read http://asf.atmel.com/docs/latest/samd21/html/group__asfdoc__sam0__drivers__crc32__group.html
// 
// 	//ERRATA Part 1 - To be done before RAM CRC
// 	//CRC of SD Card. Errata 1.8.3 determines we should run this code every time we do CRC32 from a RAM source. See http://ww1.microchip.com/downloads/en/DeviceDoc/SAM-D21-Family-Silicon-Errata-and-DataSheet-Clarification-DS80000760D.pdf Section 1.8.3
// 	uint32_t resultCrcSd = 0;
// 	*((volatile unsigned int*) 0x41007058) &= ~0x30000UL;
// 
// 	//CRC of SD Card
// 	enum status_code crcres = dsu_crc32_cal	(readBuffer	,256, &resultCrcSd); //Instructor note: Was it the third parameter used for? Please check how you can use the third parameter to do the CRC of a long data stream in chunks - you will need it!
// 	
// 	//Errata Part 2 - To be done after RAM CRC
// 	*((volatile unsigned int*) 0x41007058) |= 0x20000UL;
// 	 
// 	 
// 	//CRC of memory (NVM)
// 	uint32_t resultCrcNvm = 0;
// 	crcres |= dsu_crc32_cal	(APP_START_ADDRESS	,256, &resultCrcNvm);
// 	
// 	
// 
// 	if (crcres != STATUS_OK)
// 	{
// 		SerialConsoleWriteString("Could not calculate CRC!!\r\n");
// 	}
// 	else
// 	{
// 		snprintf(helpStr, 63,"CRC SD CARD: %d  CRC NVM: %d \r\n", resultCrcSd, resultCrcNvm);
// 		SerialConsoleWriteString(helpStr);
// 	}
// 
// 
// #endif

	/*END BOOTLOADER HERE!*/

	//4.) DEINITIALIZE HW AND JUMP TO MAIN APPLICATION!
	SerialConsoleWriteString("ESE516 - EXIT BOOTLOADER");	//Order to add string to TX Buffer
	delay_cycles_ms(100); //Delay to allow print
		
		//Deinitialize HW - deinitialize started HW here!
		DeinitializeSerialConsole(); //Deinitializes UART
		sd_mmc_deinit(); //Deinitialize SD CARD


		//Jump to application
		jumpToApplication();

		//Should not reach here! The device should have jumped to the main FW.
	
}







/******************************************************************************
* Static Functions
******************************************************************************/



/**************************************************************************//**
* function      static void StartFilesystemAndTest()
* @brief        Starts the filesystem and tests it. Sets the filesystem to the global variable fs
* @details      Jumps to the main application. Please turn off ALL PERIPHERALS that were turned on by the bootloader
*				before performing the jump!
* @return       Returns true is SD card and file system test passed. False otherwise.
******************************************************************************/
static bool StartFilesystemAndTest(void)
{
	bool sdCardPass = true;
	uint8_t binbuff[256];

	//Before we begin - fill buffer for binary write test
	//Fill binbuff with values 0x00 - 0xFF
	for(int i = 0; i < 256; i++)
	{
		binbuff[i] = i;
	}

	//MOUNT SD CARD
	Ctrl_status sdStatus= SdCard_Initiate();
	if(sdStatus == CTRL_GOOD) //If the SD card is good we continue mounting the system!
	{
		SerialConsoleWriteString("SD Card initiated correctly!\n\r");

		//Attempt to mount a FAT file system on the SD Card using FATFS
		SerialConsoleWriteString("Mount disk (f_mount)...\r\n");
		memset(&fs, 0, sizeof(FATFS));
		res = f_mount(LUN_ID_SD_MMC_0_MEM, &fs); //Order FATFS Mount
		if (FR_INVALID_DRIVE == res)
		{
			LogMessage(LOG_INFO_LVL ,"[FAIL] res %d\r\n", res);
			sdCardPass = false;
			goto main_end_of_test;
		}
		SerialConsoleWriteString("[OK]\r\n");

		//Create and open a file
		SerialConsoleWriteString("Create a file (f_open)...\r\n");

		test_file_name[0] = LUN_ID_SD_MMC_0_MEM + '0';
		res = f_open(&file_object,
		(char const *)test_file_name,
		FA_CREATE_ALWAYS | FA_WRITE);
		
		if (res != FR_OK)
		{
			LogMessage(LOG_INFO_LVL ,"[FAIL] res %d\r\n", res);
			sdCardPass = false;
			goto main_end_of_test;
		}

		SerialConsoleWriteString("[OK]\r\n");

		//Write to a file
		SerialConsoleWriteString("Write to test file (f_puts)...\r\n");

		if (0 == f_puts("Test SD/MMC stack\n", &file_object))
		{
			f_close(&file_object);
			LogMessage(LOG_INFO_LVL ,"[FAIL]\r\n");
			sdCardPass = false;
			goto main_end_of_test;
		}

		SerialConsoleWriteString("[OK]\r\n");
		f_close(&file_object); //Close file
		SerialConsoleWriteString("Test is successful.\n\r");


		//Write binary file
		//Read SD Card File
		test_bin_file[0] = LUN_ID_SD_MMC_0_MEM + '0';
		res = f_open(&file_object, (char const *)test_bin_file, FA_WRITE | FA_CREATE_ALWAYS);
		
		if (res != FR_OK)
		{
			SerialConsoleWriteString("Could not open binary file!\r\n");
			LogMessage(LOG_INFO_LVL ,"[FAIL] res %d\r\n", res);
			sdCardPass = false;
			goto main_end_of_test;
		}

		//Write to a binaryfile
		SerialConsoleWriteString("Write to test file (f_write)...\r\n");
		uint32_t varWrite = 0;
		if (0 != f_write(&file_object, binbuff,256, (UINT*) &varWrite))
		{
			f_close(&file_object);
			LogMessage(LOG_INFO_LVL ,"[FAIL]\r\n");
			sdCardPass = false;
			goto main_end_of_test;
		}

		SerialConsoleWriteString("[OK]\r\n");
		f_close(&file_object); //Close file
		SerialConsoleWriteString("Test is successful.\n\r");
		
		main_end_of_test:
		SerialConsoleWriteString("End of Test.\n\r");

	}
	else
	{
		SerialConsoleWriteString("SD Card failed initiation! Check connections!\n\r");
		sdCardPass = false;
	}

	return sdCardPass;
}



/**************************************************************************//**
* function      static void jumpToApplication(void)
* @brief        Jumps to main application
* @details      Jumps to the main application. Please turn off ALL PERIPHERALS that were turned on by the bootloader
*				before performing the jump!
* @return       
******************************************************************************/
static void jumpToApplication(void)
{
// Function pointer to application section
void (*applicationCodeEntry)(void);

// Rebase stack pointer
__set_MSP(*(uint32_t *) APP_START_ADDRESS);

// Rebase vector table
SCB->VTOR = ((uint32_t) APP_START_ADDRESS & SCB_VTOR_TBLOFF_Msk);

// Set pointer to application section
applicationCodeEntry =
(void (*)(void))(unsigned *)(*(unsigned *)(APP_START_RESET_VEC_ADDRESS));

// Jump to application. By calling applicationCodeEntry() as a function we move the PC to the point in memory pointed by applicationCodeEntry, 
//which should be the start of the main FW.
applicationCodeEntry();
}



/**************************************************************************//**
* function      static void configure_nvm(void)
* @brief        Configures the NVM driver
* @details      
* @return       
******************************************************************************/
static void configure_nvm(void)
{
    struct nvm_config config_nvm;
    nvm_get_config_defaults(&config_nvm);
    config_nvm.manual_page_write = false;
    nvm_set_config(&config_nvm);
}



