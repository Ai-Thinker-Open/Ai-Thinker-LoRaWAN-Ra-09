/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file    lora_at.c
  * @author  MCD Application Team
  * @brief   AT command API
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2023 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */
/* USER CODE END Header */

/* Includes ------------------------------------------------------------------*/
#include "platform.h"
#include "lora_at.h"
#include "sys_app.h"
#include "stm32_tiny_sscanf.h"
#include "app_version.h"
#include "lorawan_version.h"
#include "subghz_phy_version.h"
#include "test_rf.h"
#include "adc_if.h"
#include "stm32_seq.h"
#include "utilities_def.h"
#include "radio.h"
#include "lora_info.h"
#include "flash_if.h"

/* USER CODE BEGIN Includes */
#include "RegionCN470.h"
#include "LoRaMac.h"
/* USER CODE END Includes */

/* External variables ---------------------------------------------------------*/
/* USER CODE BEGIN EV */

/* USER CODE END EV */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/*!
 * User application data buffer size
 */
#define LORAWAN_APP_DATA_BUFFER_MAX_SIZE            242

///*---------------------------------------------------------------------------*/
///*                             LoRaWAN NVM configuration                     */
///*---------------------------------------------------------------------------*/
///**
//  * @brief LoRaWAN NVM Flash address
//  * @note last 2 sector of a 128kBytes device
//  */
//#define LORAWAN_NVM_BASE_ADDRESS                    ((void *)0x0801F000UL)
//	
//#define PINGPONGMODE_NVM_BASE_ADDRESS               ((void *)0x0801F800UL)

/* USER CODE BEGIN PD */

/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
static bool ClassBEnableRequest = false;

/*!
 * User application buffer
 */
static uint8_t AppDataBuffer[LORAWAN_APP_DATA_BUFFER_MAX_SIZE];

/*!
 * User application data structure
 */
static LmHandlerAppData_t AppData = { 0, 0, AppDataBuffer };

/* Dummy data sent periodically to let the tester respond with start test command*/
static UTIL_TIMER_Object_t TxCertifTimer;

/* USER CODE BEGIN PV */

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
/**
  * @brief  Get 4 bytes values in hex
  * @param  from string containing the 16 bytes, something like ab:cd:01:21
  * @param  value buffer that will contain the bytes read
  * @retval The number of bytes read
  */
static int32_t sscanf_uint32_as_hhx(const char *from, uint32_t *value);

/**
  * @brief  Get 16 bytes values in hex
  * @param  from string containing the 16 bytes, something like ab:cd:01:...
  * @param  pt buffer that will contain the bytes read
  * @retval The number of bytes read
  */
static int32_t sscanf_16_hhx(const char *from, uint8_t *pt);

/**
  * @brief  Print 4 bytes as %02x
  * @param  value containing the 4 bytes to print
  */
static void print_uint32_as_02x(uint32_t value);

/**
  * @brief  Print 16 bytes as %02X
  * @param  pt pointer containing the 16 bytes to print
  */
static void print_16_02x(uint8_t *pt);

/**
  * @brief  Print 8 bytes as %02X
  * @param  pt pointer containing the 8 bytes to print
  */
static void print_8_02x(uint8_t *pt);

/**
  * @brief  Print an int
  * @param  value to print
  */
static void print_d(int32_t value);

/**
  * @brief  Print an unsigned int
  * @param  value to print
  */
static void print_u(uint32_t value);

/**
  * @brief  Certif Rejoin timer callback function
  * @param  context ptr of Certif Rejoin context
  */
static void OnCertifTimer(void *context);

/**
  * @brief  Certif send function
  */
static void CertifSend(void);

/**
  * @brief  Check if character in parameter is alphanumeric
  * @param  Char for the alphanumeric check
  */
static int32_t isHex(char Char);

/**
  * @brief  Converts hex string to a nibble ( 0x0X )
  * @param  Char hex string
  * @retval the nibble. Returns 0xF0 in case input was not an hex string (a..f; A..F or 0..9)
  */
static uint8_t Char2Nibble(char Char);

/**
  * @brief  Convert a string into a buffer of data
  * @param  str string to convert
  * @param  data output buffer
  * @param  Size of input string
  */
static int32_t stringToData(const char *str, uint8_t *data, uint32_t Size);

/* USER CODE BEGIN PFP */

/* USER CODE END PFP */

/* Exported functions --------------------------------------------------------*/
ATEerror_t AT_return_ok(const char *param)
{
  return AT_OK;
}

ATEerror_t AT_return_error(const char *param)
{
  return AT_ERROR;
}

/* --------------- Application events --------------- */
void AT_event_join(LmHandlerJoinParams_t *params)
{
  /* USER CODE BEGIN AT_event_join_1 */

  /* USER CODE END AT_event_join_1 */
  if ((params != NULL) && (params->Status == LORAMAC_HANDLER_SUCCESS))
  {
    AT_PRINTF("+EVT:JOINED\r\n");
  }
  else
  {
    AT_PRINTF("+EVT:JOIN FAILED\r\n");
  }
  /* USER CODE BEGIN AT_event_join_2 */

  /* USER CODE END AT_event_join_2 */
}

void AT_event_receive(LmHandlerAppData_t *appData, LmHandlerRxParams_t *params)
{
  /* USER CODE BEGIN AT_event_receive_1 */

  /* USER CODE END AT_event_receive_1 */
  const char *slotStrings[] = { "1", "2", "C", "C_MC", "P", "P_MC" };
  uint8_t ReceivedDataSize = 0;
  uint8_t RxPort = 0;

  if (appData != NULL)
  {
    RxPort = appData->Port;
    if ((appData->Buffer != NULL) && (appData->BufferSize > 0))
    {
      /* Received data to be copied*/
      if (LORAWAN_APP_DATA_BUFFER_MAX_SIZE <= appData->BufferSize)
      {
        ReceivedDataSize = LORAWAN_APP_DATA_BUFFER_MAX_SIZE;
      }
      else
      {
        ReceivedDataSize = appData->BufferSize;
      }

      /*asynchronous notification to the host*/
      AT_PRINTF("+EVT:%d:%02X:", appData->Port, ReceivedDataSize);

      for (uint8_t i = 0; i < ReceivedDataSize; i++)
      {
        AT_PRINTF("%02X", appData->Buffer[i]);
      }
      AT_PRINTF("\r\n");
    }
  }

  if ((params != NULL) && (params->RxSlot < RX_SLOT_NONE))
  {
    AT_PRINTF("+EVT:RX_%s, PORT %d, DR %d, RSSI %d, SNR %d", slotStrings[params->RxSlot], RxPort,
              params->Datarate, params->Rssi, params->Snr);
    if (params->LinkCheck == true)
    {
      AT_PRINTF(", DMODM %d, GWN %d", params->DemodMargin, params->NbGateways);
    }
    AT_PRINTF("\r\n");
  }

  /* USER CODE BEGIN AT_event_receive_2 */

  /* USER CODE END AT_event_receive_2 */
}

void AT_event_confirm(LmHandlerTxParams_t *params)
{
  /* USER CODE BEGIN AT_event_confirm_1 */

  /* USER CODE END AT_event_confirm_1 */
  if ((params != NULL) && (params->MsgType == LORAMAC_HANDLER_CONFIRMED_MSG) && (params->AckReceived != 0))
  {
    AT_PRINTF("+EVT:SEND_CONFIRMED\r\n");
  }
  /* USER CODE BEGIN AT_event_confirm_2 */

  /* USER CODE END AT_event_confirm_2 */
}

void AT_event_ClassUpdate(DeviceClass_t deviceClass)
{
  /* USER CODE BEGIN AT_event_ClassUpdate_1 */

  /* USER CODE END AT_event_ClassUpdate_1 */
  AT_PRINTF("+EVT:SWITCH_TO_CLASS_%c\r\n", "ABC"[deviceClass]);
  /* USER CODE BEGIN AT_event_ClassUpdate_2 */

  /* USER CODE END AT_event_ClassUpdate_2 */
}

void AT_event_Beacon(LmHandlerBeaconParams_t *params)
{
  /* USER CODE BEGIN AT_event_Beacon_1 */

  /* USER CODE END AT_event_Beacon_1 */
  if (params != NULL)
  {
    switch (params->State)
    {
      default:
      case LORAMAC_HANDLER_BEACON_LOST:
      {
        AT_PRINTF("+EVT:BEACON_LOST\r\n");
        break;
      }
      case LORAMAC_HANDLER_BEACON_RX:
      {
        AT_PRINTF("+EVT:RX_BC, DR %d, RSSI %d, SNR %d, FQ %d, TIME %d, DESC %d, "
                  "INFO %02X%02X%02X,%02X%02X%02X\r\n",
                  params->Info.Datarate, params->Info.Rssi, params->Info.Snr, params->Info.Frequency,
                  params->Info.Time.Seconds, params->Info.GwSpecific.InfoDesc,
                  params->Info.GwSpecific.Info[0], params->Info.GwSpecific.Info[1],
                  params->Info.GwSpecific.Info[2], params->Info.GwSpecific.Info[3],
                  params->Info.GwSpecific.Info[4], params->Info.GwSpecific.Info[5]);
        break;
      }
      case LORAMAC_HANDLER_BEACON_NRX:
      {
        AT_PRINTF("+EVT:BEACON_NOT_RECEIVED\r\n");
        break;
      }
    }
  }
  /* USER CODE BEGIN AT_event_Beacon_2 */

  /* USER CODE END AT_event_Beacon_2 */
}

void AT_event_OnNvmDataChange(LmHandlerNvmContextStates_t state)
{
  if (state == LORAMAC_HANDLER_NVM_STORE)
  {
    AT_PRINTF("NVM DATA STORED\r\n");
  }
  else
  {
    AT_PRINTF("NVM DATA RESTORED\r\n");
  }
}

void AT_event_OnStoreContextRequest(void *nvm, uint32_t nvm_size)
{
  /* store nvm in flash */
  if (FLASH_IF_Erase(LORAWAN_NVM_BASE_ADDRESS, FLASH_PAGE_SIZE) == FLASH_IF_OK)
  {
    FLASH_IF_Write(LORAWAN_NVM_BASE_ADDRESS, (const void *)nvm, nvm_size);
  }
}

void AT_event_OnRestoreContextRequest(void *nvm, uint32_t nvm_size)
{
  FLASH_IF_Read(nvm, LORAWAN_NVM_BASE_ADDRESS, nvm_size);
}

/* --------------- General commands --------------- */
ATEerror_t AT_version_get(const char *param)
{
  /* USER CODE BEGIN AT_version_get_1 */

  /* USER CODE END AT_version_get_1 */
  uint32_t feature_version;

  /* Get LoRa APP version*/
  AT_PRINTF("APPLICATION_VERSION: V%X.%X.%X\r\n",
            (uint8_t)(APP_VERSION_MAIN),
            (uint8_t)(APP_VERSION_SUB1),
            (uint8_t)(APP_VERSION_SUB2));

  /* Get MW LoraWAN info */
  AT_PRINTF("MW_LORAWAN_VERSION:  V%X.%X.%X\r\n",
            (uint8_t)(LORAWAN_VERSION_MAIN),
            (uint8_t)(LORAWAN_VERSION_SUB1),
            (uint8_t)(LORAWAN_VERSION_SUB2));

  /* Get MW SubGhz_Phy info */
  AT_PRINTF("MW_RADIO_VERSION:    V%X.%X.%X\r\n",
            (uint8_t)(SUBGHZ_PHY_VERSION_MAIN),
            (uint8_t)(SUBGHZ_PHY_VERSION_SUB1),
            (uint8_t)(SUBGHZ_PHY_VERSION_SUB2));

  /* Get LoraWAN Link Layer info */
  LmHandlerGetVersion(LORAMAC_HANDLER_L2_VERSION, &feature_version);
  AT_PRINTF("L2_SPEC_VERSION:     V%X.%X.%X\r\n",
            (uint8_t)(feature_version >> 24),
            (uint8_t)(feature_version >> 16),
            (uint8_t)(feature_version >> 8));

  /* Get LoraWAN Regional Parameters info */
  LmHandlerGetVersion(LORAMAC_HANDLER_REGION_VERSION, &feature_version);
  AT_PRINTF("RP_SPEC_VERSION:     V%X-%X.%X.%X\r\n",
            (uint8_t)(feature_version >> 24),
            (uint8_t)(feature_version >> 16),
            (uint8_t)(feature_version >> 8),
            (uint8_t)(feature_version));

  return AT_OK;
  /* USER CODE BEGIN AT_version_get_2 */

  /* USER CODE END AT_version_get_2 */
}


ATEerror_t AT_verbose_get(const char *param)
{
  /* USER CODE BEGIN AT_verbose_get_1 */

  /* USER CODE END AT_verbose_get_1 */
  print_u(UTIL_ADV_TRACE_GetVerboseLevel());
  return AT_OK;
  /* USER CODE BEGIN AT_verbose_get_2 */

  /* USER CODE END AT_verbose_get_2 */
}

