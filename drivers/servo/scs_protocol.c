#include "scs_protocol.h"
#include <string.h>

static SCS_Result SCS_HalResult(HAL_StatusTypeDef result, uint32_t timeout_ms)
{
    if (result == HAL_OK) {
        return SCS_OK;
    }
    if (result == HAL_TIMEOUT) {
        (void)timeout_ms;
        return SCS_ERR_TIMEOUT;
    }
    return SCS_ERR_UART;
}

static void SCS_FlushRx(UART_HandleTypeDef *uart)
{
    if (uart == NULL) {
        return;
    }

    /* Discard bytes left in the USART FIFO by a reset or a failed exchange. */
    while (__HAL_UART_GET_FLAG(uart, UART_FLAG_RXNE) != RESET) {
        volatile uint32_t discarded = uart->Instance->RDR;
        (void)discarded;
    }
    __HAL_UART_CLEAR_PEFLAG(uart);
    __HAL_UART_CLEAR_FEFLAG(uart);
    __HAL_UART_CLEAR_NEFLAG(uart);
    __HAL_UART_CLEAR_OREFLAG(uart);
}

void SCS_BusInit(SCS_Bus *bus, UART_HandleTypeDef *uart, uint32_t timeout_ms)
{
    if (bus != NULL) {
        bus->uart = uart;
        bus->timeout_ms = timeout_ms;
    }
}

uint8_t SCS_Checksum(uint8_t id, uint8_t length, uint8_t instruction,
                     const uint8_t *parameters, uint8_t parameter_length)
{
    uint16_t sum = (uint16_t)id + length + instruction;
    uint8_t i;

    for (i = 0U; i < parameter_length; ++i) {
        sum += parameters[i];
    }
    return (uint8_t)(~(uint8_t)sum);
}

SCS_Result SCS_BuildInstruction(uint8_t id, SCS_Instruction instruction,
                                const uint8_t *parameters, uint8_t parameter_length,
                                uint8_t *packet, size_t packet_capacity,
                                size_t *packet_length)
{
    size_t total = (size_t)parameter_length + 6U;

    if (packet == NULL || packet_length == NULL ||
        (parameters == NULL && parameter_length != 0U) ||
        id > 0xFEU || parameter_length > (SCS_MAX_PACKET_SIZE - 6U) ||
        packet_capacity < total) {
        return SCS_ERR_ARGUMENT;
    }

    packet[0] = 0xFFU;
    packet[1] = 0xFFU;
    packet[2] = id;
    packet[3] = (uint8_t)(parameter_length + 2U);
    packet[4] = (uint8_t)instruction;
    if (parameter_length != 0U) {
        memcpy(&packet[5], parameters, parameter_length);
    }
    packet[5U + parameter_length] =
        SCS_Checksum(id, packet[3], packet[4], parameters, parameter_length);
    *packet_length = total;
    return SCS_OK;
}

SCS_Result SCS_ParseStatus(const uint8_t *packet, size_t packet_length,
                           SCS_StatusPacket *status)
{
    uint8_t length;
    uint8_t parameter_length;
    uint8_t expected;

    if (packet == NULL || status == NULL || packet_length < 6U ||
        packet[0] != 0xFFU || packet[1] != 0xFFU) {
        return SCS_ERR_FRAME;
    }
    length = packet[3];
    if (length < 2U || packet_length != (size_t)length + 4U) {
        return SCS_ERR_FRAME;
    }
    parameter_length = (uint8_t)(length - 2U);
    if (parameter_length > sizeof(status->parameter)) {
        return SCS_ERR_FRAME;
    }
    expected = SCS_Checksum(packet[2], length, packet[4],
                            &packet[5], parameter_length);
    if (expected != packet[5U + parameter_length]) {
        return SCS_ERR_CHECKSUM;
    }

    status->id = packet[2];
    status->error = packet[4];
    status->parameter_length = parameter_length;
    if (parameter_length != 0U) {
        memcpy(status->parameter, &packet[5], parameter_length);
    }
    return (status->error == 0U) ? SCS_OK : SCS_ERR_SERVO;
}

