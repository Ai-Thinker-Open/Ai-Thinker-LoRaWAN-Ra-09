///* USER CODE BEGIN Header */
///**
//  ******************************************************************************
//  * @file    subg_at.c
//  * @author  MCD Application Team
//  * @brief   AT command API
//  ******************************************************************************
//  * @attention
//  *
//  * Copyright (c) 2023 STMicroelectronics.
//  * All rights reserved.
//  *
//  * This software is licensed under terms that can be found in the LICENSE file
//  * in the root directory of this software component.
//  * If no LICENSE file comes with this software, it is provided AS-IS.
//  *
//  ******************************************************************************
//  */
///* USER CODE END Header */

///* Includes ------------------------------------------------------------------*/
//#include "platform.h"
//#include "subg_at.h"
//#include "sys_app.h"
//#include "stm32_tiny_sscanf.h"
//#include "app_version.h"
//#include "subghz_phy_version.h"
//#include "test_rf.h"
//#include "stm32_seq.h"
//#include "utilities_def.h"
//#include "radio.h"
//#include "stm32_timer.h"
//#include "stm32_systime.h"

///* USER CODE BEGIN Includes */

//#include "platform.h"
//#include "sys_app.h"
//#include "subghz_phy_app.h"
//#include "radio.h"

///* USER CODE BEGIN Includes */
//#include "stm32_timer.h"
//#include "stm32_seq.h"
//#include "utilities_def.h"
//#include "app_version.h"
//#include "subghz_phy_version.h"
//#include "stdio.h"
///* USER CODE END Includes */

//static RadioEvents_t RadioEvents;

///* Private define ------------------------------------------------------------*/

//typedef enum
//{
//  RX,
//  RX_TIMEOUT,
//  RX_ERROR,
//  TX,
//  TX_TIMEOUT,
//} States_t;

///* USER CODE BEGIN PD */
///* Configurations */
///*Timeout*/
//#define RX_TIMEOUT_VALUE              3000
//#define TX_TIMEOUT_VALUE              3000
///* PING string*/
//#define PING "PING"
///* PONG string*/
//#define PONG "PONG"
///*Size of the payload to be sent*/
///* Size must be greater of equal the PING and PONG*/
//#define MAX_APP_BUFFER_SIZE          255
//#if (PAYLOAD_LEN > MAX_APP_BUFFER_SIZE)
//#error PAYLOAD_LEN must be less or equal than MAX_APP_BUFFER_SIZE
//#endif /* (PAYLOAD_LEN > MAX_APP_BUFFER_SIZE) */
///* wait for remote to be in Rx, before sending a Tx frame*/
//#define RX_TIME_MARGIN                200
///* Afc bandwidth in Hz */
//#define FSK_AFC_BANDWIDTH             83333
///* LED blink Period*/
//#define LED_PERIOD_MS                 200

//static States_t State = RX;
///* App Rx Buffer*/
//static uint8_t BufferRx[MAX_APP_BUFFER_SIZE];
///* App Tx Buffer*/
//static uint8_t BufferTx[MAX_APP_BUFFER_SIZE];
///* Last  Received Buffer Size*/
//uint16_t RxBufferSize = 0;
///* Last  Received packer Rssi*/
//int8_t RssiValue = 0;

//int8_t tx_RssiValue = 0;

//int8_t rx_RssiValue = 0;

///* Last  Received packer SNR (in Lora modulation)*/
//int8_t SnrValue = 0;
///* Led Timers objects*/
//static UTIL_TIMER_Object_t timerLed;
///* device state. Master: true, Slave: false*/
//bool isMaster = false;
///* random delay to make sure 2 devices will sync*/
///* the closest the random delays are, the longer it will
//   take for the devices to sync when started simultaneously*/
//static int32_t random_delay;

//static uint8_t pingpong_test_flag=0;

//static void OnTxDone(void);

///**
//  * @brief Function to be executed on Radio Rx Done event
//  * @param  payload ptr of buffer received
//  * @param  size buffer size
//  * @param  rssi
//  * @param  LoraSnr_FskCfo
//  */
//static void OnRxDone(uint8_t *payload, uint16_t size, int16_t rssi, int8_t LoraSnr_FskCfo);

