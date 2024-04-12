/**
  ******************************************************************************
  * @file    kms_counter.c
  * @author  MCD Application Team
  * @brief   This file contains implementations for Key Management Services (KMS)
  *          module Non Volatile Memory storage services.
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
#include "kms.h"
#if defined(KMS_ENABLED)
#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include "kms_init.h"           /* KMS session services */
#include "kms_mem.h"            /* KMS memory utilities */
#include "kms_objects.h"        /* KMS object management services */
#include "kms_counter.h"        /* KMS counter management services */
#include "kms_platf_objects.h"  /* KMS platform objects services */
#include "kms_nvm_storage.h"    /* KMS NVM storage services */

#if defined(KMS_NVM_ENABLED) || defined(KMS_VM_DYNAMIC_ENABLED)
/** @addtogroup Key_Management_Services Key Management Services (KMS)
  * @{
  */

/** @addtogroup KMS_COUNTER NVM and VM secure counters
  * @{
  */

/* Private types -------------------------------------------------------------*/
/* Private define ------------------------------------------------------------*/
/* Private macro -------------------------------------------------------------*/
/* Private variables ---------------------------------------------------------*/
/* Private function prototypes -----------------------------------------------*/
/* Private function ----------------------------------------------------------*/
/* Exported variables --------------------------------------------------------*/
/* Exported functions --------------------------------------------------------*/

/** @addtogroup KMS_COUNTER_Exported_Functions Exported Functions
  * @{
  */

/**
  * @brief  This function increments the specified secure counter
  * @param  hSession session handle
  * @param  hObject object handle
  * @param  pCounterValue pointer to the location that receives the updated counter value (NULL if not used)
  * @retval CKR_OK
  *         CKR_ATTRIBUTE_TYPE_INVALID
  *         CKR_ATTRIBUTE_VALUE_INVALID
  *         CKR_CRYPTOKI_NOT_INITIALIZED
  *         CKR_SESSION_HANDLE_INVALID
  *         CKR_FUNCTION_FAILED
  *         CKR_FUNCTION_NOT_SUPPORTED
  *         CKR_GENERAL_ERROR
  *         CKR_OBJECT_HANDLE_INVALID
  *         CKR_DEVICE_MEMORY
  */
