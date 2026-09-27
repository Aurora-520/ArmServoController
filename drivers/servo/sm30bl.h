#ifndef SM30BL_H
#define SM30BL_H

#include "scs_protocol.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Values documented by the Feetech SCS protocol examples for magnetic servos. */
#define SM30BL_DEFAULT_ID             1U
#define SM30BL_DEFAULT_BAUDRATE      115200U
#define SM30BL_MODEL_L_ADDR           0x03U
#define SM30BL_MODEL_H_ADDR           0x04U
#define SM30BL_ID_ADDR                0x05U
#define SM30BL_BAUDRATE_ADDR          0x06U
#define SM30BL_MIN_ANGLE_ADDR         0x09U
#define SM30BL_MAX_ANGLE_ADDR         0x0BU
#define SM30BL_PHASE_ADDR             0x1FU
#define SM30BL_MODE_ADDR              0x21U
#define SM30BL_MODE_POSITION          0U
#define SM30BL_MODE_WHEEL             1U
#define SM30BL_MODE_PWM               2U
#define SM30BL_TORQUE_ENABLE_ADDR     0x28U
#define SM30BL_ACCELERATION_ADDR      0x29U
#define SM30BL_GOAL_POSITION_ADDR  0x2AU
#define SM30BL_PWM_SPEED_ADDR         0x2CU
#define SM30BL_GOAL_SPEED_ADDR        0x2EU
#define SM30BL_TORQUE_LIMIT_ADDR      0x30U
#define SM30BL_EEPROM_LOCK_ADDR       0x37U
#define SM30BL_PRESENT_POSITION_ADDR  0x38U
#define SM30BL_PRESENT_SPEED_ADDR     0x3AU
#define SM30BL_PRESENT_LOAD_ADDR      0x3CU
#define SM30BL_PRESENT_VOLTAGE_ADDR   0x3EU
#define SM30BL_PRESENT_TEMPERATURE_ADDR 0x3FU
#define SM30BL_SERVO_STATE_ADDR       0x41U
#define SM30BL_MOVING_ADDR            0x42U
#define SM30BL_PRESENT_CURRENT_ADDR   0x45U
#define SM30BL_GOAL_BLOCK_LENGTH       7U
#define SM30BL_FEEDBACK_BLOCK_LENGTH   8U
#define SM30BL_MAX_SIGNED_MAGNITUDE 32767
#define SM30BL_SPEED_DIRECTION_BIT 0x8000U
#define SM30BL_LOAD_DIRECTION_BIT  0x0400U
#define SM30BL_LOAD_VALUE_MASK     0x03FFU
#define SM30BL_STATE_VOLTAGE_ERROR 0x01U
#define SM30BL_STATE_ENCODER_ERROR 0x02U
#define SM30BL_STATE_TEMPERATURE_ERROR 0x04U
#define SM30BL_STATE_CURRENT_ERROR 0x08U
#define SM30BL_STATE_OVERLOAD      0x20U

typedef struct {
    SCS_Bus *bus;
    uint8_t id;
} SM30BL;

typedef struct {
    int16_t position;
    int16_t speed;
    int16_t load;
    uint8_t voltage;
    uint8_t temperature;
    uint8_t servo_state;
    uint8_t moving;
    uint16_t current;
    uint8_t raw[SM30BL_FEEDBACK_BLOCK_LENGTH];
} SM30BL_Feedback;

void SM30BL_Init(SM30BL *servo, SCS_Bus *bus, uint8_t id);
SCS_Result SM30BL_Ping(SM30BL *servo, SCS_StatusPacket *status);
SCS_Result SM30BL_SetPosition(SM30BL *servo, int16_t position,
                              uint16_t time, uint16_t speed,
                              SCS_StatusPacket *status);
SCS_Result SM30BL_SetPositionEx(SM30BL *servo, int16_t position,
                                uint16_t time, uint16_t speed,
                                uint8_t acceleration,
                                SCS_StatusPacket *status);
SCS_Result SM30BL_SetMode(SM30BL *servo, uint8_t mode,
                          SCS_StatusPacket *status);
SCS_Result SM30BL_SetWheelMode(SM30BL *servo, SCS_StatusPacket *status);
SCS_Result SM30BL_SetTorqueEnable(SM30BL *servo, uint8_t enabled,
                                  SCS_StatusPacket *status);
SCS_Result SM30BL_SetSpeed(SM30BL *servo, int16_t speed,
                           uint8_t acceleration, SCS_StatusPacket *status);
SCS_Result SM30BL_ReadPosition(SM30BL *servo, int16_t *position,
                               SCS_StatusPacket *status);
SCS_Result SM30BL_ReadFeedback(SM30BL *servo, SM30BL_Feedback *feedback,
                               SCS_StatusPacket *status);
SCS_Result SM30BL_SyncSetPosition(SCS_Bus *bus, const uint8_t *ids,
                                  const int16_t *positions,
                                  const uint16_t *times,
                                  const uint16_t *speeds,
                                  uint8_t count);

#ifdef __cplusplus
}
#endif

#endif
