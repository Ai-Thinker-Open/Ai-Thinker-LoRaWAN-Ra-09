/**
  ******************************************************************************
  * @file    kms_platf_objects.c
  * @author  MCD Application Team
  * @brief   This file contains definitions for Key Management Services (KMS)
  *          module platform objects management
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2019 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file in
  * the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */

/* Includes ------------------------------------------------------------------*/
#include "kms.h"                          /* PKCS11 definitions */
#if defined(KMS_ENABLED)
#include "kms_init.h"                     /* KMS session services */
#include "kms_platf_objects.h"            /* KMS platform objects services */
#include "kms_nvm_storage.h"              /* KMS NVM storage services */
#include "kms_vm_storage.h"               /* KMS VM storage services */
#define KMS_PLATF_OBJECTS_C
#include "kms_platf_objects_config.h"     /* KMS embedded objects definitions */
#if defined(KMS_ENCRYPT_DECRYPT_BLOB)
#include "CryptoApi/ca.h"                 /* Crypto API services */
#include "kms_mem.h"                      /* KMS memory utilities */
#include "kms_low_level.h"                /* Random data generation */
#endif /* KMS_ENCRYPT_DECRYPT_BLOB */

/*
 * Key ranges verification
 */
#if (KMS_INDEX_MIN_EMBEDDED_OBJECTS > KMS_INDEX_MAX_EMBEDDED_OBJECTS)
#error "Embedded objects index min and max are not well ordered"
#endif /* KMS_INDEX_MIN_EMBEDDED_OBJECTS > KMS_INDEX_MAX_EMBEDDED_OBJECTS */
#ifdef KMS_NVM_ENABLED
#if (KMS_INDEX_MIN_NVM_STATIC_OBJECTS > KMS_INDEX_MAX_NVM_STATIC_OBJECTS)
#error "NVM static ID objects index min and max are not well ordered"
#endif /* KMS_INDEX_MIN_NVM_STATIC_OBJECTS > KMS_INDEX_MAX_NVM_STATIC_OBJECTS */
#if (KMS_INDEX_MAX_EMBEDDED_OBJECTS >= KMS_INDEX_MIN_NVM_STATIC_OBJECTS)
#error "NVM static IDs & Embedded ranges are overlapping"
#endif /* KMS_INDEX_MAX_EMBEDDED_OBJECTS >= KMS_INDEX_MIN_NVM_STATIC_OBJECTS */
#if (KMS_NVM_SLOT_NUMBERS < (KMS_INDEX_MAX_NVM_STATIC_OBJECTS-KMS_INDEX_MIN_NVM_STATIC_OBJECTS+1))
#error "Not enough slot declared in KMS_NVM_SLOT_NUMBERS to store all allowed NVM Static IDs objects"
#endif /* KMS_NVM_SLOT_NUMBERS < (KMS_INDEX_MAX_NVM_STATIC_OBJECTS-KMS_INDEX_MIN_NVM_STATIC_OBJECTS+1) */
#ifdef KMS_NVM_DYNAMIC_ENABLED
#if (KMS_INDEX_MIN_NVM_DYNAMIC_OBJECTS > KMS_INDEX_MAX_NVM_DYNAMIC_OBJECTS)
#error "NVM dynamic ID objects index min and max are not well ordered"
#endif /* KMS_INDEX_MIN_NVM_DYNAMIC_OBJECTS > KMS_INDEX_MAX_NVM_DYNAMIC_OBJECTS */
#if (KMS_INDEX_MAX_NVM_STATIC_OBJECTS >= KMS_INDEX_MIN_NVM_DYNAMIC_OBJECTS)
#error "NVM static IDs & Dynamic IDs ranges are overlapping"
#endif /* KMS_INDEX_MAX_NVM_STATIC_OBJECTS >= KMS_INDEX_MIN_NVM_DYNAMIC_OBJECTS */

#if (KMS_NVM_SLOT_NUMBERS < (KMS_INDEX_MAX_NVM_DYNAMIC_OBJECTS-KMS_INDEX_MIN_NVM_STATIC_OBJECTS+1))
#error "Not enough slot declared in KMS_NVM_SLOT_NUMBERS to store all allowed NVM Static & dynamic IDs objects"
#endif /* KMS_NVM_SLOT_NUMBERS < (KMS_INDEX_MAX_NVM_DYNAMIC_OBJECTS-KMS_INDEX_MIN_NVM_STATIC_OBJECTS+1) */
#endif /* KMS_NVM_DYNAMIC_ENABLED */
#endif /* KMS_NVM_ENABLED */
#ifdef KMS_VM_DYNAMIC_ENABLED
#if (KMS_INDEX_MIN_VM_DYNAMIC_OBJECTS > KMS_INDEX_MAX_VM_DYNAMIC_OBJECTS)
#error "VM dynamic ID objects index min and max are not well ordered"
#endif /* KMS_INDEX_MIN_VM_DYNAMIC_OBJECTS > KMS_INDEX_MAX_VM_DYNAMIC_OBJECTS */
#if defined(KMS_NVM_ENABLED)
#if (KMS_INDEX_MAX_NVM_STATIC_OBJECTS >= KMS_INDEX_MIN_VM_DYNAMIC_OBJECTS)
#error "NVM static IDs & VM Dynamic IDs ranges are overlapping"
#endif /* KMS_INDEX_MAX_NVM_STATIC_OBJECTS >= KMS_INDEX_MIN_VM_DYNAMIC_OBJECTS */
#else /* KMS_NVM_DYNAMIC_ENABLED */
#if (KMS_INDEX_MAX_EMBEDDED_OBJECTS >= KMS_INDEX_MIN_VM_DYNAMIC_OBJECTS)
#error "Embedded IDs & VM Dynamic IDs ranges are overlapping"
#endif /* KMS_INDEX_MAX_EMBEDDED_OBJECTS >= KMS_INDEX_MIN_VM_DYNAMIC_OBJECTS */
#endif /* KMS_NVM_DYNAMIC_ENABLED */

#endif /* KMS_VM_DYNAMIC_ENABLED */

/** @addtogroup Key_Management_Services Key Management Services (KMS)
  * @{
  */

/** @addtogroup KMS_PLATF Platform Objects
  * @{
  */