ATEerror_t AT_verbose_set(const char *param)
{
  /* USER CODE BEGIN AT_verbose_set_1 */

  /* USER CODE END AT_verbose_set_1 */
  const char *buf = param;
  int32_t lvl_nb;

  /* read and set the verbose level */
  if (1 != tiny_sscanf(buf, "%u", &lvl_nb))
  {
    AT_PRINTF("AT+VL: verbose level is not well set\r\n");
    return AT_PARAM_ERROR;
  }
  if ((lvl_nb > VLEVEL_H) || (lvl_nb < VLEVEL_OFF))
  {
    AT_PRINTF("AT+VL: verbose level out of range => 0(VLEVEL_OFF) to 3(VLEVEL_H)\r\n");
    return AT_PARAM_ERROR;
  }

  UTIL_ADV_TRACE_SetVerboseLevel(lvl_nb);

  return AT_OK;
  /* USER CODE BEGIN AT_verbose_set_2 */

  /* USER CODE END AT_verbose_set_2 */
}

ATEerror_t AT_LocalTime_get(const char *param)
{
  /* USER CODE BEGIN AT_LocalTime_get_1 */

  /* USER CODE END AT_LocalTime_get_1 */
  struct tm localtime;
  SysTime_t UnixEpoch = SysTimeGet();
  UnixEpoch.Seconds -= 18; /*removing leap seconds*/

  UnixEpoch.Seconds += 3600 * 2; /*adding 2 hours*/

  SysTimeLocalTime(UnixEpoch.Seconds,  & localtime);

  AT_PRINTF("LTIME:%02dh%02dm%02ds on %02d/%02d/%04d\r\n",
            localtime.tm_hour, localtime.tm_min, localtime.tm_sec,
            localtime.tm_mday, localtime.tm_mon + 1, localtime.tm_year + 1900);

  return AT_OK;
  /* USER CODE BEGIN AT_LocalTime_get_2 */

  /* USER CODE END AT_LocalTime_get_2 */
}

ATEerror_t AT_reset(const char *param)
{
  /* USER CODE BEGIN AT_reset_1 */

  /* USER CODE END AT_reset_1 */
  NVIC_SystemReset();
  /* USER CODE BEGIN AT_reset_2 */

  /* USER CODE END AT_reset_2 */
}

/* ---------------  Context Store commands --------------------------- */
ATEerror_t AT_restore_factory_settings(const char *param)
{
  /* USER CODE BEGIN AT_restore_factory_settings_1 */

  /* USER CODE END AT_restore_factory_settings_1 */
  /* store nvm in flash */
  if (FLASH_IF_Erase(LORAWAN_NVM_BASE_ADDRESS, FLASH_PAGE_SIZE) == FLASH_IF_OK)
  {
    /* System Reboot*/
    NVIC_SystemReset();
  }

  return AT_OK;
  /* USER CODE BEGIN AT_restore_factory_settings_2 */

  /* USER CODE END AT_restore_factory_settings_2 */
}

ATEerror_t AT_store_context(const char *param)
{
  /* USER CODE BEGIN AT_store_context_1 */

  /* USER CODE END AT_store_context_1 */
  LmHandlerErrorStatus_t status = LORAMAC_HANDLER_ERROR;

  status = LmHandlerNvmDataStore();

  if (status == LORAMAC_HANDLER_NVM_DATA_UP_TO_DATE)
  {
    AT_PRINTF("NVM DATA UP TO DATE\r\n");
  }
  else if (status == LORAMAC_HANDLER_ERROR)
  {
    AT_PRINTF("NVM DATA STORE FAILED\r\n");
    return AT_ERROR;
  }
  return AT_OK;
  /* USER CODE BEGIN AT_store_context_2 */

  /* USER CODE END AT_store_context_2 */
}

/* --------------- Keys, IDs and EUIs management commands --------------- */
ATEerror_t AT_JoinEUI_get(const char *param)
{
  /* USER CODE BEGIN AT_JoinEUI_get_1 */

  /* USER CODE END AT_JoinEUI_get_1 */
  uint8_t appEUI[8];
  if (LmHandlerGetAppEUI(appEUI) != LORAMAC_HANDLER_SUCCESS)
  {
    return AT_PARAM_ERROR;
  }

  print_8_02x(appEUI);
  return AT_OK;
  /* USER CODE BEGIN AT_JoinEUI_get_2 */

  /* USER CODE END AT_JoinEUI_get_2 */
}

ATEerror_t AT_JoinEUI_set(const char *param)
{
  /* USER CODE BEGIN AT_JoinEUI_set_1 */

  /* USER CODE END AT_JoinEUI_set_1 */
  uint8_t JoinEui[8];
  if (tiny_sscanf(param, "%hhx:%hhx:%hhx:%hhx:%hhx:%hhx:%hhx:%hhx",
                  &JoinEui[0], &JoinEui[1], &JoinEui[2], &JoinEui[3],
                  &JoinEui[4], &JoinEui[5], &JoinEui[6], &JoinEui[7]) != 8)
  {
    return AT_PARAM_ERROR;
  }

  if (LORAMAC_HANDLER_SUCCESS != LmHandlerSetAppEUI(JoinEui))
  {
    return AT_ERROR;
  }

  return AT_OK;
  /* USER CODE BEGIN AT_JoinEUI_set_2 */

  /* USER CODE END AT_JoinEUI_set_2 */
}

ATEerror_t AT_NwkKey_get(const char *param)
{
  /* USER CODE BEGIN AT_NwkKey_get_1 */

  /* USER CODE END AT_NwkKey_get_1 */
  uint8_t nwkKey[16];
  if (LORAMAC_HANDLER_SUCCESS != LmHandlerGetKey(NWK_KEY, nwkKey))
  {
    return AT_ERROR;
  }
  print_16_02x(nwkKey);

  return AT_OK;
  /* USER CODE BEGIN AT_NwkKey_get_2 */

  /* USER CODE END AT_NwkKey_get_2 */
}

ATEerror_t AT_NwkKey_set(const char *param)
{
  /* USER CODE BEGIN AT_NwkKey_set_1 */

  /* USER CODE END AT_NwkKey_set_1 */
  uint8_t nwkKey[16];
  if (sscanf_16_hhx(param, nwkKey) != 16)
  {
    return AT_PARAM_ERROR;
  }

  if (LORAMAC_HANDLER_SUCCESS != LmHandlerSetKey(NWK_KEY, nwkKey))
  {
    return AT_ERROR;
  }

  return AT_OK;
  /* USER CODE BEGIN AT_NwkKey_set_2 */

  /* USER CODE END AT_NwkKey_set_2 */
}

ATEerror_t AT_AppKey_get(const char *param)
{
  /* USER CODE BEGIN AT_AppKey_get_1 */

  /* USER CODE END AT_AppKey_get_1 */
  uint8_t appKey[16];
  if (LORAMAC_HANDLER_SUCCESS != LmHandlerGetKey(APP_KEY, appKey))
  {
    return AT_ERROR;
  }
  print_16_02x(appKey);

  return AT_OK;
  /* USER CODE BEGIN AT_AppKey_get_2 */

  /* USER CODE END AT_AppKey_get_2 */
}

ATEerror_t AT_AppKey_set(const char *param)
{
  /* USER CODE BEGIN AT_AppKey_set_1 */

  /* USER CODE END AT_AppKey_set_1 */
  uint8_t appKey[16];
  if (sscanf_16_hhx(param, appKey) != 16)
  {
    return AT_PARAM_ERROR;
  }

  if (LORAMAC_HANDLER_SUCCESS != LmHandlerSetKey(APP_KEY, appKey))
  {
    return AT_ERROR;
  }

  return AT_OK;
  /* USER CODE BEGIN AT_AppKey_set_2 */

  /* USER CODE END AT_AppKey_set_2 */
}

ATEerror_t AT_NwkSKey_get(const char *param)
{
  /* USER CODE BEGIN AT_NwkSKey_get_1 */

  /* USER CODE END AT_NwkSKey_get_1 */
  uint8_t nwkSKey[16];
  if (LORAMAC_HANDLER_SUCCESS != LmHandlerGetKey(NWK_S_KEY, nwkSKey))
  {
    return AT_ERROR;
  }
  print_16_02x(nwkSKey);

  return AT_OK;
  /* USER CODE BEGIN AT_NwkSKey_get_2 */

  /* USER CODE END AT_NwkSKey_get_2 */
}

ATEerror_t AT_NwkSKey_set(const char *param)
{
  /* USER CODE BEGIN AT_NwkSKey_set_1 */

  /* USER CODE END AT_NwkSKey_set_1 */
  uint8_t nwkSKey[16];
  if (sscanf_16_hhx(param, nwkSKey) != 16)
  {
    return AT_PARAM_ERROR;
  }

  if (LORAMAC_HANDLER_SUCCESS != LmHandlerSetKey(NWK_S_KEY, nwkSKey))
  {
    return AT_ERROR;
  }

  return AT_OK;
  /* USER CODE BEGIN AT_NwkSKey_set_2 */

  /* USER CODE END AT_NwkSKey_set_2 */
}

ATEerror_t AT_AppSKey_get(const char *param)
{
  /* USER CODE BEGIN AT_AppSKey_get_1 */

  /* USER CODE END AT_AppSKey_get_1 */
  uint8_t appSKey[16];
  if (LORAMAC_HANDLER_SUCCESS != LmHandlerGetKey(APP_S_KEY, appSKey))
  {
    return AT_ERROR;
  }
  print_16_02x(appSKey);

  return AT_OK;
  /* USER CODE BEGIN AT_AppSKey_get_2 */

  /* USER CODE END AT_AppSKey_get_2 */
}

ATEerror_t AT_AppSKey_set(const char *param)
{
  /* USER CODE BEGIN AT_AppSKey_set_1 */

  /* USER CODE END AT_AppSKey_set_1 */
  uint8_t appSKey[16];
  if (sscanf_16_hhx(param, appSKey) != 16)
  {
    return AT_PARAM_ERROR;
  }

  if (LORAMAC_HANDLER_SUCCESS != LmHandlerSetKey(APP_S_KEY, appSKey))
  {
    return AT_ERROR;
  }

  return AT_OK;
  /* USER CODE BEGIN AT_AppSKey_set_2 */

  /* USER CODE END AT_AppSKey_set_2 */
}

ATEerror_t AT_DevAddr_get(const char *param)
{
  /* USER CODE BEGIN AT_DevAddr_get_1 */

  /* USER CODE END AT_DevAddr_get_1 */
  uint32_t devAddr;
  if (LmHandlerGetDevAddr(&devAddr) != LORAMAC_HANDLER_SUCCESS)
  {
    return AT_PARAM_ERROR;
  }

  print_uint32_as_02x(devAddr);
  return AT_OK;
  /* USER CODE BEGIN AT_DevAddr_get_2 */

  /* USER CODE END AT_DevAddr_get_2 */
}

ATEerror_t AT_DevAddr_set(const char *param)
{
  /* USER CODE BEGIN AT_DevAddr_set_1 */

  /* USER CODE END AT_DevAddr_set_1 */
  uint32_t devAddr;
  if (sscanf_uint32_as_hhx(param, &devAddr) != 4)
  {
    return AT_PARAM_ERROR;
  }

  if (LORAMAC_HANDLER_SUCCESS != LmHandlerSetDevAddr(devAddr))
  {
    return AT_ERROR;
  }

  return AT_OK;
  /* USER CODE BEGIN AT_DevAddr_set_2 */

  /* USER CODE END AT_DevAddr_set_2 */
}

ATEerror_t AT_DevEUI_get(const char *param)
{
  /* USER CODE BEGIN AT_DevEUI_get_1 */

  /* USER CODE END AT_DevEUI_get_1 */
  uint8_t devEUI[8];
  if (LmHandlerGetDevEUI(devEUI) != LORAMAC_HANDLER_SUCCESS)
  {
    return AT_PARAM_ERROR;
  }

  print_8_02x(devEUI);
  return AT_OK;
  /* USER CODE BEGIN AT_DevEUI_get_2 */

  /* USER CODE END AT_DevEUI_get_2 */
}

ATEerror_t AT_DevEUI_set(const char *param)
{
  /* USER CODE BEGIN AT_DevEUI_set_1 */

  /* USER CODE END AT_DevEUI_set_1 */
  uint8_t devEui[8];
  if (tiny_sscanf(param, "%hhx:%hhx:%hhx:%hhx:%hhx:%hhx:%hhx:%hhx",
                  &devEui[0], &devEui[1], &devEui[2], &devEui[3],
                  &devEui[4], &devEui[5], &devEui[6], &devEui[7]) != 8)
  {
    return AT_PARAM_ERROR;
  }

  if (LORAMAC_HANDLER_SUCCESS != LmHandlerSetDevEUI(devEui))
  {
    return AT_ERROR;
  }

  return AT_OK;
  /* USER CODE BEGIN AT_DevEUI_set_2 */

  /* USER CODE END AT_DevEUI_set_2 */
}

ATEerror_t AT_NetworkID_get(const char *param)
{
  /* USER CODE BEGIN AT_NetworkID_get_1 */

  /* USER CODE END AT_NetworkID_get_1 */
  uint32_t networkId;
  if (LmHandlerGetNetworkID(&networkId) != LORAMAC_HANDLER_SUCCESS)
  {
    return AT_PARAM_ERROR;
  }

  print_d(networkId);
  return AT_OK;
  /* USER CODE BEGIN AT_NetworkID_get_2 */

  /* USER CODE END AT_NetworkID_get_2 */
}

