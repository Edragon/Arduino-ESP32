/**
 * @file LoRaWAN_OTAA_ABP.ino
 * @author rakwireless.com
 * @brief LoRaWan node example with OTAA/ABP registration
 * @version 0.1
 * @date 2020-08-21
 *
 * @copyright Copyright (c) 2020
 *
 * @note RAK4631 GPIO mapping to nRF52840 GPIO ports
   RAK4631    <->  nRF52840
   WB_IO1     <->  P0.17 (GPIO 17)
   WB_IO2     <->  P1.02 (GPIO 34)
   WB_IO3     <->  P0.21 (GPIO 21)
   WB_IO4     <->  P0.04 (GPIO 4)
   WB_IO5     <->  P0.09 (GPIO 9)
   WB_IO6     <->  P0.10 (GPIO 10)
   WB_SW1     <->  P0.01 (GPIO 1)
   WB_A0      <->  P0.04/AIN2 (AnalogIn A2)
   WB_A1      <->  P0.31/AIN7 (AnalogIn A7)
 */
#include <Arduino.h>
#include <LoRaWAN-Arduino.h> // <LoRaWan-RAK4630.h> //http://librarymanager/All#SX126x
#include <SPI.h>

// RAK4630 supply two LED
#ifndef LED_BUILTIN
#define LED_BUILTIN 35
#endif

#ifndef LED_BUILTIN2
#define LED_BUILTIN2 36
#endif

bool doOTAA = true;												  // OTAA is used by default.
#define SCHED_MAX_EVENT_DATA_SIZE APP_TIMER_SCHED_EVENT_DATA_SIZE /**< Maximum size of scheduler events. */
#define SCHED_QUEUE_SIZE 60										  /**< Maximum number of events in the scheduler queue. */
#define LORAWAN_DATERATE DR_0									  /*LoRaMac datarates definition, from DR_0 to DR_5*/
#define LORAWAN_TX_POWER TX_POWER_5								  /*LoRaMac tx power definition, from TX_POWER_0 to TX_POWER_15*/
#define JOINREQ_NBTRIALS 3										  /**< Number of trials for the join request. */
DeviceClass_t g_CurrentClass = CLASS_A;							  /* class definition*/
LoRaMacRegion_t g_CurrentRegion = LORAMAC_REGION_AS923_3;		  /* Region:EU868*/
lmh_confirm g_CurrentConfirm = LMH_UNCONFIRMED_MSG;				  /* confirm/unconfirm packet definition*/
uint8_t gAppPort = LORAWAN_APP_PORT;							  /* data port*/

/**@brief Structure containing LoRaWan parameters, needed for lmh_init()
 */
static lmh_param_t g_lora_param_init = {LORAWAN_ADR_ON, LORAWAN_DATERATE, LORAWAN_PUBLIC_NETWORK, JOINREQ_NBTRIALS, LORAWAN_TX_POWER, LORAWAN_DUTYCYCLE_OFF};

// Foward declaration
static void lorawan_has_joined_handler(void);
static void lorawan_join_failed_handler(void);
static void lorawan_rx_handler(lmh_app_data_t *app_data);
static void lorawan_confirm_class_handler(DeviceClass_t Class);
static void send_lora_frame(void);

/**@brief Structure containing LoRaWan callback functions, needed for lmh_init()
 */
static lmh_callback_t g_lora_callbacks = {BoardGetBatteryLevel, BoardGetUniqueId, BoardGetRandomSeed,
										  lorawan_rx_handler, lorawan_has_joined_handler, lorawan_confirm_class_handler, lorawan_join_failed_handler};
// OTAA keys !!!! KEYS ARE MSB !!!!
uint8_t nodeDeviceEUI[8] = {0xac, 0x1f, 0x09, 0xff, 0xfe, 0x06, 0x79, 0xDB};
uint8_t nodeAppEUI[8] = {0x70, 0xB3, 0xD5, 0x7E, 0xD0, 0x02, 0x01, 0xE1};
uint8_t nodeAppKey[16] = {0x2b, 0x84, 0xe0, 0xb0, 0x9b, 0x68, 0xe5, 0xcb, 0x42, 0x17, 0x6f, 0xe7, 0x53, 0xdc, 0xee, 0x79};

// ABP keys
uint32_t nodeDevAddr = 0x260116F8;
uint8_t nodeNwsKey[16] = {0x7E, 0xAC, 0xE2, 0x55, 0xB8, 0xA5, 0xE2, 0x69, 0x91, 0x51, 0x96, 0x06, 0x47, 0x56, 0x9D, 0x23};
uint8_t nodeAppsKey[16] = {0xFB, 0xAC, 0xB6, 0x47, 0xF3, 0x58, 0x45, 0xC7, 0x50, 0x7D, 0xBF, 0x16, 0x8B, 0xA8, 0xC1, 0x7C};