SCS_Result SCS_Transaction(SCS_Bus *bus, const uint8_t *request,
                           size_t request_length, uint8_t expect_response,
                           SCS_StatusPacket *status)
{
    uint8_t packet[SCS_MAX_PACKET_SIZE];
    uint8_t header[4];
    uint8_t sync_byte;
    uint8_t sync_count = 0U;
    uint8_t sync_scanned = 0U;
    uint8_t tail[SCS_MAX_PACKET_SIZE - 4U];
    uint8_t length;
    HAL_StatusTypeDef hal_result;
    SCS_Result result;

    if (bus == NULL || bus->uart == NULL || request == NULL ||
        request_length == 0U || request_length > SCS_MAX_PACKET_SIZE ||
        (expect_response != 0U && status == NULL)) {
        return SCS_ERR_ARGUMENT;
    }

    SCS_FlushRx(bus->uart);
    hal_result = HAL_UART_Transmit(bus->uart, (uint8_t *)request,
                                   (uint16_t)request_length, bus->timeout_ms);
    result = SCS_HalResult(hal_result, bus->timeout_ms);
    if (result != SCS_OK || expect_response == 0U) {
        return result;
    }

    /*
     * A half-duplex RS485 transceiver can leave one stale/noisy byte in the
     * RX path when DE changes from transmit to receive.  Do not assume that
     * the first byte returned by HAL_UART_Receive() is the first 0xFF of the
     * status packet.  Resynchronize on the protocol header instead.
     */
    while (sync_count < 2U && sync_scanned < 16U) {
        hal_result = HAL_UART_Receive(bus->uart, &sync_byte, 1U,
                                      bus->timeout_ms);
        result = SCS_HalResult(hal_result, bus->timeout_ms);
        if (result != SCS_OK) {
            return result;
        }
        ++sync_scanned;
        if (sync_byte == 0xFFU) {
            ++sync_count;
        } else {
            sync_count = 0U;
        }
    }
    if (sync_count < 2U) {
        return SCS_ERR_FRAME;
    }
    header[0] = 0xFFU;
    header[1] = 0xFFU;
    hal_result = HAL_UART_Receive(bus->uart, &header[2], 2U,
                                  bus->timeout_ms);
    result = SCS_HalResult(hal_result, bus->timeout_ms);
    if (result != SCS_OK) {
        return result;
    }
    if (header[0] != 0xFFU || header[1] != 0xFFU || header[3] < 2U ||
        header[3] > (SCS_MAX_PACKET_SIZE - 4U)) {
        return SCS_ERR_FRAME;
    }
    length = header[3];
    hal_result = HAL_UART_Receive(bus->uart, tail, length, bus->timeout_ms);
    result = SCS_HalResult(hal_result, bus->timeout_ms);
    if (result != SCS_OK) {
        return result;
    }
    memcpy(packet, header, sizeof(header));
    memcpy(&packet[4], tail, length);
    return SCS_ParseStatus(packet, (size_t)length + 4U, status);
}

SCS_Result SCS_Ping(SCS_Bus *bus, uint8_t id, SCS_StatusPacket *status)
{
    uint8_t packet[6];
    size_t packet_length;
    SCS_Result result = SCS_BuildInstruction(id, SCS_INST_PING, NULL, 0U,
                                             packet, sizeof(packet),
                                             &packet_length);
    if (result != SCS_OK) {
        return result;
    }
    return SCS_Transaction(bus, packet, packet_length, 1U, status);
}

SCS_Result SCS_Read(SCS_Bus *bus, uint8_t id, uint8_t address,
                    uint8_t length, uint8_t *data, SCS_StatusPacket *status)
{
    uint8_t parameters[2] = { address, length };
    uint8_t packet[8];
    size_t packet_length;
    SCS_Result result;

    if (data == NULL || status == NULL || length == 0U ||
        length > sizeof(status->parameter)) {
        return SCS_ERR_ARGUMENT;
    }
    result = SCS_BuildInstruction(id, SCS_INST_READ, parameters,
                                  sizeof(parameters), packet, sizeof(packet),
                                  &packet_length);
    if (result != SCS_OK) {
        return result;
    }
    result = SCS_Transaction(bus, packet, packet_length, 1U, status);
    if ((result == SCS_OK || result == SCS_ERR_SERVO) &&
        status->parameter_length >= length) {
        memcpy(data, status->parameter, length);
    }
    return result;
}

SCS_Result SCS_Write(SCS_Bus *bus, uint8_t id, uint8_t address,
                     const uint8_t *data, uint8_t length,
                     SCS_StatusPacket *status)
{
    uint8_t parameters[SCS_MAX_PACKET_SIZE - 6U];
    uint8_t packet[SCS_MAX_PACKET_SIZE];
    size_t packet_length;
    SCS_Result result;

    if (data == NULL || length == 0U ||
        length > (SCS_MAX_PACKET_SIZE - 7U)) {
        return SCS_ERR_ARGUMENT;
    }
    parameters[0] = address;
    memcpy(&parameters[1], data, length);
    result = SCS_BuildInstruction(id, SCS_INST_WRITE, parameters,
                                  (uint8_t)(length + 1U), packet, sizeof(packet),
                                  &packet_length);
    if (result != SCS_OK) {
        return result;
    }
    /* Broadcast writes do not return a status packet. */
    return SCS_Transaction(bus, packet, packet_length,
                           (id == SCS_BROADCAST_ID) ? 0U : 1U, status);
}

