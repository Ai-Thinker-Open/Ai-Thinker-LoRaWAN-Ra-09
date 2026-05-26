/**
  * @file    lora_transparent_at.h
  * @brief   AT command + transparent LoRa (ra08 lora_transparent_lpuart_ADDR compatible)
  *
  * Replaces ST SubGHz_Phy AT stack (subg_at.c) for this application.
  * UART path: subg_command.c -> LoraTransparent_OnUartByte().
  */
#ifndef __LORA_TRANSPARENT_AT_H__
#define __LORA_TRANSPARENT_AT_H__

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

void LoraTransparent_PreInit(void);
void LoraTransparent_PostInit(void);
void LoraTransparent_OnUartByte(uint8_t byte);
void LoraTransparent_CmdProcess(void);
void LoraTransparent_RadioProcess(void);
void LoraTransparent_UartRxCb(uint8_t *rxChar, uint16_t size, uint8_t error);

#ifdef __cplusplus
}
#endif

#endif /* __LORA_TRANSPARENT_AT_H__ */