///**
//  * @brief Function executed on Radio Tx Timeout event
//  */
//static void OnTxTimeout(void);

///**
//  * @brief Function executed on Radio Rx Timeout event
//  */
//static void OnRxTimeout(void);

///**
//  * @brief Function executed on Radio Rx Error event
//  */
//static void OnRxError(void);

///* USER CODE BEGIN PFP */
//static void PingPong_Process(void);

///* External variables ---------------------------------------------------------*/
///* USER CODE BEGIN EV */

///* USER CODE END EV */

///* Private typedef -----------------------------------------------------------*/
///* USER CODE BEGIN PTD */

///* USER CODE END PTD */

///* Private define ------------------------------------------------------------*/
///* USER CODE BEGIN PD */

///* USER CODE END PD */

///* Private macro -------------------------------------------------------------*/
///* USER CODE BEGIN PM */

///* USER CODE END PM */

///* Private variables ---------------------------------------------------------*/

///* USER CODE BEGIN PV */

///* USER CODE END PV */

///* Private function prototypes -----------------------------------------------*/
///**
//  * @brief  Print an unsigned int
//  * @param  value to print
//  */
//static void print_u(uint32_t value);

///**
//  * @brief  Check if character in parameter is alphanumeric
//  * @param  Char for the alphanumeric check
//  */
//static int32_t isHex(char Char);

///**
//  * @brief  Converts hex string to a nibble ( 0x0X )
//  * @param  Char hex string
//  * @retval the nibble. Returns 0xF0 in case input was not an hex string (a..f; A..F or 0..9)
//  */
//static uint8_t Char2Nibble(char Char);

///**
//  * @brief  Convert a string into a buffer of data
//  * @param  str string to convert
//  * @param  data output buffer
//  * @param  Size of input string
//  */
//static int32_t stringToData(const char *str, uint8_t *data, uint32_t Size);

///* USER CODE BEGIN PFP */

///* USER CODE END PFP */

///* Exported functions --------------------------------------------------------*/
////ATEerror_t AT_return_ok(const char *param)
////{
////  return AT_OK;
////}

////ATEerror_t AT_return_error(const char *param)
////{
////  return AT_ERROR;
////}

///* --------------- General commands --------------- */


//void gpio_set_state(GPIO_TypeDef *GPIOx, uint16_t GPIO_Pin,GPIO_PinState PinState){

//	GPIO_InitTypeDef GPIO_InitStruct = {0};
//	
//	GPIO_InitStruct.Pin = GPIO_Pin;
//	GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
//	GPIO_InitStruct.Pull = GPIO_NOPULL;
//	HAL_GPIO_Init(GPIOx, &GPIO_InitStruct);
//	
//	HAL_GPIO_WritePin(GPIOx, GPIO_Pin, PinState);
//}
//#include "radio_driver.h"
//ATEerror_t AT_CHIPID_GET(const char *param)
//{
//	uint8_t i;
//	uint8_t mac[8];
//	uint8_t chip_id[12];

//	for(i=0;i<8;i++)
//	{
//		mac[i]			=	*(uint8_t *)(0x1FFF7590+i);
//		chip_id[i]	=	mac[i];
//	}
//	chip_id[8]=	*(uint8_t *)(0x1FFF7590+8);
//	chip_id[9]=	*(uint8_t *)(0x1FFF7590+9);
//	chip_id[10]=	*(uint8_t *)(0x1FFF7590+10);
//	chip_id[11]=	*(uint8_t *)(0x1FFF7590+11);
//	
//	mac[4] +=chip_id[8];
//	mac[5] +=chip_id[9];
//	mac[6] +=chip_id[10];
//	mac[7] +=chip_id[11];
//	
//	AT_PRINTF("MAC=%02x:%02x:%02x:%02x:%02x:%02x:%02x:%02x\r\n",
//											mac[0],mac[1],mac[2],mac[3],
//											mac[4],mac[5],mac[6],mac[7]);
//	
//  return AT_OK;
//  /* USER CODE BEGIN AT_test_stop_2 */

//  /* USER CODE END AT_test_stop_2 */
//}