ATEerror_t AT_NetworkID_set(const char *param)
{
  /* USER CODE BEGIN AT_NetworkID_set_1 */

  /* USER CODE END AT_NetworkID_set_1 */
  uint32_t networkId;
  if (tiny_sscanf(param, "%u", &networkId) != 1)
  {
    return AT_PARAM_ERROR;
  }

  if (networkId > 127)
  {
    return AT_PARAM_ERROR;
  }

  LmHandlerSetNetworkID(networkId);
  return AT_OK;
  /* USER CODE BEGIN AT_NetworkID_set_2 */

  /* USER CODE END AT_NetworkID_set_2 */
}

/* --------------- LoRaWAN join and send data commands --------------- */
ATEerror_t AT_Join(const char *param)
{
  /* USER CODE BEGIN AT_Join_1 */

  /* USER CODE END AT_Join_1 */
  switch (param[0])
  {
    case '0':
      LmHandlerJoin(ACTIVATION_TYPE_ABP, true);
      break;
    case '1':
      LmHandlerJoin(ACTIVATION_TYPE_OTAA, true);
      break;
    default:
      return AT_PARAM_ERROR;
  }

  return AT_OK;
  /* USER CODE BEGIN AT_Join_2 */

  /* USER CODE END AT_Join_2 */
}

ATEerror_t AT_Link_Check(const char *param)
{
  /* USER CODE BEGIN AT_Link_Check_1 */

  /* USER CODE END AT_Link_Check_1 */
  if (LmHandlerLinkCheckReq() != LORAMAC_HANDLER_SUCCESS)
  {
    return AT_PARAM_ERROR;
  }

  return AT_OK;
  /* USER CODE BEGIN AT_Link_Check_2 */

  /* USER CODE END AT_Link_Check_2 */
}

ATEerror_t AT_Send(const char *param)
{
  /* USER CODE BEGIN AT_Send_1 */

  /* USER CODE END AT_Send_1 */
  const char *buf = param;
  uint16_t bufSize = strlen(param);
  uint32_t appPort;
  LmHandlerMsgTypes_t isTxConfirmed;
  unsigned size = 0;
  char hex[3] = {0, 0, 0};
  LmHandlerErrorStatus_t lmhStatus = LORAMAC_HANDLER_ERROR;
  ATEerror_t status = AT_ERROR;

  /* read and set the application port */
  if (1 != tiny_sscanf(buf, "%u:", &appPort))
  {
    AT_PRINTF("AT+SEND without the application port\r\n");
    return AT_PARAM_ERROR;
  }

  /* skip the application port */
  while (('0' <= buf[0]) && (buf[0] <= '9') && bufSize > 1)
  {
    buf ++;
    bufSize --;
  };

  if ((bufSize == 0) || (':' != buf[0]))
  {
    AT_PRINTF("AT+SEND missing : character after app port\r\n");
    return AT_PARAM_ERROR;
  }
  else
  {
    /* skip the char ':' */
    buf ++;
    bufSize --;
  }

  switch (buf[0])
  {
    case '0':
      isTxConfirmed = LORAMAC_HANDLER_UNCONFIRMED_MSG;
      break;
    case '1':
      isTxConfirmed = LORAMAC_HANDLER_CONFIRMED_MSG;
      break;
    default:
      AT_PRINTF("AT+SEND without the acknowledge flag\r\n");
      return AT_PARAM_ERROR;
  }

  if (bufSize > 0)
  {
    /* skip the acknowledge flag */
    buf ++;
    bufSize --;
  }

  if ((bufSize == 0) || (':' != buf[0]))
  {
    AT_PRINTF("AT+SEND missing : character after ack flag\r\n");
    return AT_PARAM_ERROR;
  }
  else
  {
    /* skip the char ':' */
    buf ++;
    bufSize --;
  }

  while ((size < LORAWAN_APP_DATA_BUFFER_MAX_SIZE) && (bufSize > 1))
  {
    hex[0] = buf[size * 2];
    hex[1] = buf[size * 2 + 1];
    if (tiny_sscanf(hex, "%hhx", &AppData.Buffer[size]) != 1)
    {
      return AT_PARAM_ERROR;
    }
    size++;
    bufSize -= 2;
  }
  if (bufSize != 0)
  {
    return AT_PARAM_ERROR;
  }

  AppData.BufferSize = size;
  AppData.Port = appPort;

  lmhStatus = LmHandlerSend(&AppData, isTxConfirmed, false);

  switch (lmhStatus)
  {
    case LORAMAC_HANDLER_SUCCESS:
      status = AT_OK;
      break;
    case LORAMAC_HANDLER_BUSY_ERROR:
    case LORAMAC_HANDLER_COMPLIANCE_RUNNING:
      status = (LmHandlerJoinStatus() != LORAMAC_HANDLER_SET) ? AT_NO_NET_JOINED : AT_BUSY_ERROR;
      break;
    case LORAMAC_HANDLER_NO_NETWORK_JOINED:
      status = AT_NO_NET_JOINED;
      break;
    case LORAMAC_HANDLER_DUTYCYCLE_RESTRICTED:
      status = AT_DUTYCYCLE_RESTRICTED;
      break;
    case LORAMAC_HANDLER_CRYPTO_ERROR:
      status = AT_CRYPTO_ERROR;
      break;
    case LORAMAC_HANDLER_ERROR:
    default:
      status = AT_ERROR;
      break;
  }

  return status;
  /* USER CODE BEGIN AT_Send_2 */

  /* USER CODE END AT_Send_2 */
}

/* --------------- LoRaWAN network management commands --------------- */
ATEerror_t AT_ADR_get(const char *param)
{
  /* USER CODE BEGIN AT_ADR_get_1 */

  /* USER CODE END AT_ADR_get_1 */
  bool adrEnable;
  if (LmHandlerGetAdrEnable(&adrEnable) != LORAMAC_HANDLER_SUCCESS)
  {
    return AT_PARAM_ERROR;
  }

  print_d(adrEnable);
  return AT_OK;
  /* USER CODE BEGIN AT_ADR_get_2 */

  /* USER CODE END AT_ADR_get_2 */
}

ATEerror_t AT_ADR_set(const char *param)
{
  /* USER CODE BEGIN AT_ADR_set_1 */

  /* USER CODE END AT_ADR_set_1 */
  switch (param[0])
  {
    case '0':
    case '1':
      LmHandlerSetAdrEnable(param[0] - '0');
      break;
    default:
      return AT_PARAM_ERROR;
  }

  return AT_OK;
  /* USER CODE BEGIN AT_ADR_set_2 */

  /* USER CODE END AT_ADR_set_2 */
}

ATEerror_t AT_DataRate_get(const char *param)
{
  /* USER CODE BEGIN AT_DataRate_get_1 */

  /* USER CODE END AT_DataRate_get_1 */
  int8_t txDatarate;
  if (LmHandlerGetTxDatarate(&txDatarate) != LORAMAC_HANDLER_SUCCESS)
  {
    return AT_PARAM_ERROR;
  }

  print_d(txDatarate);
  return AT_OK;
  /* USER CODE BEGIN AT_DataRate_get_2 */

  /* USER CODE END AT_DataRate_get_2 */
}

ATEerror_t AT_DataRate_set(const char *param)
{
  /* USER CODE BEGIN AT_DataRate_set_1 */

  /* USER CODE END AT_DataRate_set_1 */
  int8_t datarate;

  if (tiny_sscanf(param, "%hhu", &datarate) != 1)
  {
    return AT_PARAM_ERROR;
  }
  if ((datarate < 0) || (datarate > 15))
  {
    return AT_PARAM_ERROR;
  }

  if (LmHandlerSetTxDatarate(datarate) != LORAMAC_HANDLER_SUCCESS)
  {
    return AT_ERROR;
  }

  return AT_OK;
  /* USER CODE BEGIN AT_DataRate_set_2 */

  /* USER CODE END AT_DataRate_set_2 */
}

ATEerror_t AT_Region_get(const char *param)
{
  /* USER CODE BEGIN AT_Region_get_1 */

  /* USER CODE END AT_Region_get_1 */
  const char *regionStrings[] =
  {
    "AS923", "AU915", "CN470", "CN779", "EU433", "EU868", "KR920", "IN865", "US915", "RU864"
  };
  LoRaMacRegion_t region;
  if (LmHandlerGetActiveRegion(&region) != LORAMAC_HANDLER_SUCCESS)
  {
    return AT_PARAM_ERROR;
  }

  if (region > LORAMAC_REGION_RU864)
  {
    return AT_PARAM_ERROR;
  }

  AT_PRINTF("%d:%s\r\n", region, regionStrings[region]);
  return AT_OK;
  /* USER CODE BEGIN AT_Region_get_2 */

  /* USER CODE END AT_Region_get_2 */
}

ATEerror_t AT_Region_set(const char *param)
{
  /* USER CODE BEGIN AT_Region_set_1 */

  /* USER CODE END AT_Region_set_1 */
  LoRaMacRegion_t region;
  if (tiny_sscanf(param, "%hhu", &region) != 1)
  {
    return AT_PARAM_ERROR;
  }
  if (region > LORAMAC_REGION_RU864)
  {
    return AT_PARAM_ERROR;
  }

  if (LmHandlerSetActiveRegion(region) != LORAMAC_HANDLER_SUCCESS)
  {
    return AT_PARAM_ERROR;
  }

  return AT_OK;
  /* USER CODE BEGIN AT_Region_set_2 */

  /* USER CODE END AT_Region_set_2 */
}

ATEerror_t AT_DeviceClass_get(const char *param)
{
  /* USER CODE BEGIN AT_DeviceClass_get_1 */

  /* USER CODE END AT_DeviceClass_get_1 */
  DeviceClass_t currentClass;
  LoraInfo_t *loraInfo = LoraInfo_GetPtr();
  if (loraInfo == NULL)
  {
    return AT_ERROR;
  }

  if (LmHandlerGetCurrentClass(&currentClass) != LORAMAC_HANDLER_SUCCESS)
  {
    return AT_PARAM_ERROR;
  }

  if ((loraInfo->ClassB == 1) && (ClassBEnableRequest == true) && (currentClass == CLASS_A))
  {
    BeaconState_t beaconState;

    if (LmHandlerGetBeaconState(&beaconState) != LORAMAC_HANDLER_SUCCESS)
    {
      return AT_PARAM_ERROR;
    }

    if ((beaconState == BEACON_STATE_ACQUISITION) ||
        (beaconState == BEACON_STATE_ACQUISITION_BY_TIME) ||
        (beaconState == BEACON_STATE_REACQUISITION)) /*Beacon_Searching on Class B request*/
    {
      AT_PRINTF("B,S0\r\n");
    }
    else if ((beaconState == BEACON_STATE_LOCKED) || /*Beacon locked on Gateway*/
             (beaconState == BEACON_STATE_IDLE)   ||
             (beaconState == BEACON_STATE_GUARD)  ||
             (beaconState == BEACON_STATE_RX))
    {
      AT_PRINTF("B,S1\r\n");
    }
    else
    {
      AT_PRINTF("B,S2\r\n");
    }
  }
  else /* we are now either in Class B enable or Class C enable*/
  {
    AT_PRINTF("%c\r\n", 'A' + currentClass);
  }

  return AT_OK;
  /* USER CODE BEGIN AT_DeviceClass_get_2 */

  /* USER CODE END AT_DeviceClass_get_2 */
}

ATEerror_t AT_DeviceClass_set(const char *param)
{
  /* USER CODE BEGIN AT_DeviceClass_set_1 */

  /* USER CODE END AT_DeviceClass_set_1 */
  LmHandlerErrorStatus_t errorStatus = LORAMAC_HANDLER_SUCCESS;
  LoraInfo_t *loraInfo = LoraInfo_GetPtr();
  if (loraInfo == NULL)
  {
    return AT_ERROR;
  }

  switch (param[0])
  {
    case 'A':
      if (loraInfo->ClassB == 1)
      {
        ClassBEnableRequest = false;
      }
      errorStatus = LmHandlerRequestClass(CLASS_A);
      break;
    case 'B':
      if (loraInfo->ClassB == 1)
      {
        ClassBEnableRequest = true;
        errorStatus = LmHandlerRequestClass(CLASS_B);  /*Class B AT cmd switch Class B not supported cf.[UM2073]*/
      }
      else
      {
        return AT_NO_CLASS_B_ENABLE;
      }
      break;
    case 'C':
      errorStatus = LmHandlerRequestClass(CLASS_C);
      break;
    default:
      return AT_PARAM_ERROR;
  }

  if (errorStatus == LORAMAC_HANDLER_NO_NETWORK_JOINED)
  {
    return AT_NO_NET_JOINED;
  }
  else if (errorStatus != LORAMAC_HANDLER_SUCCESS)
  {
    return AT_ERROR;
  }

  return AT_OK;
  /* USER CODE BEGIN AT_DeviceClass_set_2 */

  /* USER CODE END AT_DeviceClass_set_2 */
}

