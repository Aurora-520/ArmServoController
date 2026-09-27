#include "app_freertos.h"
#include "app_servo_test.h"
#include "main.h"
#include "ws2812.h"
#include "ws2812_palette.h"

static StaticTask_t idle_task_tcb;
static StackType_t idle_task_stack[configMINIMAL_STACK_SIZE];
static StaticTask_t startup_task_tcb;
static StackType_t startup_task_stack[256];

static uint8_t BreathLevel(uint32_t phase)
{
    /* Triangle wave: 0 -> 255 -> 0. */
    phase %= 512U;
    return (phase <= 255U) ? (uint8_t)phase : (uint8_t)(511U - phase);
}

static void StartupTask(void *argument)
{
    uint32_t phase = 0U;
    uint32_t color_index = 0U;

    (void)argument;
    APP_ServoId3WheelRun();
    WS2812_Init();

    for (;;) {
        WS2812_Color color = WS2812_PaletteGet((WS2812_ColorId)color_index);
        color = WS2812_ColorScale(color, BreathLevel(phase));
        WS2812_SetAll(color);
        WS2812_Show();

        ++phase;
        if (phase >= 512U) {
            phase = 0U;
            ++color_index;
            if (color_index >= WS2812_PaletteCount()) {
                color_index = 0U;
            }
        }
        vTaskDelay(pdMS_TO_TICKS(8U));
    }
}

void APP_FreeRTOS_Init(void)
{
    (void)xTaskCreateStatic(StartupTask, "Startup", 256, NULL,
                            tskIDLE_PRIORITY + 1, startup_task_stack,
                            &startup_task_tcb);
}

void vApplicationGetIdleTaskMemory(StaticTask_t **tcb,
                                   StackType_t **stack,
                                   uint32_t *stack_size)
{
    *tcb = &idle_task_tcb;
    *stack = idle_task_stack;
    *stack_size = configMINIMAL_STACK_SIZE;
}

void vApplicationStackOverflowHook(TaskHandle_t task, char *task_name)
{
    (void)task;
    (void)task_name;
    taskDISABLE_INTERRUPTS();
    for (;;) { }
}

void vApplicationMallocFailedHook(void)
{
    taskDISABLE_INTERRUPTS();
    for (;;) { }
}

void vApplicationTickHook(void)
{
    HAL_IncTick();
}