SCS_Result SCS_RegWrite(SCS_Bus *bus, uint8_t id, uint8_t address,
                        const uint8_t *data, uint8_t length,
                        SCS_StatusPacket *status)
{
    uint8_t parameters[SCS_MAX_PACKET_SIZE - 6U];
    uint8_t packet[SCS_MAX_PACKET_SIZE];
    size_t packet_length;
    SCS_Result result;

    if (data == NULL || length == 0U ||
        length > (SCS_MAX_PACKET_SIZE - 7U)) {
        return SCS_ERR_ARGUMENT;
    }
    parameters[0] = address;
    memcpy(&parameters[1], data, length);
    result = SCS_BuildInstruction(id, SCS_INST_REG_WRITE, parameters,
                                  (uint8_t)(length + 1U), packet, sizeof(packet),
                                  &packet_length);
    if (result != SCS_OK) {
        return result;
    }
    return SCS_Transaction(bus, packet, packet_length,
                           (id == SCS_BROADCAST_ID) ? 0U : 1U, status);
}

SCS_Result SCS_Action(SCS_Bus *bus, SCS_StatusPacket *status)
{
    uint8_t packet[6];
    size_t packet_length;
    SCS_Result result = SCS_BuildInstruction(SCS_BROADCAST_ID, SCS_INST_ACTION,
                                             NULL, 0U, packet, sizeof(packet),
                                             &packet_length);
    if (result != SCS_OK) {
        return result;
    }
    return SCS_Transaction(bus, packet, packet_length, 0U, status);
}

SCS_Result SCS_Reset(SCS_Bus *bus, uint8_t id, SCS_StatusPacket *status)
{
    uint8_t packet[6];
    size_t packet_length;
    SCS_Result result = SCS_BuildInstruction(id, SCS_INST_RESET, NULL, 0U,
                                             packet, sizeof(packet),
                                             &packet_length);
    if (result != SCS_OK) {
        return result;
    }
    return SCS_Transaction(bus, packet, packet_length,
                           (id == SCS_BROADCAST_ID) ? 0U : 1U, status);
}

SCS_Result SCS_Calibrate(SCS_Bus *bus, uint8_t id, const uint8_t *value,
                         uint8_t value_length, SCS_StatusPacket *status)
{
    uint8_t packet[SCS_MAX_PACKET_SIZE];
    size_t packet_length;
    SCS_Result result;

    if (value_length != 0U && value_length != 2U) {
        return SCS_ERR_ARGUMENT;
    }
    result = SCS_BuildInstruction(id, SCS_INST_CALIBRATE, value, value_length,
                                  packet, sizeof(packet), &packet_length);
    if (result != SCS_OK) {
        return result;
    }
    return SCS_Transaction(bus, packet, packet_length,
                           (id == SCS_BROADCAST_ID) ? 0U : 1U, status);
}

SCS_Result SCS_SyncWrite(SCS_Bus *bus, uint8_t address, uint8_t data_length,
                         const uint8_t *entries, uint8_t entry_length,
                         uint8_t entry_count)
{
    uint8_t parameters[SCS_MAX_PACKET_SIZE - 6U];
    uint8_t packet[SCS_MAX_PACKET_SIZE];
    size_t packet_length;
    SCS_Result result;
    uint16_t required;

    if (bus == NULL || entries == NULL || data_length == 0U ||
        entry_length != (uint8_t)(data_length + 1U) || entry_count == 0U) {
        return SCS_ERR_ARGUMENT;
    }
    required = (uint16_t)entry_count * entry_length + 2U;
    if (required > sizeof(parameters)) {
        return SCS_ERR_ARGUMENT;
    }
    parameters[0] = address;
    parameters[1] = data_length;
    memcpy(&parameters[2], entries, (size_t)entry_count * entry_length);
    result = SCS_BuildInstruction(SCS_BROADCAST_ID, SCS_INST_SYNC_WRITE,
                                  parameters, (uint8_t)required, packet,
                                  sizeof(packet), &packet_length);
    if (result != SCS_OK) {
        return result;
    }
    return SCS_Transaction(bus, packet, packet_length, 0U, NULL);
}