CK_RV KMS_CounterIncrement(CK_SESSION_HANDLE hSession, CK_OBJECT_HANDLE hObject, CK_ULONG_PTR pCounterValue)
{
#if defined(KMS_SECURE_COUNTERS)
  CK_RV e_ret_status = CKR_FUNCTION_FAILED;
  kms_obj_keyhead_t *pkms_object;
  kms_attr_t *pkms_counter_attribute;
  CK_BBOOL saturated_counter;
  CK_ULONG max_value;
  CK_ULONG reset_value;
  CK_ULONG increment_value;
  CK_ULONG current_counter;
  kms_obj_keyhead_t *pBlob = NULL_PTR;

  if (!KMS_IS_INITIALIZED())
  {
    return CKR_CRYPTOKI_NOT_INITIALIZED;
  }
  if (KMS_CheckSessionHdle(hSession) != CKR_OK)
  {
    return CKR_SESSION_HANDLE_INVALID;
  }
  /* No processing already on going  */
  if (KMS_GETSESSION(hSession).state != KMS_SESSION_IDLE)
  {
    return CKR_SESSION_HANDLE_INVALID;
  }

  /* Read the KMS object using the object handle */
  pkms_object = KMS_Objects_GetPointer(hObject);

  /* Check that the KMS object is valid:
   * - NULL_PTR value means that the counter handle was not found
   * - KMS_ABI_VERSION_CK_2_40 & KMS_ABI_CONFIG_KEYHEAD are magic in header of the KMS object
   */
  if ((pkms_object != NULL) &&
      (pkms_object->version == KMS_ABI_VERSION_CK_2_40) &&
      (pkms_object->configuration == KMS_ABI_CONFIG_KEYHEAD))
  {
    /* Read the saturation configuration */
    e_ret_status = KMS_Objects_SearchAttributes(CKA_STM_COUNTER_SATURATED, pkms_object, &pkms_counter_attribute);
    if (e_ret_status == CKR_OK)
    {
      saturated_counter = *(pkms_counter_attribute->data);
    }
    else if (e_ret_status == CKR_ATTRIBUTE_TYPE_INVALID)
    {
      /* Set a default value */
      saturated_counter = CK_FALSE;
      e_ret_status = CKR_OK;
    }
    else
    {
      /* Do nothing */
    }

    if (e_ret_status == CKR_OK)
    {
      /* Read the max counter value */
      e_ret_status = KMS_Objects_SearchAttributes(CKA_STM_COUNTER_MAX_VALUE, pkms_object, &pkms_counter_attribute);
      if (e_ret_status == CKR_OK)
      {
        max_value = *(pkms_counter_attribute->data);
      }
      else if (e_ret_status == CKR_ATTRIBUTE_TYPE_INVALID)
      {
        /* Set a default value */
        max_value = 0xFFFFFFFF;
        e_ret_status = CKR_OK;
      }
      else
      {
        /* Do nothing */
      }
    }

    if (e_ret_status == CKR_OK)
    {
      /* Read the reset counter value */
      e_ret_status = KMS_Objects_SearchAttributes(CKA_STM_COUNTER_RESET_VALUE, pkms_object, &pkms_counter_attribute);
      if (e_ret_status == CKR_OK)
      {
        reset_value = *(pkms_counter_attribute->data);
      }
      else if (e_ret_status == CKR_ATTRIBUTE_TYPE_INVALID)
      {
        /* Set a default value */
        reset_value = 0x00000000;
        e_ret_status = CKR_OK;
      }
      else
      {
        /* Do nothing */
      }
    }

    if (e_ret_status == CKR_OK)
    {
      /* Read the increment value */
      e_ret_status = KMS_Objects_SearchAttributes(CKA_STM_COUNTER_INCREMENT, pkms_object, &pkms_counter_attribute);
      if (e_ret_status == CKR_OK)
      {
        increment_value = *(pkms_counter_attribute->data);
      }
      else if (e_ret_status == CKR_ATTRIBUTE_TYPE_INVALID)
      {
        /* Set a default value */
        increment_value = 0x00000001;
        e_ret_status = CKR_OK;
      }
      else
      {
        /* Do nothing */
      }
    }

    if (e_ret_status == CKR_OK)
    {
      /* Read the current counter value */
      e_ret_status = KMS_Objects_SearchAttributes(CKA_STM_COUNTER_VALUE, pkms_object, &pkms_counter_attribute);
      current_counter = *(pkms_counter_attribute->data);
    }
  }
  else
  {
    /* Can not retrieve proper counter handle */
    e_ret_status = CKR_OBJECT_HANDLE_INVALID;
  }

  /* Check the current value of the counter */
  if ((e_ret_status == CKR_OK) && (current_counter > max_value))
  {
    /* The counter should never be superior to its maximum value */
    e_ret_status = CKR_ATTRIBUTE_VALUE_INVALID;
  }

  /* Increment the counter */
  if (e_ret_status == CKR_OK)
  {
    if(current_counter <= max_value - increment_value)
    {
      current_counter = current_counter + increment_value;
    }
    else
    {
      if(saturated_counter == CK_TRUE)
      {
        current_counter = max_value;
      }
      else
      {
        current_counter = reset_value;
      }
    }

    /* pkms_object and pkms_counter_attribute are in flash:
     * An intermediate RAM buffer is needed */

    if (e_ret_status == CKR_OK)
    {
      pBlob = KMS_Alloc(hSession, sizeof(kms_obj_keyhead_no_blob_t) + pkms_object->blobs_size);
      if (pBlob == NULL_PTR)
      {
        e_ret_status = CKR_DEVICE_MEMORY;
      }
      if (e_ret_status == CKR_OK)
      {
        /* Copy header */
        (void)memcpy(pBlob, pkms_object, sizeof(kms_obj_keyhead_no_blob_t));
        /* Copy blob */
        (void)memcpy(&(pBlob->blobs[0]), &(pkms_object->blobs[0]), pkms_object->blobs_size);
        /* Search for the counter value to use */
        e_ret_status = KMS_Objects_SearchAttributes(CKA_STM_COUNTER_VALUE, pBlob, &pkms_counter_attribute);
        if (e_ret_status == CKR_OK)
        {
          /* Update the value in the KMS object */
          pkms_counter_attribute->data[0] = current_counter;
#ifdef KMS_NVM_DYNAMIC_ENABLED
          /* Write the KMS object into the NVM datastorage */
          e_ret_status = KMS_PlatfObjects_NvmStoreObject(pBlob->object_id,
                                                         (uint8_t *)pBlob,
                                                          pBlob->blobs_size + sizeof(kms_obj_keyhead_no_blob_t));
#endif /* KMS_NVM_DYNAMIC_ENABLED */
#ifdef KMS_VM_DYNAMIC_ENABLED
          /* Write the KMS object into the VM datastorage */
          e_ret_status = KMS_PlatfObjects_VmStoreObject(pBlob->object_id,
                                                        (uint8_t *)pBlob,
                                                        pBlob->blobs_size + sizeof(kms_obj_keyhead_no_blob_t));
#endif /* KMS_VM_DYNAMIC_ENABLED */
          if ((e_ret_status == CKR_OK) && (pCounterValue != NULL))
          {
            *pCounterValue = current_counter;
          }
          KMS_Free(hSession, pBlob);
        }
      }
    }
  }

#if defined(KMS_ENCRYPT_DECRYPT_BLOB)
  if (pkms_object != NULL_PTR)
  {
    KMS_Objects_ReleasePointer(hObject, pkms_object);
  }
#endif /* KMS_ENCRYPT_DECRYPT_BLOB */

  return e_ret_status;
#else /* KMS_SECURE_COUNTERS */
  return CKR_FUNCTION_NOT_SUPPORTED;
#endif /* KMS_SECURE_COUNTERS */
}

