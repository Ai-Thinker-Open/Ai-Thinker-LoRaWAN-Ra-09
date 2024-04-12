/**
  ******************************************************************************
  * @file    se_nvm_template.c
  * @author  MCD Application Team
  * @brief   manages SE nvm data
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2021 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */

/* Includes ------------------------------------------------------------------*/
#include <stdint.h>
#include <string.h>
#include "sgfx_eeprom_if.h"
#include "st_sigfox_api.h"
#include "se_nvm.h"
#include "sgfx_credentials.h"
/* External variables ---------------------------------------------------------*/
/* Private typedef -----------------------------------------------------------*/
/* Private define ------------------------------------------------------------*/
/* Private macro -------------------------------------------------------------*/
/* Private variables ---------------------------------------------------------*/
/* Private function prototypes -----------------------------------------------*/
/* Exported functions --------------------------------------------------------*/
sfx_u8 SE_NVM_get(sfx_u8 read_data[SFX_SE_NVMEM_BLOCK_SIZE])
{
  sfx_u8  ret = SFX_ERR_NONE;

  if (E2P_Read_SeNvm(read_data, SFX_SE_NVMEM_BLOCK_SIZE) != E2P_OK)
  {
    ret = SE_ERR_API_SE_NVM;
  }

  return ret;
}

sfx_u8 SE_NVM_set(sfx_u8 data_to_write[SFX_SE_NVMEM_BLOCK_SIZE])
{
  sfx_u8  ret = SFX_ERR_NONE;

  if (E2P_Write_SeNvm(data_to_write, SFX_SE_NVMEM_BLOCK_SIZE) != E2P_OK)
  {
    ret = SE_ERR_API_SE_NVM;
  }

  return ret;
}

sfx_key_type_t SE_NVM_get_key_type(void)
{
  return E2P_Read_KeyType();
}

void  SE_NVM_set_key_type(sfx_key_type_t keyType)
{
  E2P_Write_KeyType(keyType);
}

sfx_u8 SE_NVM_get_encrypt_flag(void)
{
  return E2P_Read_EncryptionFlag();
}

void  SE_NVM_set_encrypt_flag(sfx_u8 encryption_flag)
{
  E2P_Write_EncryptionFlag(encryption_flag);
}