//ATEerror_t AT_GPIO_TEST(const char *param)
//{
//	const char *buf = param;
//	const char io0_15_bit[4];
//	const char io16_31_bit[4];
//	const char io32_47_bit[4];
//	GPIO_TypeDef *GPIOx;
//	
//  char port_buf[3];
//	
//	if (3 != tiny_sscanf(buf, "0x%08x,0x%08x,0x%08x", &io0_15_bit, &io16_31_bit, &io32_47_bit))
//  {
//    return AT_PARAM_ERROR;
//  }
//	AT_PRINTF("param1[0-8bit]=0x%02x,[9-16bit]=0x%02x,[17-24bit]=0x%02x,[25-32bit]=0x%02x\r\n",io0_15_bit[0],io0_15_bit[1],io0_15_bit[2],io0_15_bit[3]);
//	AT_PRINTF("param2[0-8bit]=0x%02x,[9-16bit]=0x%02x,[17-24bit]=0x%02x,[25-32bit]=0x%02x\r\n",io16_31_bit[0],io16_31_bit[1],io16_31_bit[2],io16_31_bit[3]);
//	AT_PRINTF("param3[0-8bit]=0x%02x,[9-16bit]=0x%02x,[17-24bit]=0x%02x,[25-32bit]=0x%02x\r\n",io32_47_bit[0],io32_47_bit[1],io32_47_bit[2],io32_47_bit[3]);

//	int32_t cnt=0;
//	uint32_t IO_Pin;

//	__HAL_RCC_GPIOA_CLK_ENABLE();
//	__HAL_RCC_GPIOB_CLK_ENABLE();
//	__HAL_RCC_GPIOC_CLK_ENABLE();
//	
//	for(int32_t i=0;i<4;i++){
//		for(int32_t j=0;j<4;j++){
//			if(((io0_15_bit[i]) & ((0x02)<<(2*j))))
//			{	
//				if(((io0_15_bit[i]) & ((0x01)<<(2*j)))){
//						gpio_set_state(GPIOA,GPIO_PIN_0<<(cnt%16),1); //out high
//						AT_PRINTF("LORAGPIOPA%d Set output high\r\n",(cnt%16));
//				}else{
//						gpio_set_state(GPIOA,GPIO_PIN_0<<(cnt%16),0); //out low
//						AT_PRINTF("LORAGPIOPA%d Set output low\r\n",(cnt%16));
//				}
//			}else{
//				AT_PRINTF("CBB_GPIO%d NC\r\n",cnt);
//			}
//			cnt++;
//		}
//	}
//	for(int32_t i=0;i<4;i++){
//		for(int32_t j=0;j<4;j++){
//			if(((io16_31_bit[i]) & ((0x02)<<(2*j))))
//			{
//				if(((io16_31_bit[i]) & ((0x01)<<(2*j)))){
//						gpio_set_state(GPIOB,GPIO_PIN_0<<(cnt%16),1); //PB0-15 out high
//						AT_PRINTF("LORAGPIOPB%d Set output high\r\n",(cnt%16));
//				}else{
//					gpio_set_state(GPIOB,GPIO_PIN_0<<(cnt%16),0); //PB0-15 out low
//					AT_PRINTF("LORAGPIOPB%d Set output low\r\n",(cnt%16));
//				}
//			}else{
//				AT_PRINTF("CBB_GPIO%d NC\r\n",cnt);
//			}
//			cnt++;
//		}
//	}
//	for(int32_t i=0;i<4;i++){
//		for(int32_t j=0;j<4;j++){
//			if(((io32_47_bit[i]) & ((0x02)<<(2*j))))
//			{
//				if(((io32_47_bit[i]) & ((0x01)<<(2*j)))){
//					gpio_set_state(GPIOC,GPIO_PIN_0<<(cnt%16),1); //PC0-15 out high
//					AT_PRINTF("LORAGPIOPC%d Set output high\r\n",(cnt%16));
//				}else{
//					gpio_set_state(GPIOC,GPIO_PIN_0<<(cnt%16),0); //PC0-15 out low
//					AT_PRINTF("LORAGPIOPC%d Set output low\r\n",(cnt%16));
//				}
//			}else{
//				AT_PRINTF("CBB_GPIO%d NC\r\n",cnt);
//			}
//			cnt++;
//		}
//	}