ATEerror_t AT_DutyCycle_get(const char *param)
{
  /* USER CODE BEGIN AT_DutyCycle_get_1 */

  /* USER CODE END AT_DutyCycle_get_1 */
  bool dutyCycleEnable;
  if (LmHandlerGetDutyCycleEnable(&dutyCycleEnable) != LORAMAC_HANDLER_SUCCESS)
  {
    return AT_PARAM_ERROR;
  }

  print_d(dutyCycleEnable);
  return AT_OK;
  /* USER CODE BEGIN AT_DutyCycle_get_2 */

  /* USER CODE END AT_DutyCycle_get_2 */
}

ATEerror_t AT_DutyCycle_set(const char *param)
{
  /* USER CODE BEGIN AT_DutyCycle_set_1 */

  /* USER CODE END AT_DutyCycle_set_1 */
  switch (param[0])
  {
    case '0':
    case '1':
      LmHandlerSetDutyCycleEnable(param[0] - '0');
      break;
    default:
      return AT_PARAM_ERROR;
  }

  return AT_OK;
  /* USER CODE BEGIN AT_DutyCycle_set_2 */

  /* USER CODE END AT_DutyCycle_set_2 */
}

ATEerror_t AT_JoinAcceptDelay1_get(const char *param)
{
  /* USER CODE BEGIN AT_JoinAcceptDelay1_get_1 */

  /* USER CODE END AT_JoinAcceptDelay1_get_1 */
  uint32_t rxDelay;
  if (LmHandlerGetJoinRx1Delay(&rxDelay) != LORAMAC_HANDLER_SUCCESS)
  {
    return AT_PARAM_ERROR;
  }

  print_u(rxDelay);
  return AT_OK;
  /* USER CODE BEGIN AT_JoinAcceptDelay1_get_2 */

  /* USER CODE END AT_JoinAcceptDelay1_get_2 */
}

ATEerror_t AT_JoinAcceptDelay1_set(const char *param)
{
  /* USER CODE BEGIN AT_JoinAcceptDelay1_set_1 */

  /* USER CODE END AT_JoinAcceptDelay1_set_1 */
  uint32_t rxDelay;
  if (tiny_sscanf(param, "%lu", &rxDelay) != 1)
  {
    return AT_PARAM_ERROR;
  }
  else if (LmHandlerSetJoinRx1Delay(rxDelay) != LORAMAC_HANDLER_SUCCESS)
  {
    return AT_PARAM_ERROR;
  }

  return AT_OK;
  /* USER CODE BEGIN AT_JoinAcceptDelay1_set_2 */

  /* USER CODE END AT_JoinAcceptDelay1_set_2 */
}

ATEerror_t AT_JoinAcceptDelay2_get(const char *param)
{
  /* USER CODE BEGIN AT_JoinAcceptDelay2_get_1 */

  /* USER CODE END AT_JoinAcceptDelay2_get_1 */
  uint32_t rxDelay;
  if (LmHandlerGetJoinRx2Delay(&rxDelay) != LORAMAC_HANDLER_SUCCESS)
  {
    return AT_PARAM_ERROR;
  }

  print_u(rxDelay);
  return AT_OK;
  /* USER CODE BEGIN AT_JoinAcceptDelay2_get_2 */

  /* USER CODE END AT_JoinAcceptDelay2_get_2 */
}

ATEerror_t AT_JoinAcceptDelay2_set(const char *param)
{
  /* USER CODE BEGIN AT_JoinAcceptDelay2_set_1 */

  /* USER CODE END AT_JoinAcceptDelay2_set_1 */
  uint32_t rxDelay;
  if (tiny_sscanf(param, "%lu", &rxDelay) != 1)
  {
    return AT_PARAM_ERROR;
  }
  else if (LmHandlerSetJoinRx2Delay(rxDelay) != LORAMAC_HANDLER_SUCCESS)
  {
    return AT_PARAM_ERROR;
  }

  return AT_OK;
  /* USER CODE BEGIN AT_JoinAcceptDelay2_set_2 */

  /* USER CODE END AT_JoinAcceptDelay2_set_2 */
}

ATEerror_t AT_Rx1Delay_get(const char *param)
{
  /* USER CODE BEGIN AT_Rx1Delay_get_1 */

  /* USER CODE END AT_Rx1Delay_get_1 */
  uint32_t rxDelay;
  if (LmHandlerGetRx1Delay(&rxDelay) != LORAMAC_HANDLER_SUCCESS)
  {
    return AT_PARAM_ERROR;
  }

  print_u(rxDelay);
  return AT_OK;
  /* USER CODE BEGIN AT_Rx1Delay_get_2 */

  /* USER CODE END AT_Rx1Delay_get_2 */
}

ATEerror_t AT_Rx1Delay_set(const char *param)
{
  /* USER CODE BEGIN AT_Rx1Delay_set_1 */

  /* USER CODE END AT_Rx1Delay_set_1 */
  uint32_t rxDelay;
  if (tiny_sscanf(param, "%lu", &rxDelay) != 1)
  {
    return AT_PARAM_ERROR;
  }
  else if (LmHandlerSetRx1Delay(rxDelay) != LORAMAC_HANDLER_SUCCESS)
  {
    return AT_PARAM_ERROR;
  }

  return AT_OK;
  /* USER CODE BEGIN AT_Rx1Delay_set_2 */

  /* USER CODE END AT_Rx1Delay_set_2 */
}

ATEerror_t AT_Rx2Delay_get(const char *param)
{
  /* USER CODE BEGIN AT_Rx2Delay_get_1 */

  /* USER CODE END AT_Rx2Delay_get_1 */
  uint32_t rxDelay;
  if (LmHandlerGetRx2Delay(&rxDelay) != LORAMAC_HANDLER_SUCCESS)
  {
    return AT_PARAM_ERROR;
  }

  print_u(rxDelay);
  return AT_OK;
  /* USER CODE BEGIN AT_Rx2Delay_get_2 */

  /* USER CODE END AT_Rx2Delay_get_2 */
}

ATEerror_t AT_Rx2Delay_set(const char *param)
{
  /* USER CODE BEGIN AT_Rx2Delay_set_1 */

  /* USER CODE END AT_Rx2Delay_set_1 */
  uint32_t rxDelay;
  if (tiny_sscanf(param, "%lu", &rxDelay) != 1)
  {
    return AT_PARAM_ERROR;
  }
  else if (LmHandlerSetRx2Delay(rxDelay) != LORAMAC_HANDLER_SUCCESS)
  {
    return AT_PARAM_ERROR;
  }

  return AT_OK;
  /* USER CODE BEGIN AT_Rx2Delay_set_2 */

  /* USER CODE END AT_Rx2Delay_set_2 */
}

ATEerror_t AT_Rx2DataRate_get(const char *param)
{
  /* USER CODE BEGIN AT_Rx2DataRate_get_1 */

  /* USER CODE END AT_Rx2DataRate_get_1 */
  RxChannelParams_t rx2Params;
  LmHandlerGetRX2Params(&rx2Params);
  print_d(rx2Params.Datarate);
  return AT_OK;
  /* USER CODE BEGIN AT_Rx2DataRate_get_2 */

  /* USER CODE END AT_Rx2DataRate_get_2 */
}

ATEerror_t AT_Rx2DataRate_set(const char *param)
{
  /* USER CODE BEGIN AT_Rx2DataRate_set_1 */

  /* USER CODE END AT_Rx2DataRate_set_1 */
  RxChannelParams_t rx2Params;

  /* Get the current configuration of RX2 */
  LmHandlerGetRX2Params(&rx2Params);

  /* Update the Datarate with scanf */
  if (tiny_sscanf(param, "%hhu", &(rx2Params.Datarate)) != 1)
  {
    return AT_PARAM_ERROR;
  }
  else if (rx2Params.Datarate > 15)
  {
    return AT_PARAM_ERROR;
  }
  else if (LmHandlerSetRX2Params(&rx2Params) != LORAMAC_HANDLER_SUCCESS)
  {
    return AT_PARAM_ERROR;
  }

  return AT_OK;
  /* USER CODE BEGIN AT_Rx2DataRate_set_2 */

  /* USER CODE END AT_Rx2DataRate_set_2 */
}

ATEerror_t AT_Rx2Frequency_get(const char *param)
{
  /* USER CODE BEGIN AT_Rx2Frequency_get_1 */

  /* USER CODE END AT_Rx2Frequency_get_1 */
  RxChannelParams_t rx2Params;
  LmHandlerGetRX2Params(&rx2Params);
  print_d(rx2Params.Frequency);
  return AT_OK;
  /* USER CODE BEGIN AT_Rx2Frequency_get_2 */

  /* USER CODE END AT_Rx2Frequency_get_2 */
}

ATEerror_t AT_Rx2Frequency_set(const char *param)
{
  /* USER CODE BEGIN AT_Rx2Frequency_set_1 */

  /* USER CODE END AT_Rx2Frequency_set_1 */
  RxChannelParams_t rx2Params;

  /* Get the current configuration of RX2 */
  LmHandlerGetRX2Params(&rx2Params);

  /* Update the frequency with scanf */
  if (tiny_sscanf(param, "%lu", &(rx2Params.Frequency)) != 1)
  {
    return AT_PARAM_ERROR;
  }
  else if (LmHandlerSetRX2Params(&rx2Params) != LORAMAC_HANDLER_SUCCESS)
  {
    return AT_PARAM_ERROR;
  }

  return AT_OK;
  /* USER CODE BEGIN AT_Rx2Frequency_set_2 */

  /* USER CODE END AT_Rx2Frequency_set_2 */
}

ATEerror_t AT_TransmitPower_get(const char *param)
{
  /* USER CODE BEGIN AT_TransmitPower_get_1 */

  /* USER CODE END AT_TransmitPower_get_1 */
  int8_t txPower;
  if (LmHandlerGetTxPower(&txPower) != LORAMAC_HANDLER_SUCCESS)
  {
    return AT_PARAM_ERROR;
  }

  print_d(txPower);
  return AT_OK;
  /* USER CODE BEGIN AT_TransmitPower_get_2 */

  /* USER CODE END AT_TransmitPower_get_2 */
}

ATEerror_t AT_TransmitPower_set(const char *param)
{
  /* USER CODE BEGIN AT_TransmitPower_set_1 */

  /* USER CODE END AT_TransmitPower_set_1 */
  int8_t txPower;
  if (tiny_sscanf(param, "%hhu", &txPower) != 1)
  {
    return AT_PARAM_ERROR;
  }

  if (LmHandlerSetTxPower(txPower) != LORAMAC_HANDLER_SUCCESS)
  {
    return AT_PARAM_ERROR;
  }

  return AT_OK;
  /* USER CODE BEGIN AT_TransmitPower_set_2 */

  /* USER CODE END AT_TransmitPower_set_2 */
}

ATEerror_t AT_PingSlot_get(const char *param)
{
  /* USER CODE BEGIN AT_PingSlot_get_1 */

  /* USER CODE END AT_PingSlot_get_1 */
  uint8_t periodicity;

  if (LmHandlerGetPingPeriodicity(&periodicity) != LORAMAC_HANDLER_SUCCESS)
  {
    return AT_PARAM_ERROR;
  }

  print_d(periodicity);
  return AT_OK;
  /* USER CODE BEGIN AT_PingSlot_get_2 */

  /* USER CODE END AT_PingSlot_get_2 */
}

ATEerror_t AT_PingSlot_set(const char *param)
{
  /* USER CODE BEGIN AT_PingSlot_set_1 */

  /* USER CODE END AT_PingSlot_set_1 */
  uint8_t periodicity;

  if (tiny_sscanf(param, "%hhu", &periodicity) != 1)
  {
    return AT_PARAM_ERROR;
  }
  else if (periodicity > 7)
  {
    return AT_PARAM_ERROR;
  }
  else if (LmHandlerSetPingPeriodicity(periodicity) != LORAMAC_HANDLER_SUCCESS)
  {
    return AT_PARAM_ERROR;
  }

  return AT_OK;
  /* USER CODE BEGIN AT_PingSlot_set_2 */

  /* USER CODE END AT_PingSlot_set_2 */
}

/* --------------- Radio tests commands --------------- */
ATEerror_t AT_test_txTone(const char *param)
{
  /* USER CODE BEGIN AT_test_txTone_1 */

  /* USER CODE END AT_test_txTone_1 */
  if (0U == TST_TxTone())
  {
    return AT_OK;
  }
  else
  {
    return AT_BUSY_ERROR;
  }
  /* USER CODE BEGIN AT_test_txTone_2 */

  /* USER CODE END AT_test_txTone_2 */
}

ATEerror_t AT_test_rxRssi(const char *param)
{
  /* USER CODE BEGIN AT_test_rxRssi_1 */

  /* USER CODE END AT_test_rxRssi_1 */
  if (0U == TST_RxRssi())
  {
    return AT_OK;
  }
  else
  {
    return AT_BUSY_ERROR;
  }
  /* USER CODE BEGIN AT_test_rxRssi_2 */

  /* USER CODE END AT_test_rxRssi_2 */
}