// Private defination
#define LORAWAN_APP_DATA_BUFF_SIZE 64										  /**< buffer size of the data to be transmitted. */
#define LORAWAN_APP_INTERVAL 20000											  /**< Defines for user timer, the application data transmission interval. 20s, value in [ms]. */
static uint8_t m_lora_app_data_buffer[LORAWAN_APP_DATA_BUFF_SIZE];			  //< Lora user application data buffer.
static lmh_app_data_t m_lora_app_data = {m_lora_app_data_buffer, 0, 0, 0, 0}; //< Lora user application data structure.

TimerEvent_t appTimer;
static uint32_t timers_init(void);
static uint32_t count = 0;
static uint32_t count_fail = 0;

// #ifdef NRF52_SERIES
// // Replace PIN_SPI_MISO, PIN_SPI_SCK, PIN_SPI_MOSI with your
// SPIClass SPI_LORA(NRF_SPIM2, MISO, SCK, MOSI);
// #endif

time_t wait_for_join;

void setup()
{
	pinMode(LED_BUILTIN, OUTPUT);
	digitalWrite(LED_BUILTIN, LOW);

	// Initialize Serial for debug output
	time_t timeout = millis();
	Serial.begin(115200);
	while (!Serial)
	{
		if ((millis() - timeout) < 5000)
		{
			delay(100);
		}
		else
		{
			break;
		}
	}

	// Initialize LoRa chip.
#ifdef USE_RAK13300
	// // Define the HW configuration between MCU and SX126x
	// hwConfig.CHIP_TYPE = SX1262_CHIP;		  // Example uses an eByte E22 module with an SX1262
	// hwConfig.PIN_LORA_RESET = WB_IO4;		  // LORA RESET
	// hwConfig.PIN_LORA_NSS = SS;				  // LORA SPI CS
	// hwConfig.PIN_LORA_SCLK = SCK;			  // LORA SPI CLK
	// hwConfig.PIN_LORA_MISO = MISO;			  // LORA SPI MISO
	// hwConfig.PIN_LORA_DIO_1 = WB_IO6;		  // LORA DIO_1
	// hwConfig.PIN_LORA_BUSY = WB_IO5;		  // LORA SPI BUSY
	// hwConfig.PIN_LORA_MOSI = MOSI;			  // LORA SPI MOSI
	// hwConfig.RADIO_TXEN = -1;				  // LORA ANTENNA TX ENABLE
	// hwConfig.RADIO_RXEN = WB_IO3;			  // LORA ANTENNA RX ENABLE
	// hwConfig.USE_DIO2_ANT_SWITCH = true;	  // Example uses an CircuitRocks Alora RFM1262 which uses DIO2 pins as antenna control
	// hwConfig.USE_DIO3_TCXO = true;			  // Example uses an CircuitRocks Alora RFM1262 which uses DIO3 to control oscillator voltage
	// hwConfig.USE_DIO3_ANT_SWITCH = false;	  // Only Insight ISP4520 module uses DIO3 as antenna control
	// hwConfig.USE_RXEN_ANT_PWR = true;		  // RXEN is used as power for antenna switch

	Serial.println("=====================================");
	Serial.println("Welcome to RAK3401 LoRaWan!!!");
	pinMode(WB_IO2, OUTPUT);
	digitalWrite(WB_IO2, HIGH);
	delay(100);
	lora_rak13300_init();
#elif defined (_VARIANT_RAK3400_)
	Serial.println("=====================================");
	Serial.println("Welcome to RAK3401 1W LoRaWan!!!");
	pinMode(WB_IO2, OUTPUT);
	digitalWrite(WB_IO2, HIGH);
	delay(100);
	lora_rak3400_init();
#else
	Serial.println("=====================================");
	Serial.println("Welcome to RAK4630 LoRaWan!!!");
	lora_rak4630_init();
#endif

	if (doOTAA)
	{
		Serial.println("Type: OTAA");
	}
	else
	{
		Serial.println("Type: ABP");
	}

	switch (g_CurrentRegion)
	{
	case LORAMAC_REGION_AS923:
		Serial.println("Region: AS923");
		break;
	case LORAMAC_REGION_AU915:
		Serial.println("Region: AU915");
		break;
	case LORAMAC_REGION_CN470:
		Serial.println("Region: CN470");
		break;
	case LORAMAC_REGION_CN779:
		Serial.println("Region: CN779");
		break;
	case LORAMAC_REGION_EU433:
		Serial.println("Region: EU433");
		break;
	case LORAMAC_REGION_IN865:
		Serial.println("Region: IN865");
		break;
	case LORAMAC_REGION_EU868:
		Serial.println("Region: EU868");
		break;
	case LORAMAC_REGION_KR920:
		Serial.println("Region: KR920");
		break;
	case LORAMAC_REGION_US915:
		Serial.println("Region: US915");
		break;
	case LORAMAC_REGION_RU864:
		Serial.println("Region: RU864");
		break;
	case LORAMAC_REGION_AS923_2:
		Serial.println("Region: AS923-2");
		break;
	case LORAMAC_REGION_AS923_3:
		Serial.println("Region: AS923-3");
		break;
	case LORAMAC_REGION_AS923_4:
		Serial.println("Region: AS923-4");
		break;
	}
	Serial.println("=====================================");

	// creat a user timer to send data to server period
	uint32_t err_code;
	err_code = timers_init();
	if (err_code != 0)
	{
		Serial.printf("timers_init failed - %d\n", err_code);
		return;
	}

	// Setup the EUIs and Keys
	if (doOTAA)
	{
		lmh_setDevEui(nodeDeviceEUI);
		lmh_setAppEui(nodeAppEUI);
		lmh_setAppKey(nodeAppKey);
	}
	else
	{
		lmh_setNwkSKey(nodeNwsKey);
		lmh_setAppSKey(nodeAppsKey);
		lmh_setDevAddr(nodeDevAddr);
	}

	// Initialize LoRaWan
	err_code = lmh_init(&g_lora_callbacks, g_lora_param_init, doOTAA, g_CurrentClass, g_CurrentRegion);
	if (err_code != 0)
	{
		Serial.printf("lmh_init failed - %d\n", err_code);
		return;
	}

	// Start Join procedure
	lmh_join();

	wait_for_join = millis();
}

