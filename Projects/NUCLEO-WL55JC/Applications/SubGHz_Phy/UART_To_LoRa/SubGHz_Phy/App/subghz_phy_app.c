/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file    subghz_phy_app.c
  * @brief   UART_To_LoRa — ra08-compatible AT + transparent LoRa
  ******************************************************************************
  */
/* USER CODE END Header */

#include "platform.h"
#include "sys_app.h"
#include "subghz_phy_app.h"
#include "stm32_timer.h"
#include "stm32_seq.h"
#include "utilities_def.h"
#include "app_version.h"
#include "subghz_phy_version.h"
#include "subg_command.h"
#include "lora_transparent_at.h"

static void CmdProcessNotify(void);

void SubghzApp_Init(void)
{
  /* Init order aligned with SubGHz_Phy_AT_Slave: UART first, Radio on demand (AT+CTX). */
  LoraTransparent_PreInit();

  CMD_Init(CmdProcessNotify);

  UTIL_SEQ_RegTask((1 << CFG_SEQ_Task_Vcom), UTIL_SEQ_RFU, CMD_Process);
  UTIL_SEQ_RegTask((1 << CFG_SEQ_Task_LoraProcess), UTIL_SEQ_RFU, LoraTransparent_RadioProcess);

  LoraTransparent_PostInit();

  APP_LOG(TS_OFF, VLEVEL_M, "APPLICATION_VERSION: V%X.%X.%X\r\n",
          (uint8_t)(APP_VERSION_MAIN),
          (uint8_t)(APP_VERSION_SUB1),
          (uint8_t)(APP_VERSION_SUB2));

  APP_LOG(TS_OFF, VLEVEL_M, "MW_RADIO_VERSION:    V%X.%X.%X\r\n",
          (uint8_t)(SUBGHZ_PHY_VERSION_MAIN),
          (uint8_t)(SUBGHZ_PHY_VERSION_SUB1),
          (uint8_t)(SUBGHZ_PHY_VERSION_SUB2));

  APP_PPRINTF("Available commands (ra08 compatible):\r\n");
  APP_PPRINTF("  AT+CTX=<freq>,<data_rate>,<bandwidth>,<code_rate>,<pwr>,<iqconverted>\r\n");
  APP_PPRINTF("  AT+CADDR=<local_addr>\r\n");
  APP_PPRINTF("  AT+CTXADDR=<target_addr>\r\n");
  APP_PPRINTF("  AT+CSLEEP=<sleep_mode>\r\n");
  APP_PPRINTF("  AT+CSTDBY=<standby_mode>\r\n");
  APP_PPRINTF("  AT+CTXCW=<freq>,<pwr>\r\n");
  APP_PPRINTF("  AT (CRLF) -> OK\r\n");
  APP_PPRINTF("  Transparent: AT+CTX then data; +++ returns AT_MODE\r\n");
}

static void CmdProcessNotify(void)
{
  UTIL_SEQ_SetTask(1 << CFG_SEQ_Task_Vcom, CFG_SEQ_Prio_1);
}
