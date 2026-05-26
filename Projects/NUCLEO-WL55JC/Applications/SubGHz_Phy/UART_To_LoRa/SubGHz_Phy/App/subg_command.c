/**
  * @file    subg_command.c
  * @brief   UART RX bridge to ra08-compatible AT parser (lora_transparent_at.c)
  */
#include "platform.h"
#include "subg_command.h"
#include "lora_transparent_at.h"
#include "stm32_adv_trace.h"

static void (*NotifyCb)(void) = NULL;

void CMD_Init(void (*CmdProcessNotify)(void))
{
  UTIL_ADV_TRACE_StartRxProcess(LoraTransparent_UartRxCb);
  if (CmdProcessNotify != NULL)
  {
    NotifyCb = CmdProcessNotify;
  }
}

void CMD_Process(void)
{
  LoraTransparent_CmdProcess();
}

void LoraTransparent_UartRxCb(uint8_t *rxChar, uint16_t size, uint8_t error)
{
  (void)size;
  (void)error;
  LoraTransparent_OnUartByte(*rxChar);
  if (NotifyCb != NULL)
  {
    NotifyCb();
  }
}