/* Private types -------------------------------------------------------------*/
/* Private define ------------------------------------------------------------*/
#if defined(KMS_ENCRYPT_DECRYPT_BLOB)
#define ENCRYPT_DECRYPT_BLOB_KEY_LENGTH CA_CRL_AES128_KEY
#define ENCRYPT_DECRYPT_BLOB_IV_LENGTH  12
#define ENCRYPT_DECRYPT_BLOB_TAG_LENGTH 16
#define AES_BLOCK_SIZE                  16
#define PADDING_VALUE                   0xFF
#endif /* KMS_ENCRYPT_DECRYPT_BLOB */

/* Private macro -------------------------------------------------------------*/
/* Private variables ---------------------------------------------------------*/
/** @addtogroup KMS_PLATF_Private_Variables Private Variables
  * @{
  */
#ifdef KMS_NVM_ENABLED
/**
  * @brief  NVM initialization status
  */
static uint32_t kms_platf_nvm_initialisation_done = 0UL;

/**
  * @brief  NVM static objects access variables
  * @note   This "cache" table is used to speed up access to NVM static objects
  */
static kms_obj_keyhead_t *KMS_PlatfObjects_NvmStaticList[KMS_INDEX_MAX_NVM_STATIC_OBJECTS -
                                                         KMS_INDEX_MIN_NVM_STATIC_OBJECTS + 1];
#ifdef KMS_NVM_DYNAMIC_ENABLED
/**
  * @brief  NVM dynamic objects access variables
  * @note   This "cache" table is used to speed up access to NVM dynamic objects
  */
static kms_obj_keyhead_t *KMS_PlatfObjects_NvmDynamicList[KMS_INDEX_MAX_NVM_DYNAMIC_OBJECTS -
                                                          KMS_INDEX_MIN_NVM_DYNAMIC_OBJECTS + 1];
#endif /* KMS_NVM_DYNAMIC_ENABLED */
#endif /* KMS_NVM_ENABLED */

#ifdef KMS_VM_DYNAMIC_ENABLED
/**
  * @brief  VM initialization status
  */
static uint32_t kms_platf_vm_initialisation_done = 0UL;

/**
  * @brief  VM dynamic objects access variables
  * @note   This "cache" table is used to speed up access to VM dynamic objects
  */
static kms_obj_keyhead_t *KMS_PlatfObjects_VmDynamicList[KMS_INDEX_MAX_VM_DYNAMIC_OBJECTS -
                                                         KMS_INDEX_MIN_VM_DYNAMIC_OBJECTS + 1];
#endif /* KMS_VM_DYNAMIC_ENABLED */
/**
  * @}
  */
/* Private function prototypes -----------------------------------------------*/
#ifdef KMS_NVM_ENABLED
static void KMS_PlatfObjects_NvmStaticObjectList(void);
#endif  /* KMS_NVM_ENABLED */
#ifdef KMS_NVM_DYNAMIC_ENABLED
static void KMS_PlatfObjects_NvmDynamicObjectList(void);
#endif  /* KMS_NVM_DYNAMIC_ENABLED */
#ifdef KMS_VM_DYNAMIC_ENABLED
static void KMS_PlatfObjects_VmDynamicObjectList(void);
#endif  /* KMS_VM_DYNAMIC_ENABLED */
#if defined(KMS_ENCRYPT_DECRYPT_BLOB)
static void KMS_PlatfObjects_InitBlobEncryptDecryptContext(CA_AESGCMctx_stt *AESGCMctx,
                                                           kms_obj_keyhead_t *pEncryptedObject,
                                                           uint8_t *pIv);
static uint32_t KMS_PlatfObjects_GetPaddingLength(kms_obj_keyhead_t *pObject);
static void KMS_PlatfObjects_AllocateDecryptedObject(CK_SESSION_HANDLE hSession,
                                                     kms_obj_keyhead_t *pSrcObject,
                                                     kms_obj_keyhead_t **ppDestObject,
                                                     uint32_t *pObjectSize);
static void KMS_PlatfObjects_AllocateEncryptedObject(CK_SESSION_HANDLE hSession,
                                                     kms_obj_keyhead_t *pSrcObject,
                                                     kms_obj_keyhead_t **ppDestObject,
                                                     uint32_t *pObjectSize);
static void KMS_PlatfObjects_FreeObject(CK_SESSION_HANDLE hSession,
                                        kms_obj_keyhead_t **ppDestObject);
static void KMS_PlatfObjects_GetTagInObjectForNvm(kms_obj_keyhead_t *pObject,
                                                  uint8_t **ppTag);
#endif /* KMS_ENCRYPT_DECRYPT_BLOB */
/* Private function ----------------------------------------------------------*/
/** @addtogroup KMS_PLATF_Private_Functions Private Functions
  * @{
  */
#ifdef KMS_NVM_ENABLED
/**
  * @brief  Update @ref KMS_PlatfObjects_NvmStaticList with NVM contents
  * @retval None
  */
static void KMS_PlatfObjects_NvmStaticObjectList(void)
{
  nvms_error_t  nvms_rv;
  size_t nvms_data_size;
  kms_obj_keyhead_t *p_nvms_data;

  /* Load the KMS_PlatfObjects_NvmStaticList[], used to store buffer to NVM  */
  /* This should save processing time  */
  for (uint32_t i = KMS_INDEX_MIN_NVM_STATIC_OBJECTS; i <= KMS_INDEX_MAX_NVM_STATIC_OBJECTS; i++)
  {
    /* Read values from NVM */
    nvms_rv = NVMS_GET_DATA(i - KMS_INDEX_MIN_NVM_STATIC_OBJECTS, &nvms_data_size, (uint8_t **)(uint32_t)&p_nvms_data);

    if ((nvms_data_size != 0UL) && (nvms_rv == NVMS_NOERROR))
    {
      KMS_PlatfObjects_NvmStaticList[i - KMS_INDEX_MIN_NVM_STATIC_OBJECTS] = p_nvms_data;
    }
    else
    {
      KMS_PlatfObjects_NvmStaticList[i - KMS_INDEX_MIN_NVM_STATIC_OBJECTS] = NULL;
    }
  }
}
#endif  /* KMS_NVM_ENABLED */

#ifdef KMS_NVM_DYNAMIC_ENABLED
/**
  * @brief  Update @ref KMS_PlatfObjects_NvmDynamicList with NVM contents
  * @retval None
  */