ATEerror_t AT_test_get_config(const char *param)
{
  /* USER CODE BEGIN AT_test_get_config_1 */

  /* USER CODE END AT_test_get_config_1 */
  testParameter_t testParam;
  uint32_t loraBW[7] = {7812, 15625, 31250, 62500, 125000, 250000, 500000};

  TST_get_config(&testParam);

  AT_PRINTF("1: Freq= %d Hz\r\n", testParam.freq);
  AT_PRINTF("2: Power= %d dBm\r\n", testParam.power);

  if ((testParam.modulation == TEST_FSK) || (testParam.modulation == TEST_MSK))
  {
    /*fsk*/
    AT_PRINTF("3: Bandwidth= %d Hz\r\n", testParam.bandwidth);
    AT_PRINTF("4: FSK datarate= %d bps\r\n", testParam.loraSf_datarate);
    AT_PRINTF("5: Coding Rate not applicable\r\n");
    AT_PRINTF("6: LNA State= %d  \r\n", testParam.lna);
    AT_PRINTF("7: PA Boost State= %d  \r\n", testParam.paBoost);
    if (testParam.modulation == TEST_FSK)
    {
      AT_PRINTF("8: modulation FSK\r\n");
    }
    else
    {
      AT_PRINTF("8: modulation MSK\r\n");
    }
    AT_PRINTF("9: Payload len= %d Bytes\r\n", testParam.payloadLen);
    if (testParam.modulation == TEST_FSK)
    {
      AT_PRINTF("10: FSK deviation= %d Hz\r\n", testParam.fskDev);
    }
    else
    {
      AT_PRINTF("10: FSK deviation forced to FSK datarate/4\r\n");
    }
    AT_PRINTF("11: LowDRopt not applicable\r\n");
    AT_PRINTF("12: FSK gaussian BT product= %d \r\n", testParam.BTproduct);
  }
  else if (testParam.modulation == TEST_LORA)
  {
    /*Lora*/
    AT_PRINTF("3: Bandwidth= %d (=%d Hz)\r\n", testParam.bandwidth, loraBW[testParam.bandwidth]);
    AT_PRINTF("4: SF= %d \r\n", testParam.loraSf_datarate);
    AT_PRINTF("5: CR= %d (=4/%d) \r\n", testParam.codingRate, testParam.codingRate + 4);
    AT_PRINTF("6: LNA State= %d  \r\n", testParam.lna);
    AT_PRINTF("7: PA Boost State= %d  \r\n", testParam.paBoost);
    AT_PRINTF("8: modulation LORA\r\n");
    AT_PRINTF("9: Payload len= %d Bytes\r\n", testParam.payloadLen);
    AT_PRINTF("10: Frequency deviation not applicable\r\n");
    AT_PRINTF("11: LowDRopt[0 to 2]= %d \r\n", testParam.lowDrOpt);
    AT_PRINTF("12 BT product not applicable\r\n");
  }
  else
  {
    AT_PRINTF("4: BPSK datarate= %d bps\r\n", testParam.loraSf_datarate);
  }
  return AT_OK;
  /* USER CODE BEGIN AT_test_get_config_2 */

  /* USER CODE END AT_test_get_config_2 */
}

ATEerror_t AT_test_set_config(const char *param)
{
  /* USER CODE BEGIN AT_test_set_config_1 */

  /* USER CODE END AT_test_set_config_1 */
  testParameter_t testParam = {0};
  uint32_t freq;
  int32_t power;
  uint32_t bandwidth;
  uint32_t loraSf_datarate;
  uint32_t codingRate;
  uint32_t lna;
  uint32_t paBoost;
  uint32_t modulation;
  uint32_t payloadLen;
  uint32_t fskDeviation;
  uint32_t lowDrOpt;
  uint32_t BTproduct;
  uint32_t crNum;

  if (13 == tiny_sscanf(param, "%d:%d:%d:%d:%d/%d:%d:%d:%d:%d:%d:%d:%d",
                        &freq,
                        &power,
                        &bandwidth,
                        &loraSf_datarate,
                        &crNum,
                        &codingRate,
                        &lna,
                        &paBoost,
                        &modulation,
                        &payloadLen,
                        &fskDeviation,
                        &lowDrOpt,
                        &BTproduct))
  {
    /*extend to new format for extended*/
  }
  else
  {
    return AT_PARAM_ERROR;
  }
  /*get current config*/
  TST_get_config(&testParam);

  /* 8: modulation check and set */
  /* first check because required for others */
  if (modulation == TEST_FSK)
  {
    testParam.modulation = TEST_FSK;
  }
  else if (modulation == TEST_LORA)
  {
    testParam.modulation = TEST_LORA;
  }
  else if (modulation == TEST_BPSK)
  {
    testParam.modulation = TEST_BPSK;
  }
  else if (modulation == TEST_MSK)
  {
    testParam.modulation = TEST_MSK;
  }
  else
  {
    return AT_PARAM_ERROR;
  }

  /* 1: frequency check and set */
  if (freq < 1000)
  {
    /*given in MHz*/
    testParam.freq = freq * 1000000;
  }
  else
  {
    testParam.freq = freq;
  }

  /* 2: power check and set */
  if ((power >= -9) && (power <= 22))
  {
    testParam.power = power;
  }
  else
  {
    return AT_PARAM_ERROR;
  }

  /* 3: bandwidth check and set */
  if ((testParam.modulation == TEST_FSK) && (bandwidth >= 4800) && (bandwidth <= 467000))
  {
    testParam.bandwidth = bandwidth;
  }
  else if ((testParam.modulation == TEST_MSK) && (bandwidth >= 4800) && (bandwidth <= 467000))
  {
    testParam.bandwidth = bandwidth;
  }
  else if ((testParam.modulation == TEST_LORA) && (bandwidth <= BW_500kHz))
  {
    testParam.bandwidth = bandwidth;
  }
  else if (testParam.modulation == TEST_BPSK)
  {
    /* Not used */
  }
  else
  {
    return AT_PARAM_ERROR;
  }

  /* 4: datarate/spreading factor check and set */
  if ((testParam.modulation == TEST_FSK) && (loraSf_datarate >= 600) && (loraSf_datarate <= 300000))
  {
    testParam.loraSf_datarate = loraSf_datarate;
  }
  else if ((testParam.modulation == TEST_MSK) && (loraSf_datarate >= 100) && (loraSf_datarate <= 300000))
  {
    testParam.loraSf_datarate = loraSf_datarate;
  }
  else if ((testParam.modulation == TEST_LORA) && (loraSf_datarate >= 5) && (loraSf_datarate <= 12))
  {
    testParam.loraSf_datarate = loraSf_datarate;
  }
  else if ((testParam.modulation == TEST_BPSK) && (loraSf_datarate <= 1000))
  {
    testParam.loraSf_datarate = loraSf_datarate;
  }
  else
  {
    return AT_PARAM_ERROR;
  }

  /* 5: coding rate check and set */
  if ((testParam.modulation == TEST_FSK) || (testParam.modulation == TEST_MSK) || (testParam.modulation == TEST_BPSK))
  {
    /* Not used */
  }
  else if ((testParam.modulation == TEST_LORA) && ((codingRate >= 5) && (codingRate <= 8)))
  {
    testParam.codingRate = codingRate - 4;
  }
  else
  {
    return AT_PARAM_ERROR;
  }

  /* 6: lna state check and set */
  if (lna <= 1)
  {
    testParam.lna = lna;
  }
  else
  {
    return AT_PARAM_ERROR;
  }

  /* 7: pa boost check and set */
  if (paBoost <= 1)
  {
    /* Not used */
    testParam.paBoost = paBoost;
  }

  /* 9: payloadLen check and set */
  if ((payloadLen != 0) && (payloadLen < 256))
  {
    testParam.payloadLen = payloadLen;
  }
  else
  {
    return AT_PARAM_ERROR;
  }

  /* 10: fsk Deviation check and set */
  if ((testParam.modulation == TEST_LORA) || (testParam.modulation == TEST_BPSK) || (testParam.modulation == TEST_MSK))
  {
    /* Not used */
  }
  else if ((testParam.modulation == TEST_FSK) && ((fskDeviation >= 600) && (fskDeviation <= 200000)))
  {
    /*given in MHz*/
    testParam.fskDev = fskDeviation;
  }
  else
  {
    return AT_PARAM_ERROR;
  }

  /* 11: low datarate optimization check and set */
  if ((testParam.modulation == TEST_FSK) || (testParam.modulation == TEST_BPSK) || (testParam.modulation == TEST_MSK))
  {
    /* Not used */
  }
  else if ((testParam.modulation == TEST_LORA) && (lowDrOpt <= 2))
  {
    testParam.lowDrOpt = lowDrOpt;
  }
  else
  {
    return AT_PARAM_ERROR;
  }

  /* 12: FSK gaussian BT product check and set */
  if ((testParam.modulation == TEST_LORA) || (testParam.modulation == TEST_BPSK))
  {
    /* Not used */
  }
  else if (((testParam.modulation == TEST_FSK) || (testParam.modulation == TEST_MSK)) && (BTproduct <= 4))
  {
    /*given in MHz*/
    testParam.BTproduct = BTproduct;
  }
  else
  {
    return AT_PARAM_ERROR;
  }

  TST_set_config(&testParam);

  return AT_OK;
  /* USER CODE BEGIN AT_test_set_config_2 */

  /* USER CODE END AT_test_set_config_2 */
}

ATEerror_t AT_test_tx(const char *param)
{
  /* USER CODE BEGIN AT_test_tx_1 */

  /* USER CODE END AT_test_tx_1 */
  const char *buf = param;
  uint32_t nb_packet;

  if (1 != tiny_sscanf(buf, "%u", &nb_packet))
  {
    AT_PRINTF("AT+TTX: nb packets sent is missing\r\n");
    return AT_PARAM_ERROR;
  }

  if (0U == TST_TX_Start(nb_packet))
  {
    return AT_OK;
  }
  else
  {
    return AT_ERROR;
  }
  /* USER CODE BEGIN AT_test_tx_2 */

  /* USER CODE END AT_test_tx_2 */
}

ATEerror_t AT_test_rx(const char *param)
{
  /* USER CODE BEGIN AT_test_rx_1 */

  /* USER CODE END AT_test_rx_1 */
  const char *buf = param;
  uint32_t nb_packet;

  if (1 != tiny_sscanf(buf, "%u", &nb_packet))
  {
    AT_PRINTF("AT+TRX: nb expected packets is missing\r\n");
    return AT_PARAM_ERROR;
  }

  if (0U == TST_RX_Start(nb_packet))
  {
    return AT_OK;
  }
  else
  {
    return AT_ERROR;
  }
  /* USER CODE BEGIN AT_test_rx_2 */

  /* USER CODE END AT_test_rx_2 */
}

ATEerror_t AT_test_tx_hopping(const char *param)
{
  /* USER CODE BEGIN AT_test_tx_hopping_1 */

  /* USER CODE END AT_test_tx_hopping_1 */
  const char *buf = param;
  uint32_t freq_start;
  uint32_t freq_stop;
  uint32_t delta_f;
  uint32_t nb_tx;

  testParameter_t test_param;
  uint32_t hop_freq;

  if (4 != tiny_sscanf(buf, "%u,%u,%u,%u", &freq_start, &freq_stop, &delta_f, &nb_tx))
  {
    return AT_PARAM_ERROR;
  }

  /*if freq is set in MHz, convert to Hz*/
  if (freq_start < 1000)
  {
    freq_start *= 1000000;
  }
  if (freq_stop < 1000)
  {
    freq_stop *= 1000000;
  }
  /**/
  hop_freq = freq_start;

  for (int32_t i = 0; i < nb_tx; i++)
  {
    /*get current config*/
    TST_get_config(&test_param);

    /*increment frequency*/
    test_param.freq = hop_freq;
    /*Set new config*/
    TST_set_config(&test_param);

    APP_TPRINTF("Tx Hop at %dHz. %d of %d\r\n", hop_freq, i, nb_tx);

    if (0U != TST_TX_Start(1))
    {
      return AT_BUSY_ERROR;
    }

    hop_freq += delta_f;

    if (hop_freq > freq_stop)
    {
      hop_freq = freq_start;
    }
  }

  return AT_OK;
  /* USER CODE BEGIN AT_test_tx_hopping_2 */

  /* USER CODE END AT_test_tx_hopping_2 */
}

ATEerror_t AT_test_stop(const char *param)
{
  /* USER CODE BEGIN AT_test_stop_1 */

  /* USER CODE END AT_test_stop_1 */
  TST_stop();
  AT_PRINTF("Test Stop\r\n");
  return AT_OK;
  /* USER CODE BEGIN AT_test_stop_2 */

  /* USER CODE END AT_test_stop_2 */
}

