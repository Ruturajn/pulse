/**************************************************************************/ /**
 * @file      WifiHandler.h
 * @brief     File to handle HTTP Download and MQTT support
 * @author    Eduardo Garcia
 * @date      2020-01-01

 ******************************************************************************/

#pragma once

#pragma once
#ifdef __cplusplus
extern "C" {
#endif

/******************************************************************************
 * Includes
 ******************************************************************************/
#include "asf.h"

#include "MQTTClient/Wrapper/mqtt.h"
#include "SerialConsole.h"
#include "asf.h"
#include "driver/include/m2m_wifi.h"
#include "iot/http/http_client.h"
#include "main.h"
#include "socket/include/socket.h"
#include "stdio_serial.h"
#include "SmokeSensThread/SmokeSensThread.h"
#include "INA219OneThread/INA219OneThread.h"
#include "INA219TwoThread/INA219TwoThread.h"
#include "CliThread/CliThread.h"
/******************************************************************************
 * Defines
 ******************************************************************************/

#define WIFI_MQTT_INIT 0        ///< State for Wifi handler to Initialize MQTT Connection
#define WIFI_MQTT_HANDLE 1      ///< State for Wifi handler to Handle MQTT Connection
#define WIFI_DOWNLOAD_INIT 2    ///< State for Wifi handler to Initialize Download Connection
#define WIFI_DOWNLOAD_HANDLE 3  ///< State for Wifi handler to Handle Download Connection

#define WIFI_TASK_SIZE 1000
#define WIFI_PRIORITY (configMAX_PRIORITIES - 2)

/** Wi-Fi AP Settings. */
#define MAIN_WLAN_SSID "Pixel_8158"             /**< Destination SSID. Change to your WIFI SSID */
#define MAIN_WLAN_AUTH M2M_WIFI_SEC_WPA_PSK /**< Security manner */
#define MAIN_WLAN_PSK "123456789"            /**< Password for Destination SSID. Change to your password. Please dont hack my WiFi router */

/** IP address parsing. */
#define IPV4_BYTE(val, index) ((val >> (index * 8)) & 0xFF)

/** Content URI for download. */
#define MAIN_HTTP_FILE_URL "http://74.235.242.136/ESE516_MAIN_FW.bin"  ///< Change me to the URL to download your OTAU binary file from!

/** Maximum size for packet buffer. */
#define MAIN_BUFFER_MAX_SIZE (512)
/** Maximum file name length. */
#define MAIN_MAX_FILE_NAME_LENGTH (64)
/** Maximum file extension length. */
#define MAIN_MAX_FILE_EXT_LENGTH (8)
/** Output format with '0'. */
#define MAIN_ZERO_FMT(SZ) (SZ == 4) ? "%04d" : (SZ == 3) ? "%03d" : (SZ == 2) ? "%02d" : "%d"
#define GAME_SIZE 20  ///< Number of plays in game

typedef enum {
    NOT_READY = 0,         /*!< Not ready. */
    STORAGE_READY = 0x01,  /*!< Storage is ready. */
    WIFI_CONNECTED = 0x02, /*!< Wi-Fi is connected. */
    GET_REQUESTED = 0x04,  /*!< GET request is sent. */
    DOWNLOADING = 0x08,    /*!< Running to download. */
    COMPLETED = 0x10,      /*!< Download completed. */
    CANCELED = 0x20        /*!< Download canceled. */
} download_state;

// Structure definition that holds IMU data
struct ImuDataPacket {
    int16_t xmg;
    int16_t ymg;
    int16_t zmg;
};

// Structure to hold a game packet
struct GameDataPacket {
    uint8_t game[GAME_SIZE];
};

// Structure to hold an RGB LED Color packet
struct RgbColorPacket {
    uint8_t red;
    uint8_t green;
    uint8_t blue;
};

/*************************** OUR CODE HERE ***************************/
struct INA219Sens1 {
	float voltage;
	float current;
	float power;
	bool comm_status;
	bool efuse_1_stat;	
};

struct INA219Sens2 {
	float voltage;
	float current;
	float power;
	bool comm_status;
	bool efuse_2_stat;
};

extern bool efuse_stat_1;
extern bool efuse_stat_2;

// Smoke sensor data is `uint16_t` type, so no need for a separate 
// struct.
/*************************** OUR CODE HERE ***************************/

/* Max size of UART buffer. */
#define MAIN_CHAT_BUFFER_SIZE 64

/* Max size of MQTT buffer. */
#define MAIN_MQTT_BUFFER_SIZE 512

/* Limitation of user name. */
#define MAIN_CHAT_USER_NAME_SIZE 64




// #define PLAYER1 1  ///< Comment me to compile for player 2. Uncomment me to define for player 1.
// 
// #ifdef PLAYER1
// /* Chat MQTT topic. */
// #define LED_TOPIC "P1_LED_ESE516_T0"            // Students to change to an unique identifier for each device! LED Data
// #define GAME_TOPIC_IN "P1_GAME_ESE516_T0"       // Students to change to an unique identifier for each device! Game Data
// #define GAME_TOPIC_OUT "P2_GAME_ESE516_T0"      // Students to change to an unique identifier for each device! Game Data
// #define IMU_TOPIC "P1_IMU_ESE516_T0"            // Students to change to an unique identifier for each device! IMU Data
// #define DISTANCE_TOPIC "P1_DISTANCE_ESE516_T0"  // Students to change to an unique identifier for each device! Distance Data
// #define TEMPERATURE_TOPIC "P1_TEMPERATURE_ESE516_T0" // Students to change to an unique identifier for each device! Distance Data
// 
// #else
/* Chat MQTT topic. */
#define LED_TOPIC "P2_LED_ESE516_T7"            // Students to change to an unique identifier for each device! LED Data
#define GAME_TOPIC_IN "P2_GAME_ESE516_T7"       // Students to change to an unique identifier for each device! Game Data
#define GAME_TOPIC_OUT "P1_GAME_ESE516_T7"      // Students to change to an unique identifier for each device! Game Data
#define IMU_TOPIC "P2_IMU_ESE516_T7"            // Students to change to an unique identifier for each device! IMU Data
#define DISTANCE_TOPIC "P2_DISTANCE_ESE516_T7"  // Students to change to an unique identifier for each device! Distance Data
#define TEMPERATURE_TOPIC "P2_TEMPERATURE_ESE516_T7" // Students to change to an unique identifier for each device! Distance Data

#define CURR_SENS1_VAL "CURR_SENS1_VAL_T7"
#define CURR_SENS2_VAL "CURR_SENS2_VAL_T7"
#define VOLT_SENS1_VAL "VOLT_SENS1_VAL_T7"
#define VOLT_SENS2_VAL "VOLT_SENS2_VAL_T7"
#define SMOKE_SENS_VAL "SMOKE_SENS_VAL_T7"
#define SENS1_COMM     "SENS1_COMM_T7"
#define SENS2_COMM     "SENS2_COMM_T7"
#define EFUSE1_STAT    "EFUSE1_STAT_T7"
#define EFUSE2_STAT    "EFUSE2_STAT_T7"
#define POW_SENS1_VAL  "POW_SENS1_VAL_T7"
#define POW_SENS2_VAL  "POW_SENS2_VAL_T7"
#define EFUSE1_CONTROL "EFUSE1_CONTROL_T7"
#define EFUSE2_CONTROL "EFUSE2_CONTROL_T7"
#define OTAFU_CONTROL "OTAFU_T7"

// #endif

#define LED_TOPIC_LED_OFF "false"
#define LED_TOPIC_LED_ON "true"

// Cloud MQTT User
#define CLOUDMQTT_USER_ID "fusiot"

// Cloud MQTT pASSWORD
#define CLOUDMQTT_USER_PASSWORD "fusiot-pr"

#define CLOUDMQTT_PORT 1883

/*
 * A MQTT broker server which was connected.
 * m2m.eclipse.org is public MQTT broker.
 */
static const char main_mqtt_broker[] = "broker.hivemq.com";

#define STRING_EOL "\r\n"
#define STRING_HEADER                                                                 \
    "-- HTTP file downloader example --" STRING_EOL "-- " BOARD_NAME " --" STRING_EOL \
    "-- Compiled: "__DATE__                                                           \
    " "__TIME__                                                                       \
    " --" STRING_EOL

/******************************************************************************
 * Structures and Enumerations
 ******************************************************************************/

/******************************************************************************
 * Global Function Declaration
 ******************************************************************************/
void vWifiTask(void *pvParameters);
void init_storage(void);
void WifiHandlerSetState(uint8_t state);
int WifiAddDistanceDataToQueue(uint16_t *distance);
int WifiAddImuDataToQueue(struct ImuDataPacket *imuPacket);
int WifiAddGameDataToQueue(struct GameDataPacket *game);
void SubscribeHandlerLedTopic(MessageData *msgData);
void SubscribeHandlerGameTopic(MessageData *msgData);
void SubscribeHandlerImuTopic(MessageData *msgData);
void SubscribeHandlerDistanceTopic(MessageData *msgData);
void configure_extint_channel(void);
void configure_extint_callbacks(void);

/*************************** OUR CODE HERE ***************************/
int WifiAddSmokeDataToQueue(bool *smoke_stat);
int WifiAddINA219OneDataToQueue(struct INA219Sens1 *ina);
int WifiAddINA219TwoDataToQueue(struct INA219Sens2 *ina);

void SubscribeHandlerEFUSE1Topic(MessageData *msgData);
void SubscribeHandlerEFUSE2Topic(MessageData *msgData);
/*************************** OUR CODE HERE ***************************/

#ifdef __cplusplus
}
#endif