static void KMS_PlatfObjects_NvmDynamicObjectList(void)
{
  nvms_error_t  nvms_rv;
  size_t nvms_data_size;
  kms_obj_keyhead_t *p_nvms_data;

  /* Load the KMS_PlatfObjects_NvmDynamicList[], used to store buffer to NVM  */
  /* This should save processing time  */
  for (uint32_t i = KMS_INDEX_MIN_NVM_DYNAMIC_OBJECTS; i <= KMS_INDEX_MAX_NVM_DYNAMIC_OBJECTS; i++)
  {
    /* Read values from NVM */
    nvms_rv = NVMS_GET_DATA(i - KMS_INDEX_MIN_NVM_STATIC_OBJECTS, &nvms_data_size, (uint8_t **)(uint32_t)&p_nvms_data);

    if ((nvms_data_size != 0UL) && (nvms_rv == NVMS_NOERROR))
    {
      KMS_PlatfObjects_NvmDynamicList[i - KMS_INDEX_MIN_NVM_DYNAMIC_OBJECTS] = p_nvms_data;
    }
    else
    {
      KMS_PlatfObjects_NvmDynamicList[i - KMS_INDEX_MIN_NVM_DYNAMIC_OBJECTS] = NULL;
    }

  }
}
#endif  /* KMS_NVM_DYNAMIC_ENABLED */

#ifdef KMS_VM_DYNAMIC_ENABLED
/**
  * @brief  Update @ref KMS_PlatfObjects_VmDynamicList with VM contents
  * @retval None
  */
static void KMS_PlatfObjects_VmDynamicObjectList(void)
{
  vms_error_t  vms_rv;
  size_t vms_data_size;
  kms_obj_keyhead_t *p_vms_data;

  /* Load the KMS_PlatfObjects_VmDynamicList[], used to store buffer to VM  */
  /* This should save processing time  */
  for (uint32_t i = KMS_INDEX_MIN_VM_DYNAMIC_OBJECTS; i <= KMS_INDEX_MAX_VM_DYNAMIC_OBJECTS; i++)
  {
    /* Read values from VM */
    vms_rv = VMS_GET_DATA(i - KMS_INDEX_MIN_VM_DYNAMIC_OBJECTS, &vms_data_size, (uint8_t **)(uint32_t)&p_vms_data);

    if ((vms_data_size != 0UL) && (vms_rv == VMS_NOERROR))
    {
      KMS_PlatfObjects_VmDynamicList[i - KMS_INDEX_MIN_VM_DYNAMIC_OBJECTS] = p_vms_data;
    }
    else
    {
      KMS_PlatfObjects_VmDynamicList[i - KMS_INDEX_MIN_VM_DYNAMIC_OBJECTS] = NULL;
    }

  }
}
#endif  /* KMS_VM_DYNAMIC_ENABLED */

#if defined(KMS_ENCRYPT_DECRYPT_BLOB)
/**
  * @brief  Initialize the data required for the AES GCM encryption/decryption
  * @param  pAESCtx AES GCM context
  * @param  pKey AES GCM key (size: ENCRYPT_DECRYPT_BLOB_KEY_LENGTH)
  * @param  pKey AES GCM IV (size: ENCRYPT_DECRYPT_BLOB_IV_LENGTH)
  * @retval None
  */
static void KMS_PlatfObjects_InitBlobEncryptDecryptContext(CA_AESGCMctx_stt *AESGCMctx,
                                                           kms_obj_keyhead_t *pEncryptedObject,
                                                           uint8_t *pIv)
{
  uint32_t current_size;

  /* Set flag field to default value */
  AESGCMctx->mFlags = CA_E_SK_DEFAULT;
  /* Set key size */
  AESGCMctx->mKeySize = ENCRYPT_DECRYPT_BLOB_KEY_LENGTH;

  /* Read value from the secure RAM. */
  AESGCMctx->pmKey = (uint8_t*)(KMS_DATASTORAGE_KEY_START);

  /* Set nonce size field to iv_length, note that valid values are 7,8,9,10,11,12,13*/
  AESGCMctx->mIvSize = ENCRYPT_DECRYPT_BLOB_IV_LENGTH;

  /* Size of returned authentication TAG */
  AESGCMctx->mTagSize = ENCRYPT_DECRYPT_BLOB_TAG_LENGTH;

  /* Build a diversified IV: use the IEEE 64-bit unique device ID */
  memcpy(pIv, (void*)(UID64_BASE), sizeof(uint32_t));
  current_size = sizeof(uint32_t);
  /* Build a diversified IV: use the random number and handle the endianness */
  pIv[current_size]     = ((pEncryptedObject->blobs_count) & 0x00FF0000UL) >> 16;
  pIv[current_size + 1] = ((pEncryptedObject->blobs_count) & 0xFF000000UL) >> 24;
  pIv[current_size + 2] = ((pEncryptedObject->object_id)   & 0x00FF0000UL) >> 16;
  pIv[current_size + 3] = ((pEncryptedObject->object_id)   & 0xFF000000UL) >> 24;
  current_size = current_size + sizeof(uint32_t);
  /* Build a diversified IV: use the unique device ID */
  memcpy(&pIv[current_size], (void*)(UID_BASE), ENCRYPT_DECRYPT_BLOB_IV_LENGTH - current_size);
}

/**
  * @brief  Get the padding length required for the blobs
  * @param  pObject KMS object with additional data for blob encryption/decryption
  * @retval uint32_t
  */
static uint32_t KMS_PlatfObjects_GetPaddingLength(kms_obj_keyhead_t *pObject)
{
  uint32_t object_length;
  uint32_t padding_length;

  /* Compute the padding length */
  object_length = sizeof(kms_obj_keyhead_no_blob_t) + pObject->blobs_size;
  padding_length = object_length % AES_BLOCK_SIZE;
  if (padding_length != 0)
  {
    padding_length = AES_BLOCK_SIZE - padding_length;
  }
  return padding_length;
}

/**
  * @brief  Allocate a KMS object without additional data for blob encryption/decryption
  * @param  hSession Session handle
  * @param  pSrcObject Object providing the header to copy
  * @param  ppDestObject Object to allocate
  * @param  pDestObjectSize Size of the allocated object
  * @retval None
  */