/* --------------- Radio access commands --------------- */
ATEerror_t AT_write_register(const char *param)
{
  /* USER CODE BEGIN AT_write_register_1 */

  /* USER CODE END AT_write_register_1 */
  uint8_t add[2];
  uint16_t add16;
  uint8_t data;

  if (strlen(param) != 7)
  {
    return AT_PARAM_ERROR;
  }

  if (stringToData(param, add, 2) != 0)
  {
    return AT_PARAM_ERROR;
  }
  param += 5;
  if (stringToData(param, &data, 1) != 0)
  {
    return AT_PARAM_ERROR;
  }
  add16 = (((uint16_t)add[0]) << 8) + (uint16_t)add[1];
  Radio.Write(add16, data);

  return AT_OK;
  /* USER CODE BEGIN AT_write_register_2 */

  /* USER CODE END AT_write_register_2 */
}

ATEerror_t AT_read_register(const char *param)
{
  /* USER CODE BEGIN AT_read_register_1 */

  /* USER CODE END AT_read_register_1 */
  uint8_t add[2];
  uint16_t add16;
  uint8_t data;

  if (strlen(param) != 4)
  {
    return AT_PARAM_ERROR;
  }

  if (stringToData(param, add, 2) != 0)
  {
    return AT_PARAM_ERROR;
  }

  add16 = (((uint16_t)add[0]) << 8) + (uint16_t)add[1];
  data = Radio.Read(add16);
  AT_PRINTF("REG 0x%04X=0x%02X", add16, data);

  return AT_OK;
  /* USER CODE BEGIN AT_read_register_2 */

  /* USER CODE END AT_read_register_2 */
}

/* --------------- LoraWAN Certif command --------------- */
ATEerror_t AT_Certif(const char *param)
{
  /* USER CODE BEGIN AT_Certif_1 */

  /* USER CODE END AT_Certif_1 */
  switch (param[0])
  {
    case '0':
      LmHandlerJoin(ACTIVATION_TYPE_ABP, true);
    case '1':
      LmHandlerJoin(ACTIVATION_TYPE_OTAA, true);
      break;
    default:
      return AT_PARAM_ERROR;
  }

  UTIL_TIMER_Create(&TxCertifTimer, 8000, UTIL_TIMER_ONESHOT, OnCertifTimer, NULL);  /* 8s */
  UTIL_TIMER_Start(&TxCertifTimer);
  UTIL_SEQ_RegTask((1 << CFG_SEQ_Task_LoRaCertifTx), UTIL_SEQ_RFU, CertifSend);

  return AT_OK;
  /* USER CODE BEGIN AT_Certif_2 */

  /* USER CODE END AT_Certif_2 */
}

/* --------------- Information command --------------- */
ATEerror_t AT_bat_get(const char *param)
{
  /* USER CODE BEGIN AT_bat_get_1 */

  /* USER CODE END AT_bat_get_1 */
  print_d(SYS_GetBatteryLevel());

  return AT_OK;
  /* USER CODE BEGIN AT_bat_get_2 */

  /* USER CODE END AT_bat_get_2 */
}

/* Includes ------------------------------------------------------------------*/
#include "platform.h"
#include "sys_app.h"
#include "stm32_tiny_sscanf.h"
#include "app_version.h"
#include "subghz_phy_version.h"
#include "test_rf.h"
#include "stm32_seq.h"
#include "utilities_def.h"
#include "radio.h"
#include "stm32_timer.h"
#include "stm32_systime.h"

/* USER CODE BEGIN Includes */

#include "platform.h"
#include "sys_app.h"
#include "subghz_phy_app.h"
#include "radio.h"

/* USER CODE BEGIN Includes */
#include "stm32_timer.h"
#include "stm32_seq.h"
#include "utilities_def.h"
#include "app_version.h"
#include "subghz_phy_version.h"
#include "stdio.h"
/* USER CODE END Includes */

static RadioEvents_t RadioEvents;

/* Private define ------------------------------------------------------------*/

typedef enum
{
  RX,
  RX_TIMEOUT,
  RX_ERROR,
  TX,
  TX_TIMEOUT,
} States_t;

/* USER CODE BEGIN PD */
/* Configurations */
/*Timeout*/
#define RX_TIMEOUT_VALUE              3000
#define TX_TIMEOUT_VALUE              3000
/* PING string*/
#define PING "PING"
/* PONG string*/
#define PONG "PONG"
/*Size of the payload to be sent*/
/* Size must be greater of equal the PING and PONG*/
#define MAX_APP_BUFFER_SIZE          255
#if (PAYLOAD_LEN > MAX_APP_BUFFER_SIZE)
#error PAYLOAD_LEN must be less or equal than MAX_APP_BUFFER_SIZE
#endif /* (PAYLOAD_LEN > MAX_APP_BUFFER_SIZE) */
/* wait for remote to be in Rx, before sending a Tx frame*/
#define RX_TIME_MARGIN                200
/* Afc bandwidth in Hz */
#define FSK_AFC_BANDWIDTH             83333
/* LED blink Period*/
#define LED_PERIOD_MS                 200

static States_t State = RX;
/* App Rx Buffer*/
static uint8_t BufferRx[MAX_APP_BUFFER_SIZE];
/* App Tx Buffer*/
static uint8_t BufferTx[MAX_APP_BUFFER_SIZE];
/* Last  Received Buffer Size*/
uint16_t RxBufferSize = 0;
/* Last  Received packer Rssi*/
int8_t RssiValue = 0;

int8_t tx_RssiValue = 0;

int8_t rx_RssiValue = 0;

int SendFreq=0;

/* Last  Received packer SNR (in Lora modulation)*/
int8_t SnrValue = 0;
/* Led Timers objects*/
static UTIL_TIMER_Object_t timerLed;
/* device state. Master: true, Slave: false*/
bool isMaster = false;
/* random delay to make sure 2 devices will sync*/
/* the closest the random delays are, the longer it will
   take for the devices to sync when started simultaneously*/
static int32_t random_delay;

static uint8_t pingpong_test_flag=0;

static void OnTxDone(void);

/**
  * @brief Function to be executed on Radio Rx Done event
  * @param  payload ptr of buffer received
  * @param  size buffer size
  * @param  rssi
  * @param  LoraSnr_FskCfo
  */
static void OnRxDone(uint8_t *payload, uint16_t size, int16_t rssi, int8_t LoraSnr_FskCfo);

/**
  * @brief Function executed on Radio Tx Timeout event
  */
static void OnTxTimeout(void);

/**
  * @brief Function executed on Radio Rx Timeout event
  */
static void OnRxTimeout(void);

/**
  * @brief Function executed on Radio Rx Error event
  */
static void OnRxError(void);

/* USER CODE BEGIN PFP */
static void PingPong_Process(void);



void gpio_set_state(GPIO_TypeDef *GPIOx, uint16_t GPIO_Pin,GPIO_PinState PinState){

	GPIO_InitTypeDef GPIO_InitStruct = {0};
	
	GPIO_InitStruct.Pin = GPIO_Pin;
	GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
	GPIO_InitStruct.Pull = GPIO_NOPULL;
	HAL_GPIO_Init(GPIOx, &GPIO_InitStruct);
	
	HAL_GPIO_WritePin(GPIOx, GPIO_Pin, PinState);
}
#include "radio_driver.h"
ATEerror_t AT_CHIPID_GET(const char *param)
{
	uint8_t i;
	uint8_t mac[12];

	for(i=0;i<12;i++)
	{
		mac[i]			=	*(uint8_t *)(0x1FFF7590+i);
	}
	
	AT_PRINTF("MAC=%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x\r\n",
											mac[0],mac[1],mac[2],mac[3],
											mac[4],mac[5],mac[6],mac[7],
											mac[8],mac[9],mac[10],mac[11]);
  return AT_OK;
  /* USER CODE BEGIN AT_test_stop_2 */

  /* USER CODE END AT_test_stop_2 */
}



ATEerror_t AT_GPIO_TEST(const char *param)
{
	const char *buf = param;
	const char io0_15_bit[4];
	const char io16_31_bit[4];
	const char io32_47_bit[4];
	GPIO_TypeDef *GPIOx;
	
  char port_buf[3];
	
	if (3 != tiny_sscanf(buf, "0x%08x,0x%08x,0x%08x", &io0_15_bit, &io16_31_bit, &io32_47_bit))
  {
    return AT_PARAM_ERROR;
  }
//	AT_PRINTF("param1[0-8bit]=0x%02x,[9-16bit]=0x%02x,[17-24bit]=0x%02x,[25-32bit]=0x%02x\r\n",io0_15_bit[0],io0_15_bit[1],io0_15_bit[2],io0_15_bit[3]);
//	AT_PRINTF("param2[0-8bit]=0x%02x,[9-16bit]=0x%02x,[17-24bit]=0x%02x,[25-32bit]=0x%02x\r\n",io16_31_bit[0],io16_31_bit[1],io16_31_bit[2],io16_31_bit[3]);
//	AT_PRINTF("param3[0-8bit]=0x%02x,[9-16bit]=0x%02x,[17-24bit]=0x%02x,[25-32bit]=0x%02x\r\n",io32_47_bit[0],io32_47_bit[1],io32_47_bit[2],io32_47_bit[3]);

	int32_t cnt=0;
	uint32_t IO_Pin;

	__HAL_RCC_GPIOA_CLK_ENABLE();
	__HAL_RCC_GPIOB_CLK_ENABLE();
	__HAL_RCC_GPIOC_CLK_ENABLE();
	
	for(int32_t i=0;i<4;i++){
		for(int32_t j=0;j<4;j++){
			if(((io0_15_bit[i]) & ((0x02)<<(2*j))))
			{	
				if(((io0_15_bit[i]) & ((0x01)<<(2*j)))){
						gpio_set_state(GPIOA,GPIO_PIN_0<<(cnt%16),1); //out high
						AT_PRINTF("LORAGPIOPA%d Set output high\r\n",(cnt%16));
				}else{
						gpio_set_state(GPIOA,GPIO_PIN_0<<(cnt%16),0); //out low
						AT_PRINTF("LORAGPIOPA%d Set output low\r\n",(cnt%16));
				}
			}else{
				//AT_PRINTF("CBB_GPIO%d NC\r\n",cnt);
			}
			cnt++;
		}
	}
	for(int32_t i=0;i<4;i++){
		for(int32_t j=0;j<4;j++){
			if(((io16_31_bit[i]) & ((0x02)<<(2*j))))
			{
				if(((io16_31_bit[i]) & ((0x01)<<(2*j)))){
						gpio_set_state(GPIOB,GPIO_PIN_0<<(cnt%16),1); //PB0-15 out high
						AT_PRINTF("LORAGPIOPB%d Set output high\r\n",(cnt%16));
				}else{
					gpio_set_state(GPIOB,GPIO_PIN_0<<(cnt%16),0); //PB0-15 out low
					AT_PRINTF("LORAGPIOPB%d Set output low\r\n",(cnt%16));
				}
			}else{
				//AT_PRINTF("CBB_GPIO%d NC\r\n",cnt);
			}
			cnt++;
		}
	}
	for(int32_t i=0;i<4;i++){
		for(int32_t j=0;j<4;j++){
			if(((io32_47_bit[i]) & ((0x02)<<(2*j))))
			{
				if(((io32_47_bit[i]) & ((0x01)<<(2*j)))){
					gpio_set_state(GPIOC,GPIO_PIN_0<<(cnt%16),1); //PC0-15 out high
					AT_PRINTF("LORAGPIOPC%d Set output high\r\n",(cnt%16));
				}else{
					gpio_set_state(GPIOC,GPIO_PIN_0<<(cnt%16),0); //PC0-15 out low
					AT_PRINTF("LORAGPIOPC%d Set output low\r\n",(cnt%16));
				}
			}else{
				//AT_PRINTF("CBB_GPIO%d NC\r\n",cnt);
			}
			cnt++;
		}
	}

  return AT_OK;
  /* USER CODE BEGIN AT_test_stop_2 */

  /* USER CODE END AT_test_stop_2 */
}


#if(TEST_Borad_MODE==1)

char RF_Param[64]={0};
//uint8_t need_save_rf_param_falg=0;

ATEerror_t AT_MODE_SET(const char *param)
{
	const char *buf = param;
	int FactoryTest_mode;

	if (1 != tiny_sscanf(buf, "%u,%u,%u", &FactoryTest_mode))
  {
    return AT_PARAM_ERROR;
  }
	if(FactoryTest_mode==1){
			RF_Param[0]=1;
//			need_save_rf_param_falg=1;
	}else if(FactoryTest_mode==0){
			RF_Param[0]=0;
	}else{
			return AT_PARAM_ERROR;
	}
	int32_t ret;
	if (FLASH_IF_Erase(PINGPONGMODE_NVM_BASE_ADDRESS, FLASH_PAGE_SIZE) == FLASH_IF_OK)
  {
    ret=FLASH_IF_Write(PINGPONGMODE_NVM_BASE_ADDRESS, (const void *)RF_Param, 64);		
		if(ret== FLASH_IF_OK){
			AT_PRINTF("save FactoryTest_mode OK \r\n");
		}
  }else{
		AT_PRINTF("save FactoryTest_mode error\r\n");
	}
	
	return AT_OK;
}
#endif

void InttoHex(char *buf, int data, int byteNum)
{
		int i;
		for(i=0; i<byteNum; i++)
		{
				if(i < (byteNum-1))
				{
								buf[i] = data >> (8*(byteNum-i-1));
				}
				else
				{
								buf[i] = data % 256;
				}
		}
}