//  return AT_OK;
//  /* USER CODE BEGIN AT_test_stop_2 */

//  /* USER CODE END AT_test_stop_2 */
//}


///* USER CODE BEGIN EF */

//ATEerror_t AT_PINGPONG_TEST(const char *param)
//{
//	const char *buf = param;
//	int mode;
//	int fre;
//	int out_power;
//	if (3 != tiny_sscanf(buf, "%u,%u,%u", &mode, &fre,&out_power))
//  {
//    return AT_PARAM_ERROR;
//  }
//	if(mode==1){
//			isMaster=true;
//	}else if(mode==0){
//			isMaster=false;
//	}else{
//			return AT_PARAM_ERROR;
//	}
//	
//	if(out_power<2 || out_power >22){
//			return AT_PARAM_ERROR;
//	}
//	
//  /* USER CODE BEGIN AT_test_stop_1 */
//	
//  /* USER CODE END AT_test_stop_1 */
//	if(pingpong_test_flag==0){
//		AT_PRINTF("PINGPONG Test Start\r\n");
//		/* USER CODE END SubghzApp_Init_1 */

//		/* Radio initialization */
//		RadioEvents.TxDone = OnTxDone;
//		RadioEvents.RxDone = OnRxDone;
//		RadioEvents.TxTimeout = OnTxTimeout;
//		RadioEvents.RxTimeout = OnRxTimeout;
//		RadioEvents.RxError = OnRxError;

//		Radio.Init(&RadioEvents);

//		/* USER CODE BEGIN SubghzApp_Init_2 */
//		/*calculate random delay for synchronization*/
//		random_delay = (Radio.Random()) >> 22; /*10bits random e.g. from 0 to 1023 ms*/

//		/* Radio Set frequency */
//		Radio.SetChannel(fre);

//		/* Radio configuration */
//	#if ((USE_MODEM_LORA == 1) && (USE_MODEM_FSK == 0))
//		APP_LOG(TS_OFF, VLEVEL_M, "---------------\n\r");
//		APP_LOG(TS_OFF, VLEVEL_M, "LORA_MODULATION\n\r");
//		APP_LOG(TS_OFF, VLEVEL_M, "LORA_BW=%d kHz\n\r", (1 << LORA_BANDWIDTH) * 125);
//		APP_LOG(TS_OFF, VLEVEL_M, "LORA_SF=%d\n\r", LORA_SPREADING_FACTOR);
//		APP_LOG(TS_OFF, VLEVEL_M, "LORA_TX_OUTPUT_POWER=%d\n\r", out_power);
//		
//		Radio.SetTxConfig(MODEM_LORA, out_power, 0, LORA_BANDWIDTH,
//											LORA_SPREADING_FACTOR, LORA_CODINGRATE,
//											LORA_PREAMBLE_LENGTH, LORA_FIX_LENGTH_PAYLOAD_ON,
//											true, 0, 0, LORA_IQ_INVERSION_ON, TX_TIMEOUT_VALUE);

//		Radio.SetRxConfig(MODEM_LORA, LORA_BANDWIDTH, LORA_SPREADING_FACTOR,
//											LORA_CODINGRATE, 0, LORA_PREAMBLE_LENGTH,
//											LORA_SYMBOL_TIMEOUT, LORA_FIX_LENGTH_PAYLOAD_ON,
//											0, true, 0, 0, LORA_IQ_INVERSION_ON, true);

//		Radio.SetMaxPayloadLength(MODEM_LORA, MAX_APP_BUFFER_SIZE);

//	#elif ((USE_MODEM_LORA == 0) && (USE_MODEM_FSK == 1))
//		APP_LOG(TS_OFF, VLEVEL_M, "---------------\n\r");
//		APP_LOG(TS_OFF, VLEVEL_M, "FSK_MODULATION\n\r");
//		APP_LOG(TS_OFF, VLEVEL_M, "FSK_BW=%d Hz\n\r", FSK_BANDWIDTH);
//		APP_LOG(TS_OFF, VLEVEL_M, "FSK_DR=%d bits/s\n\r", FSK_DATARATE);