static void KMS_PlatfObjects_AllocateDecryptedObject(CK_SESSION_HANDLE hSession,
                                                     kms_obj_keyhead_t *pSrcObject,
                                                     kms_obj_keyhead_t **ppDestObject,
                                                     uint32_t *pDestObjectSize)
{
  uint32_t padding_length;
  uint32_t padding_start_index;
  uint32_t padding_index;
  uint8_t *p_kms_object;

  /* Compute the padding length */
  padding_length = KMS_PlatfObjects_GetPaddingLength(pSrcObject);

  /* Allocate memory for the object only (no tag) */
  *pDestObjectSize = sizeof(kms_obj_keyhead_no_blob_t) +
                     pSrcObject->blobs_size +
                     padding_length;
  *ppDestObject = KMS_Alloc(hSession, *pDestObjectSize);

  /* Fill the header of the allocated object */
  if (*ppDestObject != NULL)
  {
    (void)memcpy((void*)*ppDestObject, (void*)pSrcObject, sizeof(kms_obj_keyhead_no_blob_t));
  }

  /* Remove the IV from blobs_count and object_id */
  (*ppDestObject)->blobs_count = (((*ppDestObject)->blobs_count) & 0x0000FFFFUL);
  (*ppDestObject)->object_id   = (((*ppDestObject)->object_id)   & 0x0000FFFFUL);

  /* Fill the padding with the default value */
  if (padding_length != 0)
  {
    padding_start_index = sizeof(kms_obj_keyhead_no_blob_t) + pSrcObject->blobs_size;
    p_kms_object = (uint8_t*)*ppDestObject;
    for (padding_index = padding_start_index ; padding_index < padding_start_index + padding_length; padding_index++)
    {
      p_kms_object[padding_index] = PADDING_VALUE;
    }
  }
}

/**
  * @brief  Allocate a KMS object with additional data for blob encryption/decryption
  * @param  hSession Session handle
  * @param  pSrcObject Object providing the header to copy
  * @param  ppDestObject Object to allocate
  * @param  pDestObjectSize Size of the allocated object
  * @retval None
  */
static void KMS_PlatfObjects_AllocateEncryptedObject(CK_SESSION_HANDLE hSession,
                                                     kms_obj_keyhead_t *pSrcObject,
                                                     kms_obj_keyhead_t **ppDestObject,
                                                     uint32_t *pDestObjectSize)
{
  uint32_t padding_length;
  uint32_t padding_start_index;
  uint32_t padding_index;
  uint32_t random_data;
  uint8_t *p_kms_object;

  /* Generate the variable part of the IV */
  if (KMS_LL_GetRandomData(&random_data) != CKR_OK)
  {
    *pDestObjectSize = 0;
    *ppDestObject = NULL;
    return;
  }

  /* Compute the padding length */
  padding_length = KMS_PlatfObjects_GetPaddingLength(pSrcObject);

  /* Allocate memory for the object and the blob encryption/decryption data */
  *pDestObjectSize = sizeof(kms_obj_keyhead_no_blob_t) +
                     pSrcObject->blobs_size            +
                     padding_length                    +
                     ENCRYPT_DECRYPT_BLOB_TAG_LENGTH;
  *ppDestObject = KMS_Alloc(hSession, *pDestObjectSize);

  /* Fill the header of the allocated object */
  if (*ppDestObject != NULL)
  {
    (void)memcpy((void*)*ppDestObject, (void*)pSrcObject, sizeof(kms_obj_keyhead_no_blob_t));
  }

  /* Store the variable part of the IV */
  (*ppDestObject)->blobs_count = ((random_data & 0x0000FFFFUL) << 16) | (((*ppDestObject)->blobs_count) & 0x0000FFFFUL);
  (*ppDestObject)->object_id   = ((random_data & 0xFFFF0000UL)      ) | (((*ppDestObject)->object_id)   & 0x0000FFFFUL);

  /* Fill the padding with the default value */
  if (padding_length != 0)
  {
    padding_start_index = sizeof(kms_obj_keyhead_no_blob_t) + pSrcObject->blobs_size;
    p_kms_object = (uint8_t*)*ppDestObject;
    for (padding_index = padding_start_index ; padding_index < padding_start_index + padding_length; padding_index++)
    {
      p_kms_object[padding_index] = PADDING_VALUE;
    }
  }
}

/**
  * @brief  Free the KMS object with or without additional data for blob encryption/decryption
  * @param  hSession Session handle
  * @param  pDestObject Object to allocate
  * @retval None
  */
static void KMS_PlatfObjects_FreeObject(CK_SESSION_HANDLE hSession,
                                        kms_obj_keyhead_t **ppDestObject)
{
  if (*ppDestObject != NULL)
  {
    /* Free memory */
    KMS_Free(hSession, *ppDestObject);
    *ppDestObject = NULL;
  }
}

/**
  * @brief  Get the tag in the KMS object with additional data for blob encryption/decryption
  * @param  pObject KMS object with additional data for blob encryption/decryption
  * @param  ppTag Tag
  * @retval None
  */
static void KMS_PlatfObjects_GetTagInObjectForNvm(kms_obj_keyhead_t *pObject,
                                                  uint8_t **ppTag)
{
  uint32_t padding_length;

  if ((pObject == NULL) || (ppTag == NULL))
  {
    return;
  }

  /* Compute the padding length */
  padding_length = KMS_PlatfObjects_GetPaddingLength(pObject);

  /* Get the address of the tag */
  *ppTag = ((uint8_t *)(pObject->blobs)) + pObject->blobs_size + padding_length;
}
#endif /* KMS_ENCRYPT_DECRYPT_BLOB */
/**
  * @}
  */
/* Exported functions --------------------------------------------------------*/

/** @addtogroup KMS_PLATF_Exported_Functions Exported Functions
  * @{
  */

/**
  * @brief  Returns range of embedded objects
  * @param  pMin Embedded objects min ID
  * @param  pMax Embedded objects max ID
  * @retval None
  */
void KMS_PlatfObjects_EmbeddedRange(uint32_t *pMin, uint32_t *pMax)
{
  *pMin = KMS_INDEX_MIN_EMBEDDED_OBJECTS;
  *pMax = KMS_INDEX_MAX_EMBEDDED_OBJECTS;
}

/**
  * @brief  Returns embedded object corresponding to given key handle
  * @param  hKey key handle
  * @retval Corresponding object
  */
kms_obj_keyhead_t *KMS_PlatfObjects_EmbeddedObject(uint32_t hKey)
{
  return (kms_obj_keyhead_t *)(uint32_t)KMS_PlatfObjects_EmbeddedList[hKey - KMS_INDEX_MIN_EMBEDDED_OBJECTS];
}

#ifdef KMS_NVM_ENABLED
/**
  * @brief  Returns range of NVM static objects
  * @param  pMin NVM static objects min ID
  * @param  pMax NVM static objects max ID
  * @retval None
  */
