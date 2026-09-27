#include "sm30bl.h"
#include <string.h>

static uint16_t U16LE(const uint8_t *data)
{
    return (uint16_t)data[0] | ((uint16_t)data[1] << 8U);
}

static void PutU16LE(uint8_t *data, uint16_t value)
{
    data[0] = (uint8_t)value;
    data[1] = (uint8_t)(value >> 8U);
}

static uint16_t EncodeSignedMagnitude(int16_t value)
{
    uint16_t encoded = (uint16_t)((value < 0) ? -value : value);
    if (value < 0) {
        encoded |= SM30BL_SPEED_DIRECTION_BIT;
    }
    return encoded;
}

static int16_t DecodeSignedMagnitude(uint16_t raw)
{
    int16_t magnitude = (int16_t)(raw & 0x7FFFU);
    return ((raw & SM30BL_SPEED_DIRECTION_BIT) != 0U) ?
           (int16_t)-magnitude : magnitude;
}

static int16_t DecodeLoad(uint16_t raw)
{
    int16_t magnitude = (int16_t)(raw & SM30BL_LOAD_VALUE_MASK);
    return ((raw & SM30BL_LOAD_DIRECTION_BIT) != 0U) ?
           (int16_t)-magnitude : magnitude;
}

void SM30BL_Init(SM30BL *servo, SCS_Bus *bus, uint8_t id)
{
    if (servo != NULL) {
        servo->bus = bus;
        servo->id = id;
    }
}

SCS_Result SM30BL_Ping(SM30BL *servo, SCS_StatusPacket *status)
{
    if (servo == NULL || servo->bus == NULL) {
        return SCS_ERR_ARGUMENT;
    }
    return SCS_Ping(servo->bus, servo->id, status);
}

SCS_Result SM30BL_SetPosition(SM30BL *servo, int16_t position,
                              uint16_t time, uint16_t speed,
                              SCS_StatusPacket *status)
{
    return SM30BL_SetPositionEx(servo, position, time, speed, 0U, status);
}

SCS_Result SM30BL_SetPositionEx(SM30BL *servo, int16_t position,
                                uint16_t time, uint16_t speed,
                                uint8_t acceleration,
                                SCS_StatusPacket *status)
{
    uint8_t data[SM30BL_GOAL_BLOCK_LENGTH];

    if (servo == NULL || servo->bus == NULL ||
        position == INT16_MIN || speed > SM30BL_MAX_SIGNED_MAGNITUDE) {
        return SCS_ERR_ARGUMENT;
    }
    data[0] = acceleration;
    PutU16LE(&data[1], EncodeSignedMagnitude(position));
    PutU16LE(&data[3], time);
    PutU16LE(&data[5], speed);
    return SCS_Write(servo->bus, servo->id, SM30BL_ACCELERATION_ADDR,
                     data, sizeof(data), status);
}

SCS_Result SM30BL_SetMode(SM30BL *servo, uint8_t mode,
                          SCS_StatusPacket *status)
{
    if (servo == NULL || servo->bus == NULL || mode > SM30BL_MODE_PWM) {
        return SCS_ERR_ARGUMENT;
    }
    return SCS_Write(servo->bus, servo->id, SM30BL_MODE_ADDR,
                     &mode, sizeof(mode), status);
}

SCS_Result SM30BL_SetWheelMode(SM30BL *servo, SCS_StatusPacket *status)
{
    return SM30BL_SetMode(servo, SM30BL_MODE_WHEEL, status);
}

SCS_Result SM30BL_SetTorqueEnable(SM30BL *servo, uint8_t enabled,
                                  SCS_StatusPacket *status)
{
    if (servo == NULL || servo->bus == NULL || enabled > 1U) {
        return SCS_ERR_ARGUMENT;
    }
    return SCS_Write(servo->bus, servo->id, SM30BL_TORQUE_ENABLE_ADDR,
                     &enabled, sizeof(enabled), status);
}

SCS_Result SM30BL_SetSpeed(SM30BL *servo, int16_t speed,
                           uint8_t acceleration, SCS_StatusPacket *status)
{
    uint8_t data[SM30BL_GOAL_BLOCK_LENGTH] = {0U};

    if (servo == NULL || servo->bus == NULL || speed == INT16_MIN) {
        return SCS_ERR_ARGUMENT;
    }

    /* In wheel mode, the SMS/STS map expects ACC + reserved position/time + speed. */
    data[0] = acceleration;
    PutU16LE(&data[5], EncodeSignedMagnitude(speed));
    return SCS_Write(servo->bus, servo->id, SM30BL_ACCELERATION_ADDR,
                     data, sizeof(data), status);
}