/**
  * @brief  This function gets the value of the specified secure counter
  * @param  hSession session handle
  * @param  hObject object handle
  * @param  pCounterValue pointer to the location that receives the current counter value
  * @retval CKR_OK
  *         CKR_ARGUMENTS_BAD
  *         CKR_ATTRIBUTE_TYPE_INVALID
  *         CKR_CRYPTOKI_NOT_INITIALIZED
  *         CKR_SESSION_HANDLE_INVALID
  *         CKR_FUNCTION_FAILED
  *         CKR_FUNCTION_NOT_SUPPORTED
  *         CKR_OBJECT_HANDLE_INVALID
  */
CK_RV KMS_CounterGetValue(CK_SESSION_HANDLE hSession, CK_OBJECT_HANDLE hObject, CK_ULONG_PTR pCounterValue)
{
#if defined(KMS_SECURE_COUNTERS)
  CK_RV e_ret_status = CKR_FUNCTION_FAILED;
  kms_obj_keyhead_t *pkms_object;
  kms_attr_t *pkms_counter_attribute;

  if (!KMS_IS_INITIALIZED())
  {
    return CKR_CRYPTOKI_NOT_INITIALIZED;
  }
  if (KMS_CheckSessionHdle(hSession) != CKR_OK)
  {
    return CKR_SESSION_HANDLE_INVALID;
  }
  /* No processing already on going  */
  if (KMS_GETSESSION(hSession).state != KMS_SESSION_IDLE)
  {
    return CKR_SESSION_HANDLE_INVALID;
  }
  /* Check parameter */
  if (pCounterValue == NULL_PTR)
  {
    return CKR_ARGUMENTS_BAD;
  }

  /* Read the KMS object using the object handle */
  pkms_object = KMS_Objects_GetPointer(hObject);

  /* Check that the KMS object is valid:
   * - NULL_PTR value means that the counter handle was not found
   * - KMS_ABI_VERSION_CK_2_40 & KMS_ABI_CONFIG_KEYHEAD are magic in header of the KMS object
   */
  if ((pkms_object != NULL) &&
      (pkms_object->version == KMS_ABI_VERSION_CK_2_40) &&
      (pkms_object->configuration == KMS_ABI_CONFIG_KEYHEAD))
  {
    /* Search for the counter Value to use */
    e_ret_status = KMS_Objects_SearchAttributes(CKA_STM_COUNTER_VALUE, pkms_object, &pkms_counter_attribute);

    if (e_ret_status == CKR_OK)
    {
      /* Set counter size with value from attribute  */
      if (pkms_counter_attribute->size == sizeof(CK_ULONG))
      {
        *pCounterValue = *(pkms_counter_attribute->data);
      }
      else
      {
        /* Unsupported value for pkms_counter_attribute->size */
        e_ret_status = CKR_ARGUMENTS_BAD;
      }
    }
  }
  else
  {
    /* Can not retrieve proper counter handle */
    e_ret_status = CKR_OBJECT_HANDLE_INVALID;
  }

#if defined(KMS_ENCRYPT_DECRYPT_BLOB)
  if (pkms_object != NULL_PTR)
  {
    KMS_Objects_ReleasePointer(hObject, pkms_object);
  }
#endif /* KMS_ENCRYPT_DECRYPT_BLOB */

  return e_ret_status;
#else /* KMS_SECURE_COUNTERS */
  return CKR_FUNCTION_NOT_SUPPORTED;
#endif /* KMS_SECURE_COUNTERS */
}

/**
  * @}
  */

#endif /* KMS_NVM_ENABLED || KMS_VM_DYNAMIC_ENABLED */
#endif /* KMS_ENABLED */
