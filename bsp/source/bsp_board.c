#include "bsp_board.h"
#include "main.h"

UART_HandleTypeDef huart3;
UART_HandleTypeDef huart10;

static void GPIO_Init(void);
static void USART3_RS485_Init(void);
static void USART10_Debug_Init(void);

void BSP_Board_Init(void)
{
    GPIO_Init();
    USART3_RS485_Init();
    USART10_Debug_Init();
}

static void GPIO_Init(void)
{
    GPIO_InitTypeDef gpio = {0};
    __HAL_RCC_GPIOA_CLK_ENABLE();
    __HAL_RCC_GPIOB_CLK_ENABLE();
    __HAL_RCC_GPIOD_CLK_ENABLE();
    __HAL_RCC_GPIOE_CLK_ENABLE();

    /* WS2812 DIN is PA7. Hold the data line low during bring-up; no LED data
       is sent in this baseline project. */
    HAL_GPIO_WritePin(GPIOA, GPIO_PIN_7, GPIO_PIN_RESET);
    gpio.Pin = GPIO_PIN_7;
    gpio.Mode = GPIO_MODE_OUTPUT_PP;
    gpio.Pull = GPIO_NOPULL;
    gpio.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
    HAL_GPIO_Init(GPIOA, &gpio);

    gpio.Pin = GPIO_PIN_8 | GPIO_PIN_9;
    gpio.Mode = GPIO_MODE_AF_PP;
    gpio.Pull = GPIO_NOPULL;
    gpio.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
    gpio.Alternate = GPIO_AF7_USART3;
    HAL_GPIO_Init(GPIOD, &gpio);

    gpio.Pin = GPIO_PIN_14;
    gpio.Alternate = GPIO_AF7_USART3;
    HAL_GPIO_Init(GPIOB, &gpio);

    /* MC02 UART10 header: PE2=RX, PE3=TX (USART10 AF4). */
    gpio.Pin = GPIO_PIN_2 | GPIO_PIN_3;
    gpio.Mode = GPIO_MODE_AF_PP;
    gpio.Pull = GPIO_NOPULL;
    gpio.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
    gpio.Alternate = GPIO_AF4_USART10;
    HAL_GPIO_Init(GPIOE, &gpio);
}

static void USART3_RS485_Init(void)
{
    __HAL_RCC_USART3_CLK_ENABLE();
    huart3.Instance = USART3;
    huart3.Init.BaudRate = 115200;
    huart3.Init.WordLength = UART_WORDLENGTH_8B;
    huart3.Init.StopBits = UART_STOPBITS_1;
    huart3.Init.Parity = UART_PARITY_NONE;
    huart3.Init.Mode = UART_MODE_TX_RX;
    huart3.Init.HwFlowCtl = UART_HWCONTROL_NONE;
    huart3.Init.OverSampling = UART_OVERSAMPLING_16;
    huart3.Init.OneBitSampling = UART_ONE_BIT_SAMPLE_DISABLE;
    huart3.Init.ClockPrescaler = UART_PRESCALER_DIV1;
    huart3.AdvancedInit.AdvFeatureInit = UART_ADVFEATURE_NO_INIT;
    if (HAL_RS485Ex_Init(&huart3, UART_DE_POLARITY_HIGH, 0, 0) != HAL_OK) {
        Error_Handler();
    }
}

static void USART10_Debug_Init(void)
{
    __HAL_RCC_USART10_CLK_ENABLE();
    huart10.Instance = USART10;
    huart10.Init.BaudRate = 115200;
    huart10.Init.WordLength = UART_WORDLENGTH_8B;
    huart10.Init.StopBits = UART_STOPBITS_1;
    huart10.Init.Parity = UART_PARITY_NONE;
    huart10.Init.Mode = UART_MODE_TX_RX;
    huart10.Init.HwFlowCtl = UART_HWCONTROL_NONE;
    huart10.Init.OverSampling = UART_OVERSAMPLING_16;
    huart10.Init.OneBitSampling = UART_ONE_BIT_SAMPLE_DISABLE;
    huart10.Init.ClockPrescaler = UART_PRESCALER_DIV1;
    huart10.AdvancedInit.AdvFeatureInit = UART_ADVFEATURE_NO_INIT;
    if (HAL_UART_Init(&huart10) != HAL_OK) {
        Error_Handler();
    }
}

void HAL_UART_MspInit(UART_HandleTypeDef *uart)
{
    if (uart->Instance == USART3) {
        /* PD8 TX, PD9 RX, PB14 DE are configured in the board pin init. */
    } else if (uart->Instance == USART10) {
        /* PE2 RX, PE3 TX are configured in the board pin init. */
    }
}
