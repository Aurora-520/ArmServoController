#ifndef SCS_PROTOCOL_H
#define SCS_PROTOCOL_H

#include "stm32h7xx_hal.h"
#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define SCS_BROADCAST_ID       0xFEU
#define SCS_MAX_PACKET_SIZE    96U

typedef enum {
    SCS_OK = 0,
    SCS_ERR_ARGUMENT,
    SCS_ERR_TIMEOUT,
    SCS_ERR_UART,
    SCS_ERR_FRAME,
    SCS_ERR_CHECKSUM,
    SCS_ERR_SERVO
} SCS_Result;

typedef enum {
    SCS_INST_PING       = 0x01U,
    SCS_INST_READ       = 0x02U,
    SCS_INST_WRITE      = 0x03U,
    SCS_INST_REG_WRITE  = 0x04U,
    SCS_INST_ACTION     = 0x05U,
    SCS_INST_RESTORE    = 0x06U,
    SCS_INST_BACKUP     = 0x09U,
    SCS_INST_REBOOT     = 0x08U,
    SCS_INST_RESET      = 0x0AU,
    SCS_INST_CALIBRATE  = 0x0BU,
    SCS_INST_SYNC_READ  = 0x82U,
    SCS_INST_SYNC_WRITE = 0x83U
} SCS_Instruction;

typedef struct {
    UART_HandleTypeDef *uart;
    uint32_t timeout_ms;
} SCS_Bus;

typedef struct {
    uint8_t id;
    uint8_t error;
    uint8_t parameter[SCS_MAX_PACKET_SIZE - 6U];
    uint8_t parameter_length;
} SCS_StatusPacket;

void SCS_BusInit(SCS_Bus *bus, UART_HandleTypeDef *uart, uint32_t timeout_ms);
uint8_t SCS_Checksum(uint8_t id, uint8_t length, uint8_t instruction,
                     const uint8_t *parameters, uint8_t parameter_length);
SCS_Result SCS_BuildInstruction(uint8_t id, SCS_Instruction instruction,
                                const uint8_t *parameters, uint8_t parameter_length,
                                uint8_t *packet, size_t packet_capacity,
                                size_t *packet_length);
SCS_Result SCS_ParseStatus(const uint8_t *packet, size_t packet_length,
                           SCS_StatusPacket *status);

SCS_Result SCS_Transaction(SCS_Bus *bus, const uint8_t *request,
                           size_t request_length, uint8_t expect_response,
                           SCS_StatusPacket *status);
SCS_Result SCS_Ping(SCS_Bus *bus, uint8_t id, SCS_StatusPacket *status);
SCS_Result SCS_Read(SCS_Bus *bus, uint8_t id, uint8_t address,
                    uint8_t length, uint8_t *data, SCS_StatusPacket *status);
SCS_Result SCS_Write(SCS_Bus *bus, uint8_t id, uint8_t address,
                     const uint8_t *data, uint8_t length,
                     SCS_StatusPacket *status);
SCS_Result SCS_RegWrite(SCS_Bus *bus, uint8_t id, uint8_t address,
                        const uint8_t *data, uint8_t length,
                        SCS_StatusPacket *status);
SCS_Result SCS_Action(SCS_Bus *bus, SCS_StatusPacket *status);
SCS_Result SCS_Reset(SCS_Bus *bus, uint8_t id, SCS_StatusPacket *status);
SCS_Result SCS_Calibrate(SCS_Bus *bus, uint8_t id, const uint8_t *value,
                         uint8_t value_length, SCS_StatusPacket *status);
SCS_Result SCS_SyncWrite(SCS_Bus *bus, uint8_t address, uint8_t data_length,
                         const uint8_t *entries, uint8_t entry_length,
                         uint8_t entry_count);

#ifdef __cplusplus
}
#endif

#endif
