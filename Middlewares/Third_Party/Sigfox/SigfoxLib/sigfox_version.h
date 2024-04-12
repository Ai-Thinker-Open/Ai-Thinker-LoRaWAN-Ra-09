/**
  ******************************************************************************
  * @file    sigfox_version.h
  * @author  MCD Application Team
  * @brief   Identifies the version of Sigfox
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

#ifndef __SIGFOX_VERSION_H__
#define __SIGFOX_VERSION_H__

#ifdef __cplusplus
 extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
/* Exported constants --------------------------------------------------------*/

#define SIGFOX_VERSION_MAIN   (0x01U) /*!< [31:24] main version */
#define SIGFOX_VERSION_SUB1   (0x08U) /*!< [23:16] sub1 version */
#define SIGFOX_VERSION_SUB2   (0x00U) /*!< [15:8]  sub2 version */
#define SIGFOX_VERSION_RC     (0x00U) /*!< [7:0]  release candidate */
#define SIGFOX_VERSION        ((SIGFOX_VERSION_MAIN <<24)\
                               |(SIGFOX_VERSION_SUB1 << 16)\
                               |(SIGFOX_VERSION_SUB2 << 8 )\
                               |(SIGFOX_VERSION_RC))

/* Exported types ------------------------------------------------------------*/
/* External variables --------------------------------------------------------*/
/* Exported macros -----------------------------------------------------------*/
/* Exported functions ------------------------------------------------------- */

#ifdef __cplusplus
}
#endif

#endif /*__SIGFOX_VERSION_H__*/