SCS_Result SM30BL_ReadPosition(SM30BL *servo, int16_t *position,
                               SCS_StatusPacket *status)
{
    uint8_t data[2];
    SCS_Result result;

    if (servo == NULL || servo->bus == NULL || position == NULL) {
        return SCS_ERR_ARGUMENT;
    }
    result = SCS_Read(servo->bus, servo->id, SM30BL_PRESENT_POSITION_ADDR,
                      sizeof(data), data, status);
    if ((result == SCS_OK || result == SCS_ERR_SERVO) &&
        status != NULL && status->parameter_length >= sizeof(data)) {
        *position = DecodeSignedMagnitude(U16LE(data));
    }
    return result;
}

SCS_Result SM30BL_ReadFeedback(SM30BL *servo, SM30BL_Feedback *feedback,
                               SCS_StatusPacket *status)
{
    SCS_Result result;
    SCS_Result feedback_result;

    if (servo == NULL || servo->bus == NULL || feedback == NULL) {
        return SCS_ERR_ARGUMENT;
    }
    result = SCS_Read(servo->bus, servo->id, SM30BL_PRESENT_POSITION_ADDR,
                      SM30BL_FEEDBACK_BLOCK_LENGTH, feedback->raw, status);
    if (result == SCS_OK || result == SCS_ERR_SERVO) {
        feedback_result = result;
        feedback->position = DecodeSignedMagnitude(U16LE(&feedback->raw[0]));
        feedback->speed = DecodeSignedMagnitude(U16LE(&feedback->raw[2]));
        feedback->load = DecodeLoad(U16LE(&feedback->raw[4]));
        feedback->voltage = feedback->raw[6];
        feedback->temperature = feedback->raw[7];
        result = SCS_Read(servo->bus, servo->id, SM30BL_SERVO_STATE_ADDR,
                          1U, &feedback->servo_state, status);
        if (result != SCS_OK) {
            return result;
        }
        result = SCS_Read(servo->bus, servo->id, SM30BL_MOVING_ADDR,
                          1U, &feedback->moving, status);
        if (result != SCS_OK) {
            return result;
        }
        {
            uint8_t current[2];
            result = SCS_Read(servo->bus, servo->id,
                              SM30BL_PRESENT_CURRENT_ADDR,
                              sizeof(current), current, status);
            if (result != SCS_OK) {
                return result;
            }
            feedback->current = U16LE(current);
        }
        return feedback_result;
    }
    return result;
}

SCS_Result SM30BL_SyncSetPosition(SCS_Bus *bus, const uint8_t *ids,
                                  const int16_t *positions,
                                  const uint16_t *times,
                                  const uint16_t *speeds,
                                  uint8_t count)
{
    uint8_t entries[(SCS_MAX_PACKET_SIZE - 8U)];
    uint8_t i;

    if (bus == NULL || ids == NULL || positions == NULL || times == NULL ||
        speeds == NULL || count == 0U ||
        ((uint16_t)count * (SM30BL_GOAL_BLOCK_LENGTH + 1U) + 2U) >
            (SCS_MAX_PACKET_SIZE - 6U)) {
        return SCS_ERR_ARGUMENT;
    }
    for (i = 0U; i < count; ++i) {
        uint16_t offset = (uint16_t)i * (SM30BL_GOAL_BLOCK_LENGTH + 1U);
        if (positions[i] == INT16_MIN ||
            speeds[i] > SM30BL_MAX_SIGNED_MAGNITUDE) {
            return SCS_ERR_ARGUMENT;
        }
        entries[offset] = ids[i];
        entries[offset + 1U] = 0U;
        PutU16LE(&entries[offset + 2U], EncodeSignedMagnitude(positions[i]));
        PutU16LE(&entries[offset + 4U], times[i]);
        PutU16LE(&entries[offset + 6U], speeds[i]);
    }
    return SCS_SyncWrite(bus, SM30BL_ACCELERATION_ADDR,
                         SM30BL_GOAL_BLOCK_LENGTH, entries,
                         SM30BL_GOAL_BLOCK_LENGTH + 1U, count);
}
