#ifndef BSP_BOARD_H
#define BSP_BOARD_H

#include "stm32h7xx_hal.h"

void BSP_Board_Init(void);

extern UART_HandleTypeDef huart3;
extern UART_HandleTypeDef huart10;

#endif