void KMS_PlatfObjects_NvmStaticRange(uint32_t *pMin, uint32_t *pMax)
{
  *pMin = KMS_INDEX_MIN_NVM_STATIC_OBJECTS;
  *pMax = KMS_INDEX_MAX_NVM_STATIC_OBJECTS;
}
#endif  /* KMS_NVM_ENABLED */

#ifdef KMS_NVM_ENABLED
/**
  * @brief  Returns NVM static object corresponding to given key handle
  * @param  hKey key handle
  * @retval Corresponding object
  */
kms_obj_keyhead_t *KMS_PlatfObjects_NvmStaticObject(uint32_t hKey)
{
#if defined(KMS_ENCRYPT_DECRYPT_BLOB)
  CA_AESGCMctx_stt ca_ctx;
  int32_t status;
  kms_obj_keyhead_t *p_encrypted_object;
  kms_obj_keyhead_t *p_decrypted_object = NULL;
  uint8_t iv[ENCRYPT_DECRYPT_BLOB_IV_LENGTH];
  int32_t tag_length;
  uint32_t decrypted_object_size;

  /* Get the encrypted object in memory */
  p_encrypted_object = KMS_PlatfObjects_NvmStaticList[hKey - KMS_INDEX_MIN_NVM_STATIC_OBJECTS];

  if (p_encrypted_object != NULL)
  {
    /* Allocate memory for the decrypted object */
    KMS_PlatfObjects_AllocateDecryptedObject(KMS_SESSION_ID_INTERNAL | hKey,
                                             p_encrypted_object,
                                             &p_decrypted_object,
                                             &decrypted_object_size);

    if (p_decrypted_object == NULL)
    {
      status = CA_AES_ERR_BAD_PARAMETER;
    }
    else
    {
      /* Configure the AES context */
      KMS_PlatfObjects_InitBlobEncryptDecryptContext(&ca_ctx, p_encrypted_object, iv);

      /* Get the tag */
      KMS_PlatfObjects_GetTagInObjectForNvm(p_encrypted_object, (uint8_t**)&(ca_ctx.pmTag));
      tag_length = ENCRYPT_DECRYPT_BLOB_TAG_LENGTH;

      /* Decrypt the encrypted object */
      status = CA_AES_GCM_Decrypt(&ca_ctx,
                                  (uint8_t*)(KMS_DATASTORAGE_KEY_START),
                                  iv,
                                  (uint8_t *)p_decrypted_object,
                                  (int32_t)sizeof(kms_obj_keyhead_no_blob_t),
                                  (uint8_t *)p_encrypted_object->blobs,
                                  (int32_t)p_encrypted_object->blobs_size,
                                  (uint8_t *)p_decrypted_object->blobs,
                                  (int32_t *)&(p_decrypted_object->blobs_size),
                                  NULL,
                                  &tag_length);

      if (status != CA_AES_SUCCESS)
      {
        KMS_PlatfObjects_FreeObject(KMS_SESSION_ID_INTERNAL | hKey,
                                    &p_decrypted_object);
      }
    }
  }
  return p_decrypted_object;
#else /* KMS_ENCRYPT_DECRYPT_BLOB */
  return KMS_PlatfObjects_NvmStaticList[hKey - KMS_INDEX_MIN_NVM_STATIC_OBJECTS];
#endif /* KMS_ENCRYPT_DECRYPT_BLOB */
}
#endif  /* KMS_NVM_ENABLED */

#ifdef KMS_NVM_DYNAMIC_ENABLED
/**
  * @brief  Returns range of NVM dynamic objects
  * @param  pMin NVM dynamic objects min ID
  * @param  pMax NVM dynamic objects max ID
  * @retval None
  */
void KMS_PlatfObjects_NvmDynamicRange(uint32_t *pMin, uint32_t *pMax)
{
  *pMin = KMS_INDEX_MIN_NVM_DYNAMIC_OBJECTS;
  *pMax = KMS_INDEX_MAX_NVM_DYNAMIC_OBJECTS;
}
#endif  /* KMS_NVM_DYNAMIC_ENABLED */

#ifdef KMS_NVM_DYNAMIC_ENABLED
/**
  * @brief  Returns NVM dynamic object corresponding to given key handle
  * @param  hKey key handle
  * @retval Corresponding object
  */
kms_obj_keyhead_t *KMS_PlatfObjects_NvmDynamicObject(uint32_t hKey)
{
#if defined(KMS_ENCRYPT_DECRYPT_BLOB)
  CA_AESGCMctx_stt ca_ctx;
  int32_t status;
  kms_obj_keyhead_t *p_encrypted_object;
  kms_obj_keyhead_t *p_decrypted_object = NULL;
  uint8_t iv[ENCRYPT_DECRYPT_BLOB_IV_LENGTH];
  int32_t tag_length;
  uint32_t decrypted_object_size;

  /* Get the encrypted object in memory */
  p_encrypted_object = KMS_PlatfObjects_NvmDynamicList[hKey - KMS_INDEX_MIN_NVM_DYNAMIC_OBJECTS];

  if (p_encrypted_object != NULL)
  {
    /* Allocate memory for the decrypted object */
    KMS_PlatfObjects_AllocateDecryptedObject(KMS_SESSION_ID_INTERNAL | hKey,
                                             p_encrypted_object,
                                             &p_decrypted_object,
                                             &decrypted_object_size);

    if (p_decrypted_object == NULL)
    {
      status = CA_AES_ERR_BAD_PARAMETER;
    }
    else
    {
      /* Configure the AES context */
      KMS_PlatfObjects_InitBlobEncryptDecryptContext(&ca_ctx, p_encrypted_object, iv);

      /* Get the tag */
      KMS_PlatfObjects_GetTagInObjectForNvm(p_encrypted_object, (uint8_t**)&(ca_ctx.pmTag));
      tag_length = ENCRYPT_DECRYPT_BLOB_TAG_LENGTH;

      status = CA_AES_GCM_Decrypt(&ca_ctx,
                                  (uint8_t*)(KMS_DATASTORAGE_KEY_START),
                                  iv,
                                  (uint8_t *)p_decrypted_object,
                                  (int32_t)sizeof(kms_obj_keyhead_no_blob_t),
                                  (uint8_t *)p_encrypted_object->blobs,
                                  (int32_t)p_encrypted_object->blobs_size,
                                  (uint8_t *)p_decrypted_object->blobs,
                                  (int32_t *)&(p_decrypted_object->blobs_size),
                                  NULL,
                                  &tag_length);

      if (status != CA_AES_SUCCESS)
      {
        KMS_PlatfObjects_FreeObject(KMS_SESSION_ID_INTERNAL | hKey,
                                       &p_decrypted_object);
      }
    }
  }
  return p_decrypted_object;
#else /* KMS_ENCRYPT_DECRYPT_BLOB */
  return KMS_PlatfObjects_NvmDynamicList[hKey - KMS_INDEX_MIN_NVM_DYNAMIC_OBJECTS];
#endif /* KMS_ENCRYPT_DECRYPT_BLOB */
}
#endif  /* KMS_NVM_DYNAMIC_ENABLED */