//		Radio.SetTxConfig(MODEM_FSK, TX_OUTPUT_POWER, FSK_FDEV, 0,
//											FSK_DATARATE, 0,
//											FSK_PREAMBLE_LENGTH, FSK_FIX_LENGTH_PAYLOAD_ON,
//											true, 0, 0, 0, TX_TIMEOUT_VALUE);

//		Radio.SetRxConfig(MODEM_FSK, FSK_BANDWIDTH, FSK_DATARATE,
//											0, FSK_AFC_BANDWIDTH, FSK_PREAMBLE_LENGTH,
//											0, FSK_FIX_LENGTH_PAYLOAD_ON, 0, true,
//											0, 0, false, true);

//		Radio.SetMaxPayloadLength(MODEM_FSK, MAX_APP_BUFFER_SIZE);

//	#else
//	#error "Please define a modulation in the subghz_phy_app.h file."
//	#endif /* USE_MODEM_LORA | USE_MODEM_FSK */

//		/*fills tx buffer*/
//		memset(BufferTx, 0x0, MAX_APP_BUFFER_SIZE);

//		APP_LOG(TS_ON, VLEVEL_L, "rand=%d\n\r", random_delay);
//		/*starts reception*/
//		Radio.Rx(RX_TIMEOUT_VALUE + random_delay);

//		/*register task to to be run in while(1) after Radio IT*/
//		UTIL_SEQ_RegTask((1 << CFG_SEQ_Task_SubGHz_Phy_App_Process), UTIL_SEQ_RFU, PingPong_Process);
//		
//		pingpong_test_flag=1;
//		/* USER CODE END SubghzApp_Init_2 */
//		
//		/* USER CODE END SubghzApp_Init_2 */
//		
//		return AT_OK;
//	}
//	AT_PRINTF(" Error!! PINGPONG Test already Start\r\n");
//	return AT_ERROR;
//}

///* USER CODE END EF */

///* Private Functions Definition -----------------------------------------------*/
//static void print_u(uint32_t value)
//{
//  /* USER CODE BEGIN print_u_1 */

//  /* USER CODE END print_u_1 */
//  AT_PRINTF("%u\r\n", value);
//  /* USER CODE BEGIN print_u_2 */

//  /* USER CODE END print_u_2 */
//}

//static uint8_t Char2Nibble(char Char)
//{
//  if (((Char >= '0') && (Char <= '9')))
//  {
//    return Char - '0';
//  }
//  else if (((Char >= 'a') && (Char <= 'f')))
//  {
//    return Char - 'a' + 10;
//  }
//  else if ((Char >= 'A') && (Char <= 'F'))
//  {
//    return Char - 'A' + 10;
//  }
//  else
//  {
//    return 0xF0;
//  }
//  /* USER CODE BEGIN CertifSend_2 */

//  /* USER CODE END CertifSend_2 */
//}

//static int32_t stringToData(const char *str, uint8_t *data, uint32_t Size)
//{
//  /* USER CODE BEGIN stringToData_1 */

//  /* USER CODE END stringToData_1 */
//  char hex[3];
//  hex[2] = 0;
//  int32_t ii = 0;
//  while (Size-- > 0)
//  {
//    hex[0] = *str++;
//    hex[1] = *str++;

//    /*check if input is hex */
//    if ((isHex(hex[0]) == -1) || (isHex(hex[1]) == -1))
//    {
//      return -1;
//    }
//    /*check if input is even nb of character*/
//    if ((hex[1] == '\0') || (hex[1] == ','))
//    {
//      return -1;
//    }
//    data[ii] = (Char2Nibble(hex[0]) << 4) + Char2Nibble(hex[1]);

//    ii++;
//  }

//  return 0;
//  /* USER CODE BEGIN stringToData_2 */

//  /* USER CODE END stringToData_2 */
//}

