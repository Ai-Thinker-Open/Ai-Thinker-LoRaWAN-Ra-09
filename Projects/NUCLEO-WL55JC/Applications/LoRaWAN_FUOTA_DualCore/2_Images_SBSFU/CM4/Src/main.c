/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file    main.c
  * @author  MCD Application Team
  * @brief   Main program body
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2017 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file in
  * the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */
/* USER CODE END Header */

/* Includes ------------------------------------------------------------------*/
#include "main.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */

#define MAIN_C

#include "sfu_fwimg_regions.h"
#include "sfu_def.h"
#include "sfu_loader.h"
#include "sfu_com_loader.h"
#include "sfu_low_level.h"
#include "sfu_low_level_security.h"
#include "sfu_low_level_flash.h"
#include "sfu_trace.h"
#include "sfu_fwimg_regions.h"
#include "sfu_new_image.h"
#include "sfu_boot.h"
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */

/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/

/* USER CODE BEGIN PV */
#if (SECBOOT_LOADER == SECBOOT_USE_LOCAL_LOADER)
/**
  * FW header (metadata) of the active FW in active slot: structured format (access by fields)
  */
SE_FwRawHeaderTypeDef fw_image_header_validated;
#endif /* (SECBOOT_LOADER == SECBOOT_USE_LOCAL_LOADER) */
/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
/* USER CODE BEGIN PFP */

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */

/* USER CODE END 0 */

/**
  * @brief  The application entry point.
  * @retval int
  */