#ifdef KMS_VM_DYNAMIC_ENABLED
/**
  * @brief  Returns range of VM dynamic objects
  * @param  pMin VM dynamic objects min ID
  * @param  pMax VM dynamic objects max ID
  * @retval None
  */
void KMS_PlatfObjects_VmDynamicRange(uint32_t *pMin, uint32_t *pMax)
{
  *pMin = KMS_INDEX_MIN_VM_DYNAMIC_OBJECTS;
  *pMax = KMS_INDEX_MAX_VM_DYNAMIC_OBJECTS;
}
#endif  /* KMS_VM_DYNAMIC_ENABLED */

#ifdef KMS_VM_DYNAMIC_ENABLED
/**
  * @brief  Returns VM dynamic object corresponding to given key handle
  * @param  hKey key handle
  * @retval Corresponding object
  */
kms_obj_keyhead_t *KMS_PlatfObjects_VmDynamicObject(uint32_t hKey)
{
  return KMS_PlatfObjects_VmDynamicList[hKey - KMS_INDEX_MIN_VM_DYNAMIC_OBJECTS];
}
#endif  /* KMS_VM_DYNAMIC_ENABLED */

#if defined(KMS_NVM_DYNAMIC_ENABLED) || defined(KMS_VM_DYNAMIC_ENABLED)
/**
  * @brief  Search an available NVM / VM dynamic ID to store blob object
  * @param  pBlob  Blob object to store
  * @param  pObjId NVM / VM dynamic ID
  * @retval CKR_OK
  *         CKR_ARGUMENTS_BAD
  *         CKR_DEVICE_MEMORY
  *         @ref KMS_PlatfObjects_NvmStoreObject returned values
  *         @ref KMS_PlatfObjects_VmStoreObject returned values
  */
CK_RV KMS_PlatfObjects_AllocateAndStore(kms_obj_keyhead_no_blob_t *pBlob, CK_OBJECT_HANDLE_PTR pObjId)
{
  CK_OBJECT_HANDLE Index;
  CK_RV e_ret_status;

  if ((pObjId == NULL_PTR) || (pBlob == NULL_PTR))
  {
    e_ret_status = CKR_ARGUMENTS_BAD;
  }
  else
  {
    *pObjId = KMS_HANDLE_KEY_NOT_KNOWN;
#ifdef KMS_NVM_DYNAMIC_ENABLED
    /* Find a Free place in nvm dynamic table */
    for (Index = 0; Index <= (KMS_INDEX_MAX_NVM_DYNAMIC_OBJECTS - KMS_INDEX_MIN_NVM_DYNAMIC_OBJECTS); Index++)
    {
      if (KMS_PlatfObjects_NvmDynamicList[Index] == NULL)
      {
        *pObjId = Index + KMS_INDEX_MIN_NVM_DYNAMIC_OBJECTS;
        break;
      }
    }
#endif /* KMS_NVM_DYNAMIC_ENABLED */
#ifdef KMS_VM_DYNAMIC_ENABLED
    /* Find a Free place in vm dynamic table */
    for (Index = 0; Index <= (KMS_INDEX_MAX_VM_DYNAMIC_OBJECTS - KMS_INDEX_MIN_VM_DYNAMIC_OBJECTS); Index++)
    {
      if (KMS_PlatfObjects_VmDynamicList[Index] == NULL)
      {
        *pObjId = Index + KMS_INDEX_MIN_VM_DYNAMIC_OBJECTS;
        break;
      }
    }
#endif /* KMS_VM_DYNAMIC_ENABLED */
    if (*pObjId == KMS_HANDLE_KEY_NOT_KNOWN)
    {
      /* No place found in Dynamic List */
      e_ret_status = CKR_DEVICE_MEMORY;
    }
    else
    {
      /* Update object ID */
      pBlob->object_id = *pObjId;
#ifdef KMS_NVM_DYNAMIC_ENABLED
      /* Store in NVM storage */
      e_ret_status = KMS_PlatfObjects_NvmStoreObject(*pObjId,
                                                     (uint8_t *)pBlob,
                                                     pBlob->blobs_size + sizeof(kms_obj_keyhead_no_blob_t));
#endif /* KMS_NVM_DYNAMIC_ENABLED */
#ifdef KMS_VM_DYNAMIC_ENABLED
      /* Store in VM storage */
      e_ret_status = KMS_PlatfObjects_VmStoreObject(*pObjId,
                                                    (uint8_t *)pBlob,
                                                    pBlob->blobs_size + sizeof(kms_obj_keyhead_no_blob_t));
#endif /* KMS_VM_DYNAMIC_ENABLED */
      /* A Garbage collection generate a WARNING ==> Not an error */
      if (e_ret_status != CKR_OK)
      {
        *pObjId = KMS_HANDLE_KEY_NOT_KNOWN;
      }
    }
  }
  return e_ret_status;
}
#endif /* KMS_NVM_DYNAMIC_ENABLED || KMS_NVM_DYNAMIC_ENABLED */

#ifdef KMS_EXT_TOKEN_ENABLED
/**
  * @brief  Returns range of External token static objects
  * @param  pMin External token static objects min ID
  * @param  pMax External token static objects max ID
  * @retval None
  */
void KMS_PlatfObjects_ExtTokenStaticRange(uint32_t *pMin, uint32_t *pMax)
{
  *pMin = KMS_INDEX_MIN_EXT_TOKEN_STATIC_OBJECTS;
  *pMax = KMS_INDEX_MAX_EXT_TOKEN_STATIC_OBJECTS;
}
/**
  * @brief  Returns range of External token dynamic objects
  * @param  pMin External token dynamic objects min ID
  * @param  pMax External token dynamic objects max ID
  * @retval None
  */
void KMS_PlatfObjects_ExtTokenDynamicRange(uint32_t *pMin, uint32_t *pMax)
{
  *pMin = KMS_INDEX_MIN_EXT_TOKEN_DYNAMIC_OBJECTS;
  *pMax = KMS_INDEX_MAX_EXT_TOKEN_DYNAMIC_OBJECTS;
}
#endif /* KMS_EXT_TOKEN_ENABLED */