bool send_now = false;

void loop()
{
	// Put your application tasks here, like reading of sensors,
	// Controlling actuators and/or other functions.

	// if (!has_joined)
	// {
	// 	if ((millis() - wait_for_join) > 5000)
	// 	{
	// 		wait_for_join = millis();
	// 		Serial.printf("Join status %d\n", lmh_join_status_get());
	// 		// if (lmh_join_status_get() == LMH_FAILED)
	// 		{
	// 			Serial.println("Join failed");
	// 			// Retry after a delay
	// 			delay(5000);
	// 			Serial.println("Retry to join");
	// 			lmh_join();
	// 		}
	// 	}
	// }
	if (send_now)
	{
		Serial.println("Sending frame now...");
		send_now = false;
		send_lora_frame();
	}
}

/**@brief LoRa function for handling HasJoined event.
 */
void lorawan_has_joined_handler(void)
{
	if (doOTAA == true)
	{
		Serial.println("OTAA Mode, Network Joined!");
	}
	else
	{
		Serial.println("ABP Mode");
	}

	lmh_error_status ret = lmh_class_request(g_CurrentClass);
	if (ret == LMH_SUCCESS)
	{
		delay(1000);
		TimerSetValue(&appTimer, LORAWAN_APP_INTERVAL);
		TimerStart(&appTimer);
	}
}
/**@brief LoRa function for handling OTAA join failed
 */
static void lorawan_join_failed_handler(void)
{
	Serial.println("OTAA join failed!");
	Serial.println("Check your EUI's and Keys's!");
	Serial.println("Check if a Gateway is in range!");
	lmh_join();
}
/**@brief Function for handling LoRaWan received data from Gateway
 *
 * @param[in] app_data  Pointer to rx data
 */
void lorawan_rx_handler(lmh_app_data_t *app_data)
{
	Serial.printf("LoRa Packet received on port %d, size:%d, rssi:%d, snr:%d, data:%s\n",
				  app_data->port, app_data->buffsize, app_data->rssi, app_data->snr, app_data->buffer);
}

void lorawan_confirm_class_handler(DeviceClass_t Class)
{
	Serial.printf("switch to class %c done\n", "ABC"[Class]);
	// Informs the server that switch has occurred ASAP
	m_lora_app_data.buffsize = 0;
	m_lora_app_data.port = gAppPort;
	lmh_send(&m_lora_app_data, g_CurrentConfirm);
}

void send_lora_frame(void)
{
	if (lmh_join_status_get() != LMH_SET)
	{
		// Not joined, try again later
		return;
	}

	uint32_t i = 0;
	memset(m_lora_app_data.buffer, 0, LORAWAN_APP_DATA_BUFF_SIZE);
	m_lora_app_data.port = gAppPort;
	m_lora_app_data.buffer[i++] = 'H';
	m_lora_app_data.buffer[i++] = 'e';
	m_lora_app_data.buffer[i++] = 'l';
	m_lora_app_data.buffer[i++] = 'l';
	m_lora_app_data.buffer[i++] = 'o';
	m_lora_app_data.buffer[i++] = '!';
	m_lora_app_data.buffsize = i;

	lmh_error_status error = lmh_send(&m_lora_app_data, g_CurrentConfirm);
	if (error == LMH_SUCCESS)
	{
		count++;
		Serial.printf("lmh_send ok count %d\n", count);
	}
	else
	{
		count_fail++;
		Serial.printf("lmh_send fail count %d\n", count_fail);
	}
}

/**@brief Function for handling user timerout event.
 */
void tx_lora_periodic_handler(void)
{
	TimerSetValue(&appTimer, LORAWAN_APP_INTERVAL);
	TimerStart(&appTimer);
	send_now = true;
}

/**@brief Function for the Timer initialization.
 *
 * @details Initializes the timer module. This creates and starts application timers.
 */
uint32_t timers_init(void)
{
	TimerInit(&appTimer, tx_lora_periodic_handler);
	return 0;
}