int main(void)
{
  /* USER CODE BEGIN 1 */
  uint32_t jump_address ;
  typedef void (*Function_Pointer)(void);
  Function_Pointer  p_jump_to_function;
  __IO uint32_t *pSbsfuFlag = (__IO uint32_t *)CM4_SBFU_BOOT_FLAG_ADDRESS;
#if (SECBOOT_LOADER == SECBOOT_USE_LOCAL_LOADER)
  SFU_ErrorStatus           e_ret_status = SFU_ERROR;
  SFU_LOADER_StatusTypeDef  e_ret_status_app = SFU_LOADER_ERR_COM;
  uint32_t                  dwl_slot;
  uint32_t                  u_size = 0;
  SFU_FLASH_StatusTypeDef   flash_if_info;
#endif /* (SECBOOT_LOADER == SECBOOT_USE_LOCAL_LOADER) */

  /* Configure the system clock using LL functions */
  SystemClock_Config();
  /* USER CODE END 1 */

  /* MCU Configuration--------------------------------------------------------*/

  /* Reset of all peripherals, Initializes the Flash interface and the Systick. */
  HAL_Init();

  /* USER CODE BEGIN Init */

  /* USER CODE END Init */

  /* USER CODE BEGIN SysInit */
  /* LED Init*/
  BSP_LED_Init(LED_RED);

  /* Reset the SRAM1 flag */
  *pSbsfuFlag = SBSFU_NOT_BOOTED;

  /* Configure the security features */
  if (SFU_BOOT_CheckApplySecurityProtections(SFU_INITIAL_CONFIGURATION) != SFU_SUCCESS)
  {
    SFU_EXCPT_Security_Error();
  }
  FLOW_CONTROL_CHECK(uFlowProtectValue, FLOW_CTRL_RUNTIME_PROTECT);

  /* Boot CPU2 */
  HAL_PWREx_ReleaseCore(PWR_CORE_CPU2);

  /* Wait for the flag */
  while (*pSbsfuFlag == SBSFU_NOT_BOOTED)
  {
    BSP_LED_Toggle(LED_RED);
    HAL_Delay(500U);
  }
  BSP_LED_Off(LED_RED);
  /* USER CODE END SysInit */

  /* Initialize all configured peripherals */
  /* USER CODE BEGIN 2 */

  /* Check the security features once CPU2 has booted */
  FLOW_CONTROL_CHECK(uFlowProtectValue, FLOW_CTRL_RUNTIME_PROTECT);
  FLOW_CONTROL_INIT(uFlowProtectValue, FLOW_CTRL_INIT_VALUE);
  if (SFU_BOOT_CheckApplySecurityProtections(SFU_SECOND_CONFIGURATION) != SFU_SUCCESS)
  {
    SFU_EXCPT_Security_Error();
  }
  FLOW_CONTROL_CHECK(uFlowProtectValue, FLOW_CTRL_RUNTIME_PROTECT);

#if (SECBOOT_LOADER == SECBOOT_USE_LOCAL_LOADER)
  if (*pSbsfuFlag == SBSFU_FW_DOWNLOAD)
  {
    /* Initialize external flash interface (OSPI/QSPI) */
    SFU_LL_FLASH_Init();

    /* Configure Communication module */
    SFU_LL_UART_Init();

    TRACE("\r\n======================================================================");
    TRACE("\r\n=                           Loader                                   =");
    TRACE("\r\n======================================================================");
    TRACE("\r\n");

    /* Loader initialization */
    e_ret_status = SFU_LOADER_Init();
    if (e_ret_status != SFU_SUCCESS)
    {
      TRACE("Initialization failure : reset !");
      NVIC_SystemReset();
    }

    /* Download new firmware */
    e_ret_status = SFU_LOADER_DownloadNewUserFw(&e_ret_status_app, &dwl_slot, &u_size);
    if (e_ret_status == SFU_SUCCESS)
    {
      /* Read header in dwl slot */
      e_ret_status = SFU_LL_FLASH_Read((uint8_t *) &fw_image_header_validated, (uint8_t *) SlotStartAdd[dwl_slot],
                                       sizeof(SE_FwRawHeaderTypeDef));
    }

    if (e_ret_status == SFU_SUCCESS)
    {
      /*
       * Notify the Secure Boot that a new image has been downloaded
       * by calling the SE interface function to trigger the installation procedure at next reboot.
       */
       TRACE("\r\nDownload successful : %d bytes received\r\n", u_size);
      if (SFU_IMG_InstallAtNextReset((uint8_t *) &fw_image_header_validated) != SFU_SUCCESS)
      {
        /* Erase downloaded image */
        SFU_LL_FLASH_Erase_Size(&flash_if_info, (void *) SlotStartAdd[SLOT_DWL_1], SLOT_SIZE(SLOT_DWL_1));

        /* no specific error cause set */
      }
    }
    else
    {
      /* Erase downloaded image */
      TRACE("\r\nDownload failed (%d)\r\n", e_ret_status);
      SFU_LL_FLASH_Erase_Size(&flash_if_info, (void *) SlotStartAdd[SLOT_DWL_1], SLOT_SIZE(SLOT_DWL_1));
    }

    /* Reset to restart SBSFU */
    NVIC_SystemReset();
  }
#endif /* (SECBOOT_LOADER == SECBOOT_USE_LOCAL_LOADER) */

  if (*pSbsfuFlag == SBSFU_BOOTED)
  {

    /* Check the security features before jumping in the user application CPU2 */
    FLOW_CONTROL_CHECK(uFlowProtectValue, FLOW_CTRL_RUNTIME_PROTECT);
    FLOW_CONTROL_INIT(uFlowProtectValue, FLOW_CTRL_INIT_VALUE);
    if (SFU_BOOT_CheckApplySecurityProtections(SFU_THIRD_CONFIGURATION) != SFU_SUCCESS)
    {
      SFU_EXCPT_Security_Error();
    }
    FLOW_CONTROL_CHECK(uFlowProtectValue, FLOW_CTRL_RUNTIME_PROTECT);

#if defined(SFU_MPU_USERAPP_ACTIVATION)
    SFU_LL_SECU_SetProtectionMPU_UserApp();
#else
    HAL_MPU_Disable();
#endif /* SFU_MPU_USERAPP_ACTIVATION */

    jump_address = *(__IO uint32_t *)((SLOT_ACTIVE_2_START + SFU_IMG_IMAGE_OFFSET + 4));
    /* Jump to user application */
    p_jump_to_function = (Function_Pointer) jump_address;
    /* Initialize user application's Stack Pointer */
    __set_MSP(*(__IO uint32_t *)(SLOT_ACTIVE_2_START + SFU_IMG_IMAGE_OFFSET));

    /* JUMP into User App */
    p_jump_to_function();
  }

  /* We shouldn't reach this point */
  SFU_EXCPT_Security_Error();

  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
    /* USER CODE BEGIN 3 */

    /* USER CODE END 3 */
  }
  /* USER CODE END WHILE */
}

/**
  * @brief  System Clock Configuration
  *         The system Clock is configured as follows :
  *            System Clock source            = PLL (MSI)
  *            SYSCLK(Hz)                     = 48000000
  *            HCLK(Hz)                       = 48000000
  *            AHB1 Prescaler                 = 1 
  *            AHB3 Prescaler                 = 1
  *            APB1 Prescaler                 = 1
  *            APB2 Prescaler                 = 1
  *            MSI Frequency(Hz)              = 4000000
  *            PLL_M                          = 1
  *            PLL_N                          = 24
  *            PLL_R                          = 2
  *            Flash Latency(WS)              = 2
  *            Voltage range                  = 1
  * @param  None
  * @retval None
  */