/**
  * @brief  Initialize platform objects
  * @note   Initialize NVM / VM storage and fill "cache" buffers
  * @retval None
  */
void KMS_PlatfObjects_Init(void)
{
#ifdef KMS_NVM_ENABLED
  /* The NVMS_Init should be done only once */
  if (kms_platf_nvm_initialisation_done == 0UL)
  {
    /* Initialize the NVMS */
    (void)NVMS_Init();
    kms_platf_nvm_initialisation_done = 1UL;
  }

  KMS_PlatfObjects_NvmStaticObjectList();
#ifdef KMS_NVM_DYNAMIC_ENABLED
  KMS_PlatfObjects_NvmDynamicObjectList();
#endif /* KMS_NVM_DYNAMIC_ENABLED */

#endif /* KMS_NVM_ENABLED */

#ifdef KMS_VM_DYNAMIC_ENABLED
  /* The VMS_Init should be done only once */
  if (kms_platf_vm_initialisation_done == 0UL)
  {
    /* Initialize the VMS */
    (void)VMS_Init();
    kms_platf_vm_initialisation_done = 1UL;
  }

  KMS_PlatfObjects_VmDynamicObjectList();
#endif /* KMS_VM_DYNAMIC_ENABLED */
}

/**
  * @brief  De-Initialize platform objects
  * @retval None
  */
void KMS_PlatfObjects_Finalize(void)
{
#ifdef KMS_NVM_ENABLED
  /* Finalize the NVMS */
  NVMS_Deinit();

  /* We must re-allow the call to NVMS_Init() */
  kms_platf_nvm_initialisation_done = 0UL;
#endif /* KMS_NVM_ENABLED */

#ifdef KMS_VM_DYNAMIC_ENABLED
  /* Finalize the VMS */
  VMS_Deinit();

  /* We must re-allow the call to VMS_Init() */
  kms_platf_vm_initialisation_done = 0UL;
#endif /* KMS_VM_DYNAMIC_ENABLED */
}

#ifdef KMS_NVM_ENABLED
/**
  * @brief  Store object in NVM storage
  * @note   Either static or dynamic objects
  * @param  ObjectId Object ID
  * @param  pObjectToAdd Object to add
  * @param  ObjectSize Object size
  * @retval CKR_OK if storage is successful
  *         CKR_DEVICE_MEMORY otherwise
  */
CK_RV KMS_PlatfObjects_NvmStoreObject(uint32_t ObjectId, uint8_t *pObjectToAdd,  uint32_t ObjectSize)
{
  nvms_error_t  rv;
  CK_RV e_ret_status;
#if defined(KMS_ENCRYPT_DECRYPT_BLOB)
  int32_t aes_status;
  kms_obj_keyhead_t *p_decrypted_object;
  kms_obj_keyhead_t *p_encrypted_object;
  CA_AESGCMctx_stt ca_ctx;
  uint8_t iv[ENCRYPT_DECRYPT_BLOB_IV_LENGTH];
  uint8_t *p_tag;
  int32_t tag_length;
  uint32_t encrypted_object_size;

  /* Initialize the status */
  e_ret_status = CKR_OK;

  /* Read the input data as a KMS object */
  p_decrypted_object = (kms_obj_keyhead_t *)pObjectToAdd;

  /* Allocate memory for the encrypted object */
  KMS_PlatfObjects_AllocateEncryptedObject(KMS_SESSION_ID_INTERNAL | ObjectId,
                                           p_decrypted_object,
                                           &p_encrypted_object,
                                           &encrypted_object_size);

  if (p_encrypted_object == NULL)
  {
    e_ret_status = CKR_DEVICE_MEMORY;
  }
  else
  {
    /* Configure the AES context */
    KMS_PlatfObjects_InitBlobEncryptDecryptContext(&ca_ctx, p_encrypted_object, iv);

    /* Get the tag */
    KMS_PlatfObjects_GetTagInObjectForNvm(p_encrypted_object, &p_tag);

    /* Perform the encryption */
    tag_length = ENCRYPT_DECRYPT_BLOB_TAG_LENGTH;
    aes_status = CA_AES_GCM_Encrypt(&ca_ctx,
                                    (uint8_t*)(KMS_DATASTORAGE_KEY_START),
                                    iv,
                                    (uint8_t *)p_decrypted_object,
                                    (int32_t)sizeof(kms_obj_keyhead_no_blob_t),
                                    (uint8_t *)p_decrypted_object->blobs,
                                    (int32_t)p_decrypted_object->blobs_size,
                                    (uint8_t *)p_encrypted_object->blobs,
                                    (int32_t *)&(p_encrypted_object->blobs_size),
                                    (uint8_t *)p_tag,
                                    &tag_length);

    if (aes_status == CA_AES_SUCCESS)
    {
      /* Update the object and its size */
      ObjectSize = encrypted_object_size;
      pObjectToAdd = (uint8_t *)p_encrypted_object;
    }
    else
    {
      /* Free the allocated object when needed */
      KMS_PlatfObjects_FreeObject(KMS_SESSION_ID_INTERNAL | ObjectId,
                                     &p_encrypted_object);
      e_ret_status = CKR_DEVICE_ERROR;
    }
  }

  if (e_ret_status != CKR_OK)
  {
    return e_ret_status;
  }
#endif /* KMS_ENCRYPT_DECRYPT_BLOB */

  /* It's a NVM STATIC object */
  if ((ObjectId >= KMS_INDEX_MIN_NVM_STATIC_OBJECTS) && (ObjectId <= KMS_INDEX_MAX_NVM_STATIC_OBJECTS))
  {
    rv = NVMS_WRITE_DATA(ObjectId - KMS_INDEX_MIN_NVM_STATIC_OBJECTS, ObjectSize, (const uint8_t *)pObjectToAdd);
  }
  else
  {
#ifdef KMS_NVM_DYNAMIC_ENABLED
    /* It's a NVM DYNAMIC object */
    if ((ObjectId >= KMS_INDEX_MIN_NVM_DYNAMIC_OBJECTS) && (ObjectId <= KMS_INDEX_MAX_NVM_DYNAMIC_OBJECTS))
    {
      rv = NVMS_WRITE_DATA(ObjectId - KMS_INDEX_MIN_NVM_STATIC_OBJECTS, ObjectSize, (const uint8_t *)pObjectToAdd);
    }
    else
    {
      rv = NVMS_SLOT_INVALID;
    }
#else /* KMS_NVM_DYNAMIC_ENABLED */
    rv = NVMS_SLOT_INVALID;
#endif /* KMS_NVM_DYNAMIC_ENABLED */
  }
#if defined(KMS_ENCRYPT_DECRYPT_BLOB)
  /* Free the allocated object when needed */
  KMS_PlatfObjects_FreeObject(KMS_SESSION_ID_INTERNAL | ObjectId,
                                 &p_encrypted_object);
#endif /* KMS_ENCRYPT_DECRYPT_BLOB */
  /* A Garbage collection generate a WARNING ==> Not an error */
  if ((rv == NVMS_NOERROR) || (rv == NVMS_WARNING))
  {
    e_ret_status = CKR_OK;
  }
  else
  {
    e_ret_status = CKR_DEVICE_MEMORY;
  }

  /* Refresh NVM lists */
  KMS_PlatfObjects_NvmStaticObjectList();
#ifdef KMS_NVM_DYNAMIC_ENABLED
  KMS_PlatfObjects_NvmDynamicObjectList();
#endif /* KMS_NVM_DYNAMIC_ENABLED */

  return e_ret_status;
}

