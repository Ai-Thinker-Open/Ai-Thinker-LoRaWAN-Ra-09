/**
  ******************************************************************************
  * @file    sgfx_credentials_template.c
  * @author  MCD Application Team
  * @brief   manages keys and encryption algorithm
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
#include "sigfox_aes.h"
#include "st_sigfox_api.h"
#include "sgfx_credentials.h"
#include "se_nvm.h"

/* External variables ---------------------------------------------------------*/

/* Private typedef -----------------------------------------------------------*/
typedef struct manuf_device_info_s
{
  /* 16bits block 1 */
  sfx_u8 dev_id[MANUF_DEVICE_ID_LENGTH];
  sfx_u8 pac[MANUF_PAC_LENGTH];
  sfx_u8 ver[MANUF_VER_LENGTH];
  sfx_u8 spare1[MANUF_SPARE_1];
  /* 16bits block 2 */
  sfx_u8 dev_key[MANUF_DEVICE_KEY_LENGTH];
  /* 16bits block 3 */
  sfx_u8 spare2[MANUF_SPARE_2];
  sfx_u8 crc[MANUF_CRC_LENGTH];
} manuf_device_info_t;

/* Private define ------------------------------------------------------------*/
/*Sigfox defines*/
#define KEY_128_LEN               16 /*bytes*/
#define CREDENTIALS_VERSION       11
#define PUBLIC_KEY                {0x00, 0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77, 0x88, 0x99, 0xAA, 0xBB, 0xCC, 0xDD, 0xEE, 0xFF}
#define N_BLOCK_128               16

/*CREDENTIAL_KEY may be used to encrypt sigfox_data
  CREDENTIAL_KEY must be aligned with the sigfox tool generating and encrypting the sigfox_data*/
/*
#define CREDENTIAL_KEY {0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01}
*/

/* Private macro -------------------------------------------------------------*/
/*PREPROCESSOR CONVERSION*/
#define DECIMAL2STRING_DEF(s) #s
#define DECIMAL2STRING(s) DECIMAL2STRING_DEF(s)

/* Private variables ---------------------------------------------------------*/
static const char sgfxSeeLibVersion[] = "." DECIMAL2STRING(CREDENTIALS_VERSION);

extern sfx_u8 encrypted_sigfox_data[ sizeof(manuf_device_info_t) ];

static uint8_t device_public_key[] = PUBLIC_KEY;
static sigfox_aes_context AesContext;
static uint8_t session_key[KEY_128_LEN] = {0};

/* Private function prototypes -----------------------------------------------*/
static sfx_error_t CREDENTIALS_get_cra(sfx_u8 *decrypted_data, sfx_u8 *data_to_decrypt, sfx_u8 data_len);
static void CREDENTIALS_get_key(uint8_t *key, sfx_key_type_t KeyType);

/* Exported functions --------------------------------------------------------*/
sfx_error_t CREDENTIALS_aes_128_cbc_encrypt(uint8_t *encrypted_data, uint8_t *data_to_encrypt, uint8_t blocks)
{
  sfx_error_t retval = SFX_ERR_NONE;
  /* USER CODE BEGIN CREDENTIALS_aes_128_cbc_encrypt_1 */

  /* USER CODE END CREDENTIALS_aes_128_cbc_encrypt_1 */
  uint8_t iv[N_BLOCK_128] = {0x00};

  uint8_t key[AES_KEY_LEN];

  sfx_key_type_t KeyType = SE_NVM_get_key_type();

  CREDENTIALS_get_key(key, KeyType);

  sigfox_aes_set_key(key, AES_KEY_LEN,  &AesContext);
  /* Clear Key */
  memset(key, 0, AES_KEY_LEN);

  sigfox_aes_cbc_encrypt(data_to_encrypt,
                         encrypted_data,
                         blocks,
                         iv,
                         &AesContext);
  return retval;
}

sfx_error_t CREDENTIALS_aes_128_cbc_encrypt_with_session_key(uint8_t *encrypted_data, uint8_t *data_to_encrypt,
                                                             uint8_t blocks)
{
  sfx_error_t retval = SFX_ERR_NONE;
  /* USER CODE BEGIN CREDENTIALS_aes_128_cbc_encrypt_with_session_key_1 */

  /* USER CODE END CREDENTIALS_aes_128_cbc_encrypt_with_session_key_1 */
  uint8_t iv[N_BLOCK_128] = {0x00};

  sigfox_aes_set_key(session_key, AES_KEY_LEN,  &AesContext);

  sigfox_aes_cbc_encrypt(data_to_encrypt,
                         encrypted_data,
                         blocks,
                         iv,
                         &AesContext);

  return retval;
}