void SystemClock_Config(void)
{
  LL_FLASH_SetLatency(LL_FLASH_LATENCY_2);

  while(LL_FLASH_GetLatency() != LL_FLASH_LATENCY_2)
  {
  };

  LL_PWR_SetRegulVoltageScaling(LL_PWR_REGU_VOLTAGE_SCALE1);
  LL_RCC_MSI_Enable();

   /* Wait till MSI is ready */
  while(LL_RCC_MSI_IsReady() != 1)
  {
  };

  LL_RCC_MSI_EnableRangeSelection();
  LL_RCC_MSI_SetRange(LL_RCC_MSIRANGE_6);
  LL_RCC_MSI_SetCalibTrimming(0);
  LL_RCC_PLL_ConfigDomain_SYS(LL_RCC_PLLSOURCE_MSI, LL_RCC_PLLM_DIV_1, 24, LL_RCC_PLLR_DIV_2);
  LL_RCC_PLL_EnableDomain_SYS();
  LL_RCC_PLL_Enable();

   /* Wait till PLL is ready */
  while(LL_RCC_PLL_IsReady() != 1)
  {
  };

  LL_RCC_SetSysClkSource(LL_RCC_SYS_CLKSOURCE_PLL);

   /* Wait till System clock is ready */
  while(LL_RCC_GetSysClkSource() != LL_RCC_SYS_CLKSOURCE_STATUS_PLL)
  {
  };

  LL_RCC_SetAHBPrescaler(LL_RCC_SYSCLK_DIV_1);
  LL_RCC_SetAHB3Prescaler(LL_RCC_SYSCLK_DIV_1);
  LL_RCC_SetAPB1Prescaler(LL_RCC_APB1_DIV_1);
  LL_RCC_SetAPB2Prescaler(LL_RCC_APB2_DIV_1);

  LL_Init1msTick(48000000);

  /* Update CMSIS variable (which can be updated also through SystemCoreClockUpdate function) */
  LL_SetSystemCoreClock(48000000);
}

/* USER CODE BEGIN 4 */

/**
  * @brief  Check (and Apply when possible) the security/safety/integrity protections.
  *         The "Apply" part depends on @ref SECBOOT_OB_DEV_MODE and @ref SFU_PROTECT_RDP_LEVEL.
  * @param  None
  * @note   This operation should be done as soon as possible after a reboot.
  * @retval SFU_ErrorStatus SFU_SUCCESS if successful, SFU_ERROR otherwise.
  */
SFU_ErrorStatus SFU_BOOT_CheckApplySecurityProtections(uint8_t uStep)
{
  SFU_ErrorStatus e_ret_status;

  /* Apply Static protections involving Option Bytes */
  e_ret_status = SFU_LL_SECU_CheckApplyStaticProtections();
  FLOW_CONTROL_CHECK(uFlowProtectValue, FLOW_CTRL_STATIC_PROTECT);
  if (e_ret_status == SFU_SUCCESS)
  {
    /* Apply runtime protections needed to be enabled after each Reset */
    e_ret_status = SFU_LL_SECU_CheckApplyRuntimeProtections(uStep);
  }
  FLOW_CONTROL_CHECK(uFlowProtectValue, FLOW_CTRL_RUNTIME_PROTECT);

  return e_ret_status;
}

/**
  * @brief  Stop in case of security error
  * @retval None
  */
void SFU_EXCPT_Security_Error(void)
{
  HAL_Delay(1000);
  /* This is the last operation executed. Force a System Reset. */
  NVIC_SystemReset();
}

/* USER CODE END 4 */

/**
  * @brief  This function is executed in case of error occurrence.
  * @retval None
  */
void Error_Handler(void)
{
  /* USER CODE BEGIN Error_Handler_Debug */
  /* User can add his own implementation to report the HAL error return state */
  /* Security issue : execution stopped ! */
  SFU_EXCPT_Security_Error();
  /* USER CODE END Error_Handler_Debug */
}

#ifdef  USE_FULL_ASSERT
/**
  * @brief  Reports the name of the source file and the source line number
  *         where the assert_param error has occurred.
  * @param  file: pointer to the source file name
  * @param  line: assert_param error line source number
  * @retval None
  */
void assert_failed(uint8_t *file, uint32_t line)
{
  /* USER CODE BEGIN 6 */
  /* User can add his own implementation to report the file name and line number,
    ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */
  /* Security issue : execution stopped ! */
  SFU_EXCPT_Security_Error();
  /* USER CODE END 6 */
}
#endif /* USE_FULL_ASSERT */