//unsigned char hex2int(char c)
//{
//    if (c >= '0' && c <= '9') {
//        return (unsigned char )(c - 48);
//    } else if (c >= 'A' && c <= 'F') {
//        return (unsigned char )(c - 55);
//    } else if (c >= 'a' && c <= 'f') {
//        return (unsigned char )(c - 87);
//    } else {
//        return 0;
//    }
//}
// 
//static void hex2str(char *hex, char *str)
//{
//     int i = 0 ;
//     for (int j = 0; j < strlen(hex) - 1 ;) {
//        unsigned char a = hex2int(hex[j++]);
//        unsigned char b = hex2int(hex[j++]);
//        str[i++] = (char)(a*16 + b);
//    }
//    str[i] = '\0';
//}

//static int32_t isHex(char Char)
//{
//  /* USER CODE BEGIN isHex_1 */

//  /* USER CODE END isHex_1 */
//  if (((Char >= '0') && (Char <= '9')) ||
//      ((Char >= 'a') && (Char <= 'f')) ||
//      ((Char >= 'A') && (Char <= 'F')))
//  {
//    return 0;
//  }
//  else
//  {
//    return -1;
//  }
//  /* USER CODE BEGIN isHex_2 */

//  /* USER CODE END isHex_2 */
//}

//static void OnTxDone(void)
//{
//  /* USER CODE BEGIN OnTxDone */
//  APP_LOG(TS_ON, VLEVEL_L, "OnTxDone\n\r");
//  /* Update the State of the FSM*/
//  State = TX;
//  /* Run PingPong process in background*/
//  UTIL_SEQ_SetTask((1 << CFG_SEQ_Task_SubGHz_Phy_App_Process), CFG_SEQ_Prio_0);
//  /* USER CODE END OnTxDone */
//}


//static void OnRxDone(uint8_t *payload, uint16_t size, int16_t rssi, int8_t LoraSnr_FskCfo)
//{
//  /* USER CODE BEGIN OnRxDone */
//  APP_LOG(TS_ON, VLEVEL_L, "OnRxDone\n\r");
//	
//#if ((USE_MODEM_LORA == 1) && (USE_MODEM_FSK == 0))
////  APP_LOG(TS_ON, VLEVEL_L, "RssiValue=%d dBm, SnrValue=%ddB\n\r", rssi, LoraSnr_FskCfo);
//  /* Record payload Signal to noise ratio in Lora*/
//  SnrValue = LoraSnr_FskCfo;
//#endif /* USE_MODEM_LORA | USE_MODEM_FSK */
//	
//#if ((USE_MODEM_LORA == 0) && (USE_MODEM_FSK == 1))
//  APP_LOG(TS_ON, VLEVEL_L, "RssiValue=%d dBm, Cfo=%dkHz\n\r", rssi, LoraSnr_FskCfo);
//  SnrValue = 0; /*not applicable in GFSK*/
//#endif /* USE_MODEM_LORA | USE_MODEM_FSK */
//  /* Update the State of the FSM*/
//  State = RX;
//  /* Clear BufferRx*/
//  memset(BufferRx, 0, MAX_APP_BUFFER_SIZE);
//  /* Record payload size*/
//  RxBufferSize = size;
//  if (RxBufferSize <= MAX_APP_BUFFER_SIZE)
//  {
//    memcpy(BufferRx, payload, RxBufferSize);
//  }
//  /* Record Received Signal Strength*/
//  RssiValue = rssi;