ATEerror_t AT_BANDMASK_SET(const char *param)
{
	const char *buf = param;
	const char mask_bit[2];
	ChanMaskSetParams_t chanMaskSet;
	int32_t channel_mask[16]={0};
	static uint16_t LoraWan_DEFAULT_MASK_Channel[6]={0};
	
	LoraWan_DEFAULT_MASK_Channel[0]=0x0000;
	LoraWan_DEFAULT_MASK_Channel[1]=0x0000;
	LoraWan_DEFAULT_MASK_Channel[2]=0x0000;
	LoraWan_DEFAULT_MASK_Channel[3]=0x0000;
	LoraWan_DEFAULT_MASK_Channel[4]=0x0000;
	LoraWan_DEFAULT_MASK_Channel[5]=0x0000;
	
	int32_t channnel_number=0;
	
	if (1 != tiny_sscanf(buf, "%04x" ,&mask_bit))
  {
    return AT_PARAM_ERROR;
  }
//	AT_PRINTF("param1[0-8bit]=0x%02x,[9-16bit]=0x%02x\r\n",mask_bit[0],mask_bit[1]);
	
	for(int32_t i=0;i<2;i++){
		for(int32_t j=0;j<8;j++){
			if((mask_bit[i] & (0x1 << j))){
				channel_mask[(i*8)+j]=1;
				
				channnel_number=(i*8)+j;
				channnel_number *=8;
				AT_PRINTF("Channel %d-%d on\r\n",channnel_number,channnel_number+7);
			}
		}
	}
	for(int32_t bit=0;bit<12;bit++){
			if(channel_mask[bit]==1){
				int32_t cnt=0;
				int32_t Left_shift_number=0;
				if(bit==0){
					cnt=0;
					LoraWan_DEFAULT_MASK_Channel[cnt] |=0x00ff;
				}else if(bit%2 ==0){ //Even number
					cnt=bit/2;
					LoraWan_DEFAULT_MASK_Channel[cnt] |=0x00ff;
				}else if(bit%2 ==1){ //Odd number
					cnt=bit/2;
					LoraWan_DEFAULT_MASK_Channel[cnt] |=0xff00;
				}
			}
	}
//	AT_PRINTF("LoraWan_DEFAULT_MASK_Channel[0]=%04x\r\n",LoraWan_DEFAULT_MASK_Channel[0]);
//	AT_PRINTF("LoraWan_DEFAULT_MASK_Channel[1]=%04x\r\n",LoraWan_DEFAULT_MASK_Channel[1]);
//	AT_PRINTF("LoraWan_DEFAULT_MASK_Channel[2]=%04x\r\n",LoraWan_DEFAULT_MASK_Channel[2]);
//	AT_PRINTF("LoraWan_DEFAULT_MASK_Channel[3]=%04x\r\n",LoraWan_DEFAULT_MASK_Channel[3]);
//	AT_PRINTF("LoraWan_DEFAULT_MASK_Channel[4]=%04x\r\n",LoraWan_DEFAULT_MASK_Channel[4]);
//	AT_PRINTF("LoraWan_DEFAULT_MASK_Channel[5]=%04x\r\n",LoraWan_DEFAULT_MASK_Channel[5]);
	

	MibRequestConfirm_t mibSet;
	mibSet.Type = MIB_CHANNELS_DEFAULT_MASK;
	mibSet.Param.ChannelsDefaultMask=LoraWan_DEFAULT_MASK_Channel;

	if( LoRaMacMibSetRequestConfirm( &mibSet ) != LORAMAC_STATUS_OK )
	{
			return AT_ERROR;
	}
	
		return AT_OK;

}

/* USER CODE BEGIN EF */
ATEerror_t AT_PINGPONG_TEST(const char *param)
{
	const char *buf = param;
	int mode;
	int fre;
	int out_power;
	if (3 != tiny_sscanf(buf, "%u,%u,%u", &mode, &fre,&out_power))
  {
    return AT_PARAM_ERROR;
  }
	if(mode==1){
			isMaster=true;
	}else if(mode==0){
			isMaster=false;
	}else{
			return AT_PARAM_ERROR;
	}
	
	if(out_power<2 || out_power >22){
			return AT_PARAM_ERROR;
	}
	
	#if(TEST_Borad_MODE==1)
				char fre_temp[4];
				char power_temp[1];
				memset(fre_temp, 0, 4);
				memset(power_temp, 0, 1);
				InttoHex(fre_temp, fre, 4);
				InttoHex(power_temp, out_power, 1);
				
				memcpy(&RF_Param[2],fre_temp,4);
				memcpy(&RF_Param[1],power_temp,1);
		
	//			AT_PRINTF("tx_fre = %02x%02x%02x%02x,TEST_FLAG[1]=%02x\r\n",fre_temp[0],fre_temp[1],fre_temp[2],fre_temp[3],power_temp[0]);
	//			AT_PRINTF("tx_fre = %02x%02x%02x%02x,TEST_FLAG[1]=%02x\r\n",TEST_FLAG[2],TEST_FLAG[3],TEST_FLAG[4],TEST_FLAG[5],TEST_FLAG[1]);
		
				int32_t ret;
				if (FLASH_IF_Erase(PINGPONGMODE_NVM_BASE_ADDRESS, FLASH_PAGE_SIZE) == FLASH_IF_OK)
				{
					ret=FLASH_IF_Write(PINGPONGMODE_NVM_BASE_ADDRESS, (const void *)RF_Param, 64);
	//				AT_PRINTF("write ret = %d\r\n",ret);
					if(ret== FLASH_IF_OK){
						AT_PRINTF("\r\nsave param OK \r\n");
					}
				}else{
					AT_PRINTF("save param error\r\n");
				}		
	#endif
  
//	if(pingpong_test_flag==0){
		
		AT_PRINTF("\r\nPINGPONG Test Start\r\n");
		/* USER CODE END SubghzApp_Init_1 */
		
		SendFreq=fre;
		
		/* Radio initialization */
		RadioEvents.TxDone = OnTxDone;
		RadioEvents.RxDone = OnRxDone;
		RadioEvents.TxTimeout = OnTxTimeout;
		RadioEvents.RxTimeout = OnRxTimeout;
		RadioEvents.RxError = OnRxError;

		Radio.Init(&RadioEvents);

		/* USER CODE BEGIN SubghzApp_Init_2 */
		/*calculate random delay for synchronization*/
		random_delay = (Radio.Random()) >> 22; /*10bits random e.g. from 0 to 1023 ms*/

		/* Radio Set frequency */
		Radio.SetChannel(fre);

		/* Radio configuration */
		APP_LOG(TS_OFF, VLEVEL_M, "---------------\n\r");
		APP_LOG(TS_OFF, VLEVEL_M, "LORA_MODULATION\n\r");
		APP_LOG(TS_OFF, VLEVEL_M, "LORA_BW=%d kHz\n\r", (1 << LORA_BANDWIDTH) * 125);
		APP_LOG(TS_OFF, VLEVEL_M, "LORA_SF=%d\n\r", LORA_SPREADING_FACTOR);
		APP_LOG(TS_OFF, VLEVEL_M, "LORA_TX_OUTPUT_POWER=%d\n\r", out_power);
		APP_LOG(TS_OFF, VLEVEL_M, "LORA_TX_Fre=%d\n\r",fre);
		
		Radio.SetTxConfig(MODEM_LORA, out_power, 0, LORA_BANDWIDTH,
											LORA_SPREADING_FACTOR, LORA_CODINGRATE,
											LORA_PREAMBLE_LENGTH, LORA_FIX_LENGTH_PAYLOAD_ON,
											true, 0, 0, LORA_IQ_INVERSION_ON, TX_TIMEOUT_VALUE);

		Radio.SetRxConfig(MODEM_LORA, LORA_BANDWIDTH, LORA_SPREADING_FACTOR,
											LORA_CODINGRATE, 0, LORA_PREAMBLE_LENGTH,
											LORA_SYMBOL_TIMEOUT, LORA_FIX_LENGTH_PAYLOAD_ON,
											0, true, 0, 0, LORA_IQ_INVERSION_ON, true);

		Radio.SetMaxPayloadLength(MODEM_LORA, MAX_APP_BUFFER_SIZE);

		/*fills tx buffer*/
		memset(BufferTx, 0x0, MAX_APP_BUFFER_SIZE);

		APP_LOG(TS_ON, VLEVEL_L, "rand=%d\n\r", random_delay);
		/*starts reception*/
		Radio.Rx(RX_TIMEOUT_VALUE + random_delay);

		/*register task to to be run in while(1) after Radio IT*/
		UTIL_SEQ_RegTask((1 << CFG_SEQ_Task_SubGHz_Phy_App_Process), UTIL_SEQ_RFU, PingPong_Process);
		
		pingpong_test_flag=1;
		/* USER CODE END SubghzApp_Init_2 */
		
		/* USER CODE END SubghzApp_Init_2 */
		
		return AT_OK;
//	}
//	AT_PRINTF(" Error!! PINGPONG Test already Start\r\n");
//	return AT_ERROR;
}





/* USER CODE BEGIN EF */

/* USER CODE END EF */

/* Private Functions Definition -----------------------------------------------*/
static int32_t sscanf_uint32_as_hhx(const char *from, uint32_t *value)
{
  /* USER CODE BEGIN sscanf_uint32_as_hhx_1 */

  /* USER CODE END sscanf_uint32_as_hhx_1 */
  return tiny_sscanf(from, "%hhx:%hhx:%hhx:%hhx",
                     &((unsigned char *)(value))[3],
                     &((unsigned char *)(value))[2],
                     &((unsigned char *)(value))[1],
                     &((unsigned char *)(value))[0]);
  /* USER CODE BEGIN sscanf_uint32_as_hhx_2 */

  /* USER CODE END sscanf_uint32_as_hhx_2 */
}

static int32_t sscanf_16_hhx(const char *from, uint8_t *pt)
{
  /* USER CODE BEGIN sscanf_16_hhx_1 */

  /* USER CODE END sscanf_16_hhx_1 */
  return tiny_sscanf(from, "%hhx:%hhx:%hhx:%hhx:%hhx:%hhx:%hhx:%hhx:%hhx:%hhx:%hhx:%hhx:%hhx:%hhx:%hhx:%hhx",
                     &pt[0], &pt[1], &pt[2], &pt[3], &pt[4], &pt[5], &pt[6],
                     &pt[7], &pt[8], &pt[9], &pt[10], &pt[11], &pt[12], &pt[13],
                     &pt[14], &pt[15]);
  /* USER CODE BEGIN sscanf_16_hhx_2 */

  /* USER CODE END sscanf_16_hhx_2 */
}

static void print_uint32_as_02x(uint32_t value)
{
  /* USER CODE BEGIN print_uint32_as_02x_1 */

  /* USER CODE END print_uint32_as_02x_1 */
  AT_PRINTF("%02X:%02X:%02X:%02X\r\n",
            (unsigned)((unsigned char *)(&value))[3],
            (unsigned)((unsigned char *)(&value))[2],
            (unsigned)((unsigned char *)(&value))[1],
            (unsigned)((unsigned char *)(&value))[0]);
  /* USER CODE BEGIN print_uint32_as_02x_2 */

  /* USER CODE END print_uint32_as_02x_2 */
}

static void print_16_02x(uint8_t *pt)
{
  /* USER CODE BEGIN print_16_02x_1 */

  /* USER CODE END print_16_02x_1 */
  AT_PRINTF("%02X:%02X:%02X:%02X:%02X:%02X:%02X:%02X:%02X:%02X:%02X:%02X:%02X:%02X:%02X:%02X\r\n",
            pt[0], pt[1], pt[2], pt[3],
            pt[4], pt[5], pt[6], pt[7],
            pt[8], pt[9], pt[10], pt[11],
            pt[12], pt[13], pt[14], pt[15]);
  /* USER CODE BEGIN print_16_02x_2 */

  /* USER CODE END print_16_02x_2 */
}

static void print_8_02x(uint8_t *pt)
{
  /* USER CODE BEGIN print_8_02x_1 */

  /* USER CODE END print_8_02x_1 */
  AT_PRINTF("%02X:%02X:%02X:%02X:%02X:%02X:%02X:%02X\r\n",
            pt[0], pt[1], pt[2], pt[3], pt[4], pt[5], pt[6], pt[7]);
  /* USER CODE BEGIN print_8_02x_2 */

  /* USER CODE END print_8_02x_2 */
}

static void print_d(int32_t value)
{
  /* USER CODE BEGIN print_d_1 */

  /* USER CODE END print_d_1 */
  AT_PRINTF("%d\r\n", value);
  /* USER CODE BEGIN print_d_2 */

  /* USER CODE END print_d_2 */
}

static void print_u(uint32_t value)
{
  /* USER CODE BEGIN print_u_1 */

  /* USER CODE END print_u_1 */
  AT_PRINTF("%u\r\n", value);
  /* USER CODE BEGIN print_u_2 */

  /* USER CODE END print_u_2 */
}

