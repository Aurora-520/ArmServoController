#include "app_servo_test.h"
#include "bsp_board.h"
#include "scs_protocol.h"
#include "sm30bl.h"
#include "FreeRTOS.h"
#include "task.h"
#include <string.h>

#define SERVO_STARTUP_DELAY_MS  500U
#define SERVO_RETRY_COUNT       5U
#define SERVO_RETRY_DELAY_MS    100U
#define SERVO_COMMAND_SETTLE_MS 20U

static void SendText(const char *text)
{
    HAL_UART_Transmit(&huart10, (uint8_t *)text, (uint16_t)strlen(text), 100U);
}

static uint8_t AppendText(char *buffer, uint8_t offset, const char *text)
{
    while (*text != '\0') {
        buffer[offset++] = *text++;
    }
    return offset;
}

static uint8_t AppendU8(char *buffer, uint8_t offset, uint8_t value)
{
    if (value >= 100U) {
        buffer[offset++] = (char)('0' + value / 100U);
        value %= 100U;
        buffer[offset++] = (char)('0' + value / 10U);
        buffer[offset++] = (char)('0' + value % 10U);
    } else if (value >= 10U) {
        buffer[offset++] = (char)('0' + value / 10U);
        buffer[offset++] = (char)('0' + value % 10U);
    } else {
        buffer[offset++] = (char)('0' + value);
    }
    return offset;
}

static uint8_t AppendHex8(char *buffer, uint8_t offset, uint8_t value)
{
    static const char digits[] = "0123456789ABCDEF";
    buffer[offset++] = digits[(value >> 4U) & 0x0FU];
    buffer[offset++] = digits[value & 0x0FU];
    return offset;
}

static SCS_Result RetryPing(SM30BL *servo, SCS_StatusPacket *status)
{
    SCS_Result result = SCS_ERR_TIMEOUT;
    uint8_t attempt;

    for (attempt = 0U; attempt < SERVO_RETRY_COUNT; ++attempt) {
        memset(status, 0, sizeof(*status));
        result = SM30BL_Ping(servo, status);
        if (result == SCS_OK) {
            return result;
        }
        vTaskDelay(pdMS_TO_TICKS(SERVO_RETRY_DELAY_MS));
    }
    return result;
}

static SCS_Result RetryWheelMode(SM30BL *servo, SCS_StatusPacket *status)
{
    SCS_Result result = SCS_ERR_TIMEOUT;
    uint8_t attempt;

    for (attempt = 0U; attempt < SERVO_RETRY_COUNT; ++attempt) {
        memset(status, 0, sizeof(*status));
        result = SM30BL_SetWheelMode(servo, status);
        if (result == SCS_OK) {
            return result;
        }
        vTaskDelay(pdMS_TO_TICKS(SERVO_RETRY_DELAY_MS));
    }
    return result;
}

static SCS_Result RetryTorqueEnable(SM30BL *servo, SCS_StatusPacket *status)
{
    SCS_Result result = SCS_ERR_TIMEOUT;
    uint8_t attempt;

    for (attempt = 0U; attempt < SERVO_RETRY_COUNT; ++attempt) {
        memset(status, 0, sizeof(*status));
        result = SM30BL_SetTorqueEnable(servo, 1U, status);
        if (result == SCS_OK) {
            return result;
        }
        vTaskDelay(pdMS_TO_TICKS(SERVO_RETRY_DELAY_MS));
    }
    return result;
}

static SCS_Result RetrySpeed(SM30BL *servo, SCS_StatusPacket *status)
{
    SCS_Result result = SCS_ERR_TIMEOUT;
    uint8_t attempt;

    for (attempt = 0U; attempt < SERVO_RETRY_COUNT; ++attempt) {
        memset(status, 0, sizeof(*status));
        result = SM30BL_SetSpeed(servo, 10, 5U, status);
        if (result == SCS_OK) {
            return result;
        }
        vTaskDelay(pdMS_TO_TICKS(SERVO_RETRY_DELAY_MS));
    }
    return result;
}

static void SendPingResult(uint8_t id, SCS_Result result,
                           const SCS_StatusPacket *status)
{
    char line[48];
    uint8_t length = 0U;
    uint8_t error = (status != NULL) ? status->error : 0xFFU;

    length = AppendText(line, length, "PING ID=");
    length = AppendU8(line, length, id);
    if (result == SCS_OK) {
        length = AppendText(line, length, " OK ERR=0x");
    } else if (result == SCS_ERR_SERVO) {
        length = AppendText(line, length, " SERVO_ERROR=0x");
    } else {
        length = AppendText(line, length, " NO_RESPONSE RESULT=");
        length = AppendU8(line, length, (uint8_t)result);
        length = AppendText(line, length, " ERR=0x");
    }
    length = AppendHex8(line, length, error);
    length = AppendText(line, length, "\r\n");
    HAL_UART_Transmit(&huart10, (uint8_t *)line, length, 100U);
}

void APP_ServoPingScan(void)
{
    SCS_Bus bus;
    SCS_StatusPacket status;
    SCS_Result result;
    uint8_t id;
    uint8_t found = 0U;

    SCS_BusInit(&bus, &huart3, 30U);
    SendText("SCS PING SCAN START\r\n");

    /* The SM-30BL specification lists ID 1 as the factory default. */
    memset(&status, 0, sizeof(status));
    result = SCS_Ping(&bus, 1U, &status);
    if (result == SCS_OK || result == SCS_ERR_SERVO) {
        SendPingResult(1U, result, &status);
        found = 1U;
    }

    /* Only scan all IDs when the default ID did not answer. */
    if (found == 0U) {
        for (id = 0U; id <= 253U; ++id) {
            if (id == 1U) {
                continue;
            }
            memset(&status, 0, sizeof(status));
            result = SCS_Ping(&bus, id, &status);
            if (result == SCS_OK || result == SCS_ERR_SERVO) {
                SendPingResult(id, result, &status);
                found = 1U;
            }
        }
    }

    if (found == 0U) {
        SendText("SCS PING NONE\r\n");
    }
    SendText("SCS PING SCAN END\r\n");
}

void APP_ServoId3WheelRun(void)
{
    SCS_Bus bus;
    SM30BL servo;
    SCS_StatusPacket status;
    SCS_Result result;

    SCS_BusInit(&bus, &huart3, 50U);
    SM30BL_Init(&servo, &bus, 3U);

    /* Let the servo and its RS485 transceiver finish their power-up. */
    vTaskDelay(pdMS_TO_TICKS(SERVO_STARTUP_DELAY_MS));
    SendText("ID3 wheel-speed test starting\r\n");
    result = RetryPing(&servo, &status);
    if (result != SCS_OK) {
        SendText("ID3 PING failed; motor command skipped\r\n");
        return;
    }

    result = RetryWheelMode(&servo, &status);
    if (result != SCS_OK) {
        SendText("ID3 wheel mode failed; motor command skipped\r\n");
        return;
    }
    vTaskDelay(pdMS_TO_TICKS(SERVO_COMMAND_SETTLE_MS));

    result = RetryTorqueEnable(&servo, &status);
    if (result != SCS_OK) {
        SendText("ID3 torque enable failed; motor command skipped\r\n");
        return;
    }
    vTaskDelay(pdMS_TO_TICKS(SERVO_COMMAND_SETTLE_MS));

    /* Positive low speed: about 7.32 RPM at the default 0.732 RPM/unit. */
    result = RetrySpeed(&servo, &status);
    if (result != SCS_OK) {
        SendText("ID3 speed command failed\r\n");
        return;
    }

    SendText("ID3 running continuously: speed=+10, accel=5\r\n");
}
