/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file    radio_board_if.c
  * @brief   Radio board interface — RF switch pins aligned with LoRaWAN_AT_Slave
  *          (lorawan_at_demo BSP: PA4/PA5/PC5, not stock Nucleo PC3/PC4/PC5).
  ******************************************************************************
  */
/* USER CODE END Header */

#include "radio_board_if.h"
#include "stm32wlxx_hal.h"

/* RF switch GPIO — same as lorawan_at_demo/stm32wlxx_nucleo_radio.h */
#define RF_SW_CTRL3_PIN       GPIO_PIN_4
#define RF_SW_CTRL3_GPIO_PORT GPIOA
#define RF_SW_CTRL1_PIN       GPIO_PIN_5
#define RF_SW_CTRL1_GPIO_PORT GPIOA
#define RF_SW_CTRL2_PIN       GPIO_PIN_5
#define RF_SW_CTRL2_GPIO_PORT GPIOC

#define RBI_STATUS_OK  0

static void rf_switch_gpio_init(void)
{
  GPIO_InitTypeDef gpio = {0};

  __HAL_RCC_GPIOA_CLK_ENABLE();
  __HAL_RCC_GPIOC_CLK_ENABLE();

  gpio.Mode = GPIO_MODE_OUTPUT_PP;
  gpio.Pull = GPIO_NOPULL;
  gpio.Speed = GPIO_SPEED_FREQ_VERY_HIGH;

  gpio.Pin = RF_SW_CTRL1_PIN;
  HAL_GPIO_Init(RF_SW_CTRL1_GPIO_PORT, &gpio);

  gpio.Pin = RF_SW_CTRL2_PIN;
  HAL_GPIO_Init(RF_SW_CTRL2_GPIO_PORT, &gpio);

  gpio.Pin = RF_SW_CTRL3_PIN;
  HAL_GPIO_Init(RF_SW_CTRL3_GPIO_PORT, &gpio);

  HAL_GPIO_WritePin(RF_SW_CTRL1_GPIO_PORT, RF_SW_CTRL1_PIN, GPIO_PIN_RESET);
  HAL_GPIO_WritePin(RF_SW_CTRL2_GPIO_PORT, RF_SW_CTRL2_PIN, GPIO_PIN_RESET);
  HAL_GPIO_WritePin(RF_SW_CTRL3_GPIO_PORT, RF_SW_CTRL3_PIN, GPIO_PIN_RESET);
}

int32_t RBI_Init(void)
{
  rf_switch_gpio_init();
  return RBI_STATUS_OK;
}

int32_t RBI_DeInit(void)
{
  HAL_GPIO_WritePin(RF_SW_CTRL1_GPIO_PORT, RF_SW_CTRL1_PIN, GPIO_PIN_RESET);
  HAL_GPIO_WritePin(RF_SW_CTRL2_GPIO_PORT, RF_SW_CTRL2_PIN, GPIO_PIN_RESET);
  HAL_GPIO_WritePin(RF_SW_CTRL3_GPIO_PORT, RF_SW_CTRL3_PIN, GPIO_PIN_RESET);

  HAL_GPIO_DeInit(RF_SW_CTRL1_GPIO_PORT, RF_SW_CTRL1_PIN);
  HAL_GPIO_DeInit(RF_SW_CTRL2_GPIO_PORT, RF_SW_CTRL2_PIN);
  HAL_GPIO_DeInit(RF_SW_CTRL3_GPIO_PORT, RF_SW_CTRL3_PIN);

  return RBI_STATUS_OK;
}

int32_t RBI_ConfigRFSwitch(RBI_Switch_TypeDef Config)
{
  switch (Config)
  {
    case RBI_SWITCH_OFF:
      HAL_GPIO_WritePin(RF_SW_CTRL3_GPIO_PORT, RF_SW_CTRL3_PIN, GPIO_PIN_RESET);
      HAL_GPIO_WritePin(RF_SW_CTRL1_GPIO_PORT, RF_SW_CTRL1_PIN, GPIO_PIN_RESET);
      HAL_GPIO_WritePin(RF_SW_CTRL2_GPIO_PORT, RF_SW_CTRL2_PIN, GPIO_PIN_RESET);
      break;

    case RBI_SWITCH_RX:
      HAL_GPIO_WritePin(RF_SW_CTRL3_GPIO_PORT, RF_SW_CTRL3_PIN, GPIO_PIN_RESET);
      HAL_GPIO_WritePin(RF_SW_CTRL1_GPIO_PORT, RF_SW_CTRL1_PIN, GPIO_PIN_SET);
      HAL_GPIO_WritePin(RF_SW_CTRL2_GPIO_PORT, RF_SW_CTRL2_PIN, GPIO_PIN_RESET);
      break;

    case RBI_SWITCH_RFO_LP:
      HAL_GPIO_WritePin(RF_SW_CTRL3_GPIO_PORT, RF_SW_CTRL3_PIN, GPIO_PIN_SET);
      HAL_GPIO_WritePin(RF_SW_CTRL1_GPIO_PORT, RF_SW_CTRL1_PIN, GPIO_PIN_RESET);
      HAL_GPIO_WritePin(RF_SW_CTRL2_GPIO_PORT, RF_SW_CTRL2_PIN, GPIO_PIN_SET);
      break;

    case RBI_SWITCH_RFO_HP:
      HAL_GPIO_WritePin(RF_SW_CTRL3_GPIO_PORT, RF_SW_CTRL3_PIN, GPIO_PIN_SET);
      HAL_GPIO_WritePin(RF_SW_CTRL1_GPIO_PORT, RF_SW_CTRL1_PIN, GPIO_PIN_RESET);
      HAL_GPIO_WritePin(RF_SW_CTRL2_GPIO_PORT, RF_SW_CTRL2_PIN, GPIO_PIN_SET);
      break;

    default:
      break;
  }

  return RBI_STATUS_OK;
}

int32_t RBI_GetTxConfig(void)
{
  return RBI_CONF_RFO_HP;
}

int32_t RBI_IsTCXO(void)
{
  /* 外接 32MHz 晶振供 SUBGHZ；与 LoRaWAN_AT_Slave demo BSP 一致（非 TCXO 供电控制） */
  return 0;
}

int32_t RBI_IsDCDC(void)
{
  return 1;
}

int32_t RBI_GetRFOMaxPowerConfig(RBI_RFOMaxPowerConfig_TypeDef Config)
{
  if (Config == RBI_RFO_LP_MAXPOWER)
  {
    return 15;
  }
  return 22;
}