//	if(isMaster==1){
//		tx_RssiValue=rssi;
//		APP_LOG(TS_ON, VLEVEL_L, "tx_RssiValue=%d dBm, SnrValue=%ddB\n\r", rssi, LoraSnr_FskCfo);
//	}else{
//		int rssi_value[4];
//		int cnt =0;
//		tx_RssiValue=0;
//		rx_RssiValue=rssi;
//		
//		APP_LOG(TS_ON, VLEVEL_H, "payload. size=%d \n\r", size);
//		for (int32_t i = 0; i < PAYLOAD_LEN; i++)
//		{
//			APP_LOG(TS_OFF, VLEVEL_H, "%02X", BufferRx[i]);
//			if(BufferRx[i]>=0x30 && BufferRx[i]<=0x39){
//					rssi_value[cnt]=hex2int(BufferRx[i]);
//					cnt++;
//			}
//			if (i % 16 == 15)
//			{
//				APP_LOG(TS_OFF, VLEVEL_H, "\n\r");
//			}
//		}
//		if(cnt==1){
//			tx_RssiValue=rssi_value[cnt-1];
//		}else if(cnt==2){
//			tx_RssiValue=rssi_value[cnt-2]*10+rssi_value[cnt-1];
//		}else if(cnt==3){
//			tx_RssiValue=rssi_value[cnt-3]*100+rssi_value[cnt-2]*10+rssi_value[cnt-1];
//		}
//		APP_LOG(TS_OFF, VLEVEL_H, "\n\r");
//		APP_LOG(TS_ON, VLEVEL_L, "rx_rssi =%d dBm,tx_rssi=-%d dBm,SnrValue=%ddB\n\r",rx_RssiValue,tx_RssiValue,LoraSnr_FskCfo);
//	}
//  /* Run PingPong process in background*/
//  UTIL_SEQ_SetTask((1 << CFG_SEQ_Task_SubGHz_Phy_App_Process), CFG_SEQ_Prio_0);
//  /* USER CODE END OnRxDone */
//}


//static void OnTxTimeout(void)
//{
//  /* USER CODE BEGIN OnTxTimeout */
//  APP_LOG(TS_ON, VLEVEL_L, "OnTxTimeout\n\r");
//  /* Update the State of the FSM*/
//  State = TX_TIMEOUT;
//  /* Run PingPong process in background*/
//  UTIL_SEQ_SetTask((1 << CFG_SEQ_Task_SubGHz_Phy_App_Process), CFG_SEQ_Prio_0);
//  /* USER CODE END OnTxTimeout */
//}

//static void OnRxTimeout(void)
//{
//  /* USER CODE BEGIN OnRxTimeout */
//  APP_LOG(TS_ON, VLEVEL_L, "OnRxTimeout\n\r");
//  /* Update the State of the FSM*/
//  State = RX_TIMEOUT;
//  /* Run PingPong process in background*/
//  UTIL_SEQ_SetTask((1 << CFG_SEQ_Task_SubGHz_Phy_App_Process), CFG_SEQ_Prio_0);
//  /* USER CODE END OnRxTimeout */
//}

//static void OnRxError(void)
//{
//  /* USER CODE BEGIN OnRxError */
//  APP_LOG(TS_ON, VLEVEL_L, "OnRxError\n\r");
//  /* Update the State of the FSM*/
//  State = RX_ERROR;
//  /* Run PingPong process in background*/
//  UTIL_SEQ_SetTask((1 << CFG_SEQ_Task_SubGHz_Phy_App_Process), CFG_SEQ_Prio_0);
//  /* USER CODE END OnRxError */
//}

///* USER CODE BEGIN PrFD */

////static void PingPong_Process(void)
////{
////  Radio.Sleep();

////  switch (State)
////  {
////    case RX:

////		if (RxBufferSize > 0)
////		{
////			if (strncmp((const char *)BufferRx, PING, sizeof(PING) - 1) == 0)
////			{
////				static uint8_t tx_rssi=0;
////				UTIL_TIMER_Stop(&timerLed);
////				/* switch off red led */
////				/* Add delay between RX and TX */
////				HAL_Delay(Radio.GetWakeupTime() + RX_TIME_MARGIN);
////				/*slave sends PONG*/
////				APP_LOG(TS_ON, VLEVEL_L, "..."
////								"PONG"
////								"\n\r"); 
////				//APP_LOG(TS_ON, VLEVEL_L, "Slave  Tx start\n\r");
////				memcpy(BufferTx, PONG, sizeof(PONG) - 1);
////				Radio.Send(BufferTx, PAYLOAD_LEN);
////			}
////			else /* valid reception but not a PING as expected */
////			{
////				/* Set device as master and start again */
////				APP_LOG(TS_ON, VLEVEL_L, "Master Rx start\n\r");
////				Radio.Rx(RX_TIMEOUT_VALUE);
////			}
////		}
////      
////      break;
////    case TX:
////      APP_LOG(TS_ON, VLEVEL_L, "Rx start\n\r");
////      Radio.Rx(RX_TIMEOUT_VALUE);
////      break;
////    case RX_TIMEOUT:
////    case RX_ERROR:
////			APP_LOG(TS_ON, VLEVEL_L, "Slave Rx start\n\r");
////			Radio.Rx(RX_TIMEOUT_VALUE);
////      break;
////    case TX_TIMEOUT:
////      APP_LOG(TS_ON, VLEVEL_L, "Slave Rx start\n\r");
////      Radio.Rx(RX_TIMEOUT_VALUE);
////      break;
////    default:
////      break;
////  }
////}