sfx_error_t CREDENTIALS_wrap_session_key(uint8_t *data, uint8_t blocks)
{
  sfx_error_t retval = SFX_ERR_NONE;
  /* USER CODE BEGIN CREDENTIALS_wrap_session_key_1 */

  /* USER CODE END CREDENTIALS_wrap_session_key_1 */
  uint8_t iv[N_BLOCK_128] = {0x00};

  uint8_t key[AES_KEY_LEN];

  CREDENTIALS_get_key(key, CREDENTIALS_KEY_PRIVATE);

  sigfox_aes_set_key(key, AES_KEY_LEN,  &AesContext);

  memset(key, 0, AES_KEY_LEN);

  sigfox_aes_cbc_encrypt(data,
                         session_key,
                         blocks,
                         iv,
                         &AesContext);

  return retval;
}

const char *CREDENTIALS_get_version(void)
{
  return sgfxSeeLibVersion;
}

void CREDENTIALS_get_dev_id(uint8_t *dev_id)
{
  manuf_device_info_t DeviceInfo;

  CREDENTIALS_get_cra((uint8_t *) &DeviceInfo, encrypted_sigfox_data, sizeof(manuf_device_info_t));

  memcpy(dev_id, DeviceInfo.dev_id, MANUF_DEVICE_ID_LENGTH);

  /*clear key*/
  memset(DeviceInfo.dev_key, 0, AES_KEY_LEN);
}

void CREDENTIALS_get_initial_pac(uint8_t *pac)
{
  manuf_device_info_t DeviceInfo;

  CREDENTIALS_get_cra((uint8_t *) &DeviceInfo, encrypted_sigfox_data, sizeof(manuf_device_info_t));
  /*clear key*/
  memcpy(pac, DeviceInfo.pac, MANUF_PAC_LENGTH);
  /*clear key*/
  memset(DeviceInfo.dev_key, 0, AES_KEY_LEN);
}

sfx_bool CREDENTIALS_get_payload_encryption_flag(void)
{
  sfx_bool ret = SFX_FALSE;

  ret = (sfx_bool) SE_NVM_get_encrypt_flag();
  return ret;
}

void CREDENTIALS_set_payload_encryption_flag(uint8_t enable)
{
  if (enable == 0)
  {
    SE_NVM_set_encrypt_flag(0);
  }
  else
  {
    SE_NVM_set_encrypt_flag(1);
  }
}
/* Private Functions Definition -----------------------------------------------*/
static void CREDENTIALS_get_key(uint8_t *key, sfx_key_type_t KeyType)
{
  switch (KeyType)
  {
    case CREDENTIALS_KEY_PUBLIC:
    {
      memcpy(key, device_public_key, AES_KEY_LEN);

      break;
    }
    case CREDENTIALS_KEY_PRIVATE:
    {
      manuf_device_info_t DeviceInfo;

      CREDENTIALS_get_cra((uint8_t *) &DeviceInfo, encrypted_sigfox_data, sizeof(manuf_device_info_t));

      memcpy(key, DeviceInfo.dev_key, AES_KEY_LEN);

      memset(DeviceInfo.dev_key, 0, AES_KEY_LEN);

      break;
    }
    default:
      break;
  }
}

static sfx_error_t CREDENTIALS_get_cra(sfx_u8 *decrypted_data, sfx_u8 *data_to_decrypt, sfx_u8 data_len)
{
#ifdef CREDENTIAL_KEY
  uint8_t iv[N_BLOCK_128] = {0x00};

  uint8_t CredentialKey[AES_KEY_LEN] = CREDENTIAL_KEY;

  /*device is provisioned with sigfox_data.h   */
  /*encrypted with CREDENTIAL_KEY in Sigfox Tool*/
  sigfox_aes_set_key(CredentialKey, AES_KEY_LEN,  &AesContext);

  memset(CredentialKey, 0, AES_KEY_LEN);

  sigfox_aes_cbc_decrypt(data_to_decrypt,
                         decrypted_data,
                         sizeof(manuf_device_info_t) / AES_KEY_LEN,
                         iv,
                         &AesContext);
#else
  /* default sigfox_data.h provided, sigfox_data.h is not encrypted*/
  memcpy((uint8_t *) decrypted_data, (uint8_t *) data_to_decrypt, sizeof(manuf_device_info_t));
#endif /* CREDENTIAL_KEY */
  return SFX_ERR_NONE;
}
