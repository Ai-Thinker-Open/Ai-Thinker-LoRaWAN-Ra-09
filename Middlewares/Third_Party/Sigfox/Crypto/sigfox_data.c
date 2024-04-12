/**
  ******************************************************************************
  * @file    sigfox_data.c
  * @author  MCD Application Team
  * @brief   holds the sigfox credentials
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
/* Includes ------------------------------------------------------------------*/
#include "sigfox_types.h"
#include "sigfox_data.h"
#include "app_sigfox.h"


/* Exported types ------------------------------------------------------------*/
/* Exported constants --------------------------------------------------------*/
/* Exported macros -----------------------------------------------------------*/
/*Format Key*/
#define RAW_TO_INT16A(a,b,c,d,e,f,g,h,i,j,k,l,m,n,o,p) 0x##a, 0x##b, 0x##c, 0x##d,\
0x##e, 0x##f, 0x##g, 0x##h, 0x##i, 0x##j, 0x##k, 0x##l, 0x##m, 0x##n, 0x##o, 0x##p
#define FORMAT_KEY(...) RAW_TO_INT16A(__VA_ARGS__)

/*Format Id*/
#define RAW_TO_INT4A(a,b,c,d) 0x##d, 0x##c, 0x##b, 0x##a
#define FORMAT_ID(...) RAW_TO_INT4A(__VA_ARGS__)

/*Format Pac*/
#define RAW_TO_INT8A(a,b,c,d,e,f,g,h) 0x##a, 0x##b, 0x##c, 0x##d,\
0x##e, 0x##f, 0x##g, 0x##h
#define FORMAT_PAC(...) RAW_TO_INT8A(__VA_ARGS__)

/* External variables --------------------------------------------------------*/
#if (!defined (SIGFOX_KMS) || (SIGFOX_KMS == 0))
#if defined(__ARMCC_VERSION)
 __attribute__((section(".USER_embedded_Keys")))
#elif defined(__ICCARM__)
#pragma default_variable_attributes = @ ".USER_embedded_Keys"
#elif defined(__CC_ARM)
#pragma arm section rodata = ".USER_embedded_Keys"
#elif defined(__GNUC__)
 __attribute__((section(".USER_embedded_Keys.encrypted_sigfox_data")))
#endif

const
sfx_u8 encrypted_sigfox_data[] =
{
  FORMAT_ID(SIGFOX_ID), FORMAT_PAC(SIGFOX_PAC), 0x00, 0x00, 0x00, 0x00,
  FORMAT_KEY(SIGFOX_KEY),
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
};

/* Stop placing data in specified section*/
#if defined(__ICCARM__)
#pragma default_variable_attributes =
#elif defined(__CC_ARM)
#pragma arm section code
#endif

#endif