//static void PingPong_Process(void)
//{
//  Radio.Sleep();

//  switch (State)
//  {
//    case RX:

//      if (isMaster == true)
//      {
//        if (RxBufferSize > 0)
//        {
//          if (strncmp((const char *)BufferRx, PONG, sizeof(PONG) - 1) == 0)
//          {
//            UTIL_TIMER_Stop(&timerLed);
//            /* switch off green led */
//            /* Add delay between RX and TX */
//            HAL_Delay(Radio.GetWakeupTime() + RX_TIME_MARGIN);
//            /* master sends PING*/
//            APP_LOG(TS_ON, VLEVEL_L, "..."
//                    "PING"
//                    "\n\r");
//						sprintf((char*)BufferTx,"PING+%d",tx_RssiValue);
//            APP_LOG(TS_ON, VLEVEL_L, "Master Tx start\n\r");
//            //memcpy(BufferTx, PING, sizeof(PING) - 1);
//            Radio.Send(BufferTx, PAYLOAD_LEN);
//          }
//          else /* valid reception but neither a PING or a PONG message */
//          {
//            /* Set device as master and start again */
//						
//            APP_LOG(TS_ON, VLEVEL_L, "Master Rx start\n\r");
//            Radio.Rx(RX_TIMEOUT_VALUE);
//          }
//        }
//      }
//      else
//      {
//        if (RxBufferSize > 0)
//        {
//          if (strncmp((const char *)BufferRx, PING, sizeof(PING) - 1) == 0)
//          {
//            UTIL_TIMER_Stop(&timerLed);
//            /* switch off red led */
//            /* Add delay between RX and TX */
//            HAL_Delay(Radio.GetWakeupTime() + RX_TIME_MARGIN);
//            /*slave sends PONG*/
//            APP_LOG(TS_ON, VLEVEL_L, "..."
//                    "PONG"
//                    "\n\r");
//            APP_LOG(TS_ON, VLEVEL_L, "Slave  Tx start\n\r");
//            memcpy(BufferTx, PONG, sizeof(PONG) - 1);
//            Radio.Send(BufferTx, PAYLOAD_LEN);
//          }
//        }
//      }
//      break;
//    case TX:
//      APP_LOG(TS_ON, VLEVEL_L, "Rx start\n\r");
//      Radio.Rx(RX_TIMEOUT_VALUE);
//      break;
//    case RX_TIMEOUT:
//    case RX_ERROR:
//      if (isMaster == true)
//      {
//        /* Send the next PING frame */
//        /* Add delay between RX and TX*/
//        /* add random_delay to force sync between boards after some trials*/
//        HAL_Delay(Radio.GetWakeupTime() + RX_TIME_MARGIN + random_delay);
//        APP_LOG(TS_ON, VLEVEL_L, "Master Tx start\n\r");
//        /* master sends PING*/
//        memcpy(BufferTx, PING, sizeof(PING) - 1);
//        Radio.Send(BufferTx, PAYLOAD_LEN);
//      }
//      else
//      {
//        APP_LOG(TS_ON, VLEVEL_L, "Slave Rx start\n\r");
//        Radio.Rx(RX_TIMEOUT_VALUE);
//      }
//      break;
//    case TX_TIMEOUT:
//      APP_LOG(TS_ON, VLEVEL_L, "Slave Rx start\n\r");
//      Radio.Rx(RX_TIMEOUT_VALUE);
//      break;
//    default:
//      break;
//  }
//}

///* USER CODE END PrFD */


///* USER CODE BEGIN PrFD */

///* USER CODE END PrFD */
