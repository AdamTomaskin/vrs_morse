/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file    gpio.c
  * @brief   This file provides code for the configuration
  *          of all used GPIO pins.
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2026 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */
/* USER CODE END Header */

/* Includes ------------------------------------------------------------------*/
#include "gpio.h"
#include "rfm22_board.h"   // ADD

/* USER CODE BEGIN 0 */

/* USER CODE END 0 */

/*----------------------------------------------------------------------------*/
/* Configure GPIO                                                             */
/*----------------------------------------------------------------------------*/
/* USER CODE BEGIN 1 */

/* USER CODE END 1 */

/** Configure pins
*/
void MX_GPIO_Init(void)
{

  GPIO_InitTypeDef GPIO_InitStruct = {0};

  /* GPIO Ports Clock Enable */
  __HAL_RCC_GPIOA_CLK_ENABLE();
  __HAL_RCC_GPIOB_CLK_ENABLE();

  /*Configure GPIO pin : PB1 */
  GPIO_InitStruct.Pin = GPIO_PIN_1;
  GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
  GPIO_InitStruct.Pull = GPIO_PULLUP;
  HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

  // ---- RFM22 pins ----
  // NSEL/CS output
  GPIO_InitStruct.Pin = RFM22_NSEL_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
  HAL_GPIO_Init(RFM22_NSEL_GPIO_Port, &GPIO_InitStruct);
  HAL_GPIO_WritePin(RFM22_NSEL_GPIO_Port, RFM22_NSEL_Pin, GPIO_PIN_SET); // deselect


  // nIRQ input (optional)
  GPIO_InitStruct.Pin = RFM22_nIRQ_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_INPUT;          // neskôr môžeš dať EXTI
  GPIO_InitStruct.Pull = GPIO_PULLUP;
  HAL_GPIO_Init(RFM22_nIRQ_GPIO_Port, &GPIO_InitStruct);
}



/* USER CODE BEGIN 2 */

/* USER CODE END 2 */