static void OnCertifTimer(void *context)
{
  /* USER CODE BEGIN OnCertifTimer_1 */

  /* USER CODE END OnCertifTimer_1 */
  UTIL_SEQ_SetTask((1 << CFG_SEQ_Task_LoRaCertifTx), CFG_SEQ_Prio_0);
  /* USER CODE BEGIN OnCertifTimer_2 */

  /* USER CODE END OnCertifTimer_2 */
}

static void CertifSend(void)
{
  /* USER CODE BEGIN CertifSend_1 */

  /* USER CODE END CertifSend_1 */
  AppData.Buffer[0] = 0x43;
  AppData.BufferSize = 1;
  AppData.Port = 99;

  /* Restart Tx to prevent a previous Join Failed */
  if (LmHandlerJoinStatus() != LORAMAC_HANDLER_SET)
  {
    UTIL_TIMER_Start(&TxCertifTimer);
  }
  LmHandlerSend(&AppData, LORAMAC_HANDLER_UNCONFIRMED_MSG, false);
}

static uint8_t Char2Nibble(char Char)
{
  if (((Char >= '0') && (Char <= '9')))
  {
    return Char - '0';
  }
  else if (((Char >= 'a') && (Char <= 'f')))
  {
    return Char - 'a' + 10;
  }
  else if ((Char >= 'A') && (Char <= 'F'))
  {
    return Char - 'A' + 10;
  }
  else
  {
    return 0xF0;
  }
  /* USER CODE BEGIN CertifSend_2 */

  /* USER CODE END CertifSend_2 */
}

static int32_t stringToData(const char *str, uint8_t *data, uint32_t Size)
{
  /* USER CODE BEGIN stringToData_1 */

  /* USER CODE END stringToData_1 */
  char hex[3];
  hex[2] = 0;
  int32_t ii = 0;
  while (Size-- > 0)
  {
    hex[0] = *str++;
    hex[1] = *str++;

    /*check if input is hex */
    if ((isHex(hex[0]) == -1) || (isHex(hex[1]) == -1))
    {
      return -1;
    }
    /*check if input is even nb of character*/
    if ((hex[1] == '\0') || (hex[1] == ','))
    {
      return -1;
    }
    data[ii] = (Char2Nibble(hex[0]) << 4) + Char2Nibble(hex[1]);

    ii++;
  }

  return 0;
  /* USER CODE BEGIN stringToData_2 */

  /* USER CODE END stringToData_2 */
}

static int32_t isHex(char Char)
{
  /* USER CODE BEGIN isHex_1 */

  /* USER CODE END isHex_1 */
  if (((Char >= '0') && (Char <= '9')) ||
      ((Char >= 'a') && (Char <= 'f')) ||
      ((Char >= 'A') && (Char <= 'F')))
  {
    return 0;
  }
  else
  {
    return -1;
  }
  /* USER CODE BEGIN isHex_2 */

  /* USER CODE END isHex_2 */
}

unsigned char hex2int(char c)
{
    if (c >= '0' && c <= '9') {
        return (unsigned char )(c - 48);
    } else if (c >= 'A' && c <= 'F') {
        return (unsigned char )(c - 55);
    } else if (c >= 'a' && c <= 'f') {
        return (unsigned char )(c - 87);
    } else {
        return 0;
    }
}


static void OnTxDone(void)
{
  /* USER CODE BEGIN OnTxDone */
  APP_LOG(TS_ON, VLEVEL_L, "OnTxDone\n\r");
  /* Update the State of the FSM*/
  State = TX;
  /* Run PingPong process in background*/
  UTIL_SEQ_SetTask((1 << CFG_SEQ_Task_SubGHz_Phy_App_Process), CFG_SEQ_Prio_0);
  /* USER CODE END OnTxDone */
}


static void OnRxDone(uint8_t *payload, uint16_t size, int16_t rssi, int8_t LoraSnr_FskCfo)
{
  /* USER CODE BEGIN OnRxDone */
  APP_LOG(TS_ON, VLEVEL_L, "OnRxDone\n\r");
	
#if ((USE_MODEM_LORA == 1) && (USE_MODEM_FSK == 0))
//  APP_LOG(TS_ON, VLEVEL_L, "RssiValue=%d dBm, SnrValue=%ddB\n\r", rssi, LoraSnr_FskCfo);
  /* Record payload Signal to noise ratio in Lora*/
  SnrValue = LoraSnr_FskCfo;
#endif /* USE_MODEM_LORA | USE_MODEM_FSK */
	
#if ((USE_MODEM_LORA == 0) && (USE_MODEM_FSK == 1))
  APP_LOG(TS_ON, VLEVEL_L, "RssiValue=%d dBm, Cfo=%dkHz\n\r", rssi, LoraSnr_FskCfo);
  SnrValue = 0; /*not applicable in GFSK*/
#endif /* USE_MODEM_LORA | USE_MODEM_FSK */
  /* Update the State of the FSM*/
  State = RX;
  /* Clear BufferRx*/
  memset(BufferRx, 0, MAX_APP_BUFFER_SIZE);
  /* Record payload size*/
  RxBufferSize = size;
  if (RxBufferSize <= MAX_APP_BUFFER_SIZE)
  {
    memcpy(BufferRx, payload, RxBufferSize);
  }
  /* Record Received Signal Strength*/
  RssiValue = rssi;

	if(isMaster==1){
		tx_RssiValue=rssi;
		APP_LOG(TS_ON, VLEVEL_L, "tx_RssiValue=%d dBm, SnrValue=%ddB\n\r", rssi, LoraSnr_FskCfo);
	}else{
		int rssi_value[4];
		int rssi_cnt =0;
		int rssi_flag =0;
		int SendFre_cnt =0;
		int tx_SendFreq=0;
	
		tx_RssiValue=0;
		rx_RssiValue=rssi;
		
		APP_LOG(TS_ON, VLEVEL_H, "payload. size=%d \n\r", size);
		for (int32_t i = 0; i < PAYLOAD_LEN; i++)
		{
			APP_LOG(TS_OFF, VLEVEL_H, "%02X", BufferRx[i]);
			if(BufferRx[i]==0x2D){  // '-'
					rssi_flag=1;
					continue;
			}else if(BufferRx[i]==0x2B){ // '+'
					rssi_flag=0;
					SendFre_cnt=i+1;
			}
			
			if(BufferRx[i]>=0x30 && BufferRx[i]<=0x39){
					if(rssi_flag==1){
						rssi_value[rssi_cnt]=hex2int(BufferRx[i]);
						rssi_cnt++;
					}
			}
			if (i % 16 == 15)
			{
				APP_LOG(TS_OFF, VLEVEL_H, "\n\r");
			}
		}
		APP_LOG(TS_OFF, VLEVEL_H, "\n\r");
		
		if(rssi_cnt==1){
			tx_RssiValue=rssi_value[rssi_cnt-1];
		}else if(rssi_cnt==2){
			tx_RssiValue=rssi_value[rssi_cnt-2]*10+rssi_value[rssi_cnt-1];
		}else if(rssi_cnt==3){
			tx_RssiValue=rssi_value[rssi_cnt-3]*100+rssi_value[rssi_cnt-2]*10+rssi_value[rssi_cnt-1];
		}
		
		tx_SendFreq=(BufferRx[SendFre_cnt]<<24)+(BufferRx[SendFre_cnt+1]<<16)+(BufferRx[SendFre_cnt+2]<<8)+(BufferRx[SendFre_cnt+3]<<0);
		
		//APP_LOG(TS_ON, VLEVEL_L, "BufferRx[SendFre_cnt]=%02x%02x%02x%02x\n\r",BufferRx[SendFre_cnt],BufferRx[SendFre_cnt+1],BufferRx[SendFre_cnt+2],BufferRx[SendFre_cnt+3]);
		
		APP_LOG(TS_ON, VLEVEL_L, "rx_rssi =%d dBm,tx_rssi=-%d dBm,SendFreq=%d,SnrValue=%ddB\n\r",rx_RssiValue,tx_RssiValue,tx_SendFreq,LoraSnr_FskCfo);
	}
  /* Run PingPong process in background*/
  UTIL_SEQ_SetTask((1 << CFG_SEQ_Task_SubGHz_Phy_App_Process), CFG_SEQ_Prio_0);
  /* USER CODE END OnRxDone */
}


static void OnTxTimeout(void)
{
  /* USER CODE BEGIN OnTxTimeout */
  APP_LOG(TS_ON, VLEVEL_L, "OnTxTimeout\n\r");
  /* Update the State of the FSM*/
  State = TX_TIMEOUT;
  /* Run PingPong process in background*/
  UTIL_SEQ_SetTask((1 << CFG_SEQ_Task_SubGHz_Phy_App_Process), CFG_SEQ_Prio_0);
  /* USER CODE END OnTxTimeout */
}

static void OnRxTimeout(void)
{
  /* USER CODE BEGIN OnRxTimeout */
  APP_LOG(TS_ON, VLEVEL_L, "OnRxTimeout\n\r");
  /* Update the State of the FSM*/
  State = RX_TIMEOUT;
  /* Run PingPong process in background*/
  UTIL_SEQ_SetTask((1 << CFG_SEQ_Task_SubGHz_Phy_App_Process), CFG_SEQ_Prio_0);
  /* USER CODE END OnRxTimeout */
}

static void OnRxError(void)
{
  /* USER CODE BEGIN OnRxError */
  APP_LOG(TS_ON, VLEVEL_L, "OnRxError\n\r");
  /* Update the State of the FSM*/
  State = RX_ERROR;
  /* Run PingPong process in background*/
  UTIL_SEQ_SetTask((1 << CFG_SEQ_Task_SubGHz_Phy_App_Process), CFG_SEQ_Prio_0);
  /* USER CODE END OnRxError */
}

/* USER CODE BEGIN PrFD */

static void PingPong_Process(void)
{
  Radio.Sleep();

  switch (State)
  {
    case RX:

      if (isMaster == true)
      {
        if (RxBufferSize > 0)
        {
          if (strncmp((const char *)BufferRx, PONG, sizeof(PONG) - 1) == 0)
          {
            UTIL_TIMER_Stop(&timerLed);
            /* switch off green led */
            /* Add delay between RX and TX */
            HAL_Delay(Radio.GetWakeupTime() + RX_TIME_MARGIN);
            /* master sends PING*/
//            APP_LOG(TS_ON, VLEVEL_L, "..."
//                    "PING"
//                    "\n\r");
						
						char tx_fre_temp[4];
						memset(tx_fre_temp, 0, 4);
						InttoHex(tx_fre_temp, SendFreq, 4);

						sprintf((char*)BufferTx,"PING%d+%s",tx_RssiValue,tx_fre_temp);
						
            APP_LOG(TS_ON, VLEVEL_L, "Master Tx start\n\r");
            //memcpy(BufferTx, PING, sizeof(PING) - 1);
            Radio.Send(BufferTx, PAYLOAD_LEN);
          }
          else /* valid reception but neither a PING or a PONG message */
          {
            /* Set device as master and start again */
						
            APP_LOG(TS_ON, VLEVEL_L, "Master Rx start\n\r");
            Radio.Rx(RX_TIMEOUT_VALUE);
          }
        }
      }
      else
      {
        if (RxBufferSize > 0)
        {
          if (strncmp((const char *)BufferRx, PING, sizeof(PING) - 1) == 0)
          {
            UTIL_TIMER_Stop(&timerLed);
            /* switch off red led */
            /* Add delay between RX and TX */
            HAL_Delay(Radio.GetWakeupTime() + RX_TIME_MARGIN);
            /*slave sends PONG*/
//            APP_LOG(TS_ON, VLEVEL_L, "..."
//                    "PONG"
//                    "\n\r");
            APP_LOG(TS_ON, VLEVEL_L, "Slave  Tx start\n\r");
            memcpy(BufferTx, PONG, sizeof(PONG) - 1);
            Radio.Send(BufferTx, PAYLOAD_LEN);
          }
        }
      }
      break;
    case TX:
      APP_LOG(TS_ON, VLEVEL_L, "Rx start\n\r");
      Radio.Rx(RX_TIMEOUT_VALUE);
      break;
    case RX_TIMEOUT:
    case RX_ERROR:
      if (isMaster == true)
      {
        /* Send the next PING frame */
        /* Add delay between RX and TX*/
        /* add random_delay to force sync between boards after some trials*/
        HAL_Delay(Radio.GetWakeupTime() + RX_TIME_MARGIN + random_delay);
        APP_LOG(TS_ON, VLEVEL_L, "Master Tx start\n\r");
        /* master sends PING*/
        memcpy(BufferTx, PING, sizeof(PING) - 1);
        Radio.Send(BufferTx, PAYLOAD_LEN);
      }
      else
      {
        APP_LOG(TS_ON, VLEVEL_L, "Slave Rx start\n\r");
        Radio.Rx(RX_TIMEOUT_VALUE);
      }
      break;
    case TX_TIMEOUT:
      APP_LOG(TS_ON, VLEVEL_L, "Slave Rx start\n\r");
      Radio.Rx(RX_TIMEOUT_VALUE);
      break;
    default:
      break;
  }
}


/* USER CODE BEGIN PrFD */

/* USER CODE END PrFD */