#ifdef KMS_NVM_DYNAMIC_ENABLED
/**
  * @brief  Remove object from NVM storage
  * @note   Only dynamic objects
  * @param  ObjectId Object ID
  * @retval CKR_OK if removal is successful
  *         CKR_DEVICE_MEMORY otherwise
  */
CK_RV KMS_PlatfObjects_NvmRemoveObject(uint32_t ObjectId)
{
  nvms_error_t rv = NVMS_DATA_NOT_FOUND;
  CK_RV e_ret_status;

  /* Check that the ObjectID is in dynamic range */
  if ((ObjectId >= KMS_INDEX_MIN_NVM_DYNAMIC_OBJECTS) && (ObjectId <= KMS_INDEX_MAX_NVM_DYNAMIC_OBJECTS))
  {
    rv = NVMS_EraseData(ObjectId - KMS_INDEX_MIN_NVM_STATIC_OBJECTS);
  }
  /* A Garbage collection generate a WARNING ==> Not an error */
  if ((rv == NVMS_NOERROR) || (rv == NVMS_WARNING))
  {
    e_ret_status = CKR_OK;
  }
  else
  {
    e_ret_status = CKR_DEVICE_MEMORY;
  }

  /* Refresh NVM lists */
  KMS_PlatfObjects_NvmStaticObjectList();
#ifdef KMS_NVM_DYNAMIC_ENABLED
  KMS_PlatfObjects_NvmDynamicObjectList();
#endif /* KMS_NVM_DYNAMIC_ENABLED */

  return e_ret_status;
}
#endif /* KMS_NVM_DYNAMIC_ENABLED */

#if defined(KMS_IMPORT_BLOB)
/**
  * @brief  Returns Blob import verification key handle
  * @retval Key handle
  */
CK_ULONG KMS_PlatfObjects_GetBlobVerifyKey(void)
{
  return (CK_ULONG)KMS_INDEX_BLOBIMPORT_VERIFY;
}
#endif /* KMS_IMPORT_BLOB */

#if defined(KMS_IMPORT_BLOB)
/**
  * @brief  Returns Blob import decryption key handle
  * @retval Key handle
  */
CK_ULONG KMS_PlatfObjects_GetBlobDecryptKey(void)
{
  return (CK_ULONG)KMS_INDEX_BLOBIMPORT_DECRYPT;
}
#endif /* KMS_IMPORT_BLOB */
#endif /* KMS_NVM_ENABLED */

#ifdef KMS_VM_DYNAMIC_ENABLED
/**
  * @brief  Store object in VM storage
  * @note   Either static or dynamic objects
  * @param  ObjectId Object ID
  * @param  pObjectToAdd Object to add
  * @param  ObjectSize Object size
  * @retval CKR_OK if storage is successful
  *         CKR_DEVICE_MEMORY otherwise
  */
CK_RV KMS_PlatfObjects_VmStoreObject(uint32_t ObjectId, uint8_t *pObjectToAdd,  uint32_t ObjectSize)
{
  vms_error_t  rv;
  CK_RV e_ret_status;

  /* It's a VM DYNAMIC object */
  if ((ObjectId >= KMS_INDEX_MIN_VM_DYNAMIC_OBJECTS) && (ObjectId <= KMS_INDEX_MAX_VM_DYNAMIC_OBJECTS))
  {
    rv = VMS_WRITE_DATA(ObjectId - KMS_INDEX_MIN_VM_DYNAMIC_OBJECTS, ObjectSize, (const uint8_t *)pObjectToAdd);
  }
  else
  {
    rv = VMS_SLOT_INVALID;
  }

  /* A WARNING was generated ==> Not an error */
  if ((rv == VMS_NOERROR) || (rv == VMS_WARNING))
  {
    e_ret_status = CKR_OK;
  }
  else
  {
    e_ret_status = CKR_DEVICE_MEMORY;
  }

  KMS_PlatfObjects_VmDynamicObjectList();

  return e_ret_status;
}

/**
  * @brief  Remove object from VM storage
  * @note   Only dynamic objects
  * @param  ObjectId Object ID
  * @retval CKR_OK if removal is successful
  *         CKR_DEVICE_MEMORY otherwise
  */
CK_RV KMS_PlatfObjects_VmRemoveObject(uint32_t ObjectId)
{
  vms_error_t rv = VMS_DATA_NOT_FOUND;
  CK_RV e_ret_status;

  /* Check that the ObjectID is in dynamic range */
  if ((ObjectId >= KMS_INDEX_MIN_VM_DYNAMIC_OBJECTS) && (ObjectId <= KMS_INDEX_MAX_VM_DYNAMIC_OBJECTS))
  {
    rv = VMS_EraseData(ObjectId - KMS_INDEX_MIN_VM_DYNAMIC_OBJECTS);
  }
  /* A WARNING was generated ==> Not an error */
  if ((rv == VMS_NOERROR) || (rv == VMS_WARNING))
  {
    e_ret_status = CKR_OK;
  }
  else
  {
    e_ret_status = CKR_DEVICE_MEMORY;
  }

  KMS_PlatfObjects_VmDynamicObjectList();

  return e_ret_status;
}
#endif /* KMS_VM_DYNAMIC_ENABLED */

/**
  * @}
  */

/**
  * @}
  */

/**
  * @}
  */

#endif /* KMS_ENABLED */
