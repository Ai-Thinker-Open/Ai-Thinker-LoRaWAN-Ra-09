/**
  ******************************************************************************
  * @file    st_sigfox_api.h
  * @author  MCD Application Team
  * @brief   add-on above sigfox_api.h
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2020(-2021) STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */

/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __ST_SIGFOX_API_H__
#define __ST_SIGFOX_API_H__

#ifdef __cplusplus
 extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "sigfox_types.h"
#include "sigfox_api.h"
#include "addon_sigfox_rf_protocol_api.h"
/* Exported types ------------------------------------------------------------*/
/* Exported constants --------------------------------------------------------*/
/* payload sizes definition */
#define SGFX_MAX_UL_PAYLOAD_SIZE 12
#define SGFX_MAX_DL_PAYLOAD_SIZE  8

/* Exported function --------------------------------------------------------*/

#ifdef __cplusplus
}
#endif

#endif /* __ST_SIGFOX_API_H__ */
