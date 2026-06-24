#ifndef __LL_PROTOCOL_h__
#define __LL_PROTOCOL_h__

#include "main.h"
#include "pb_encode.h"
#include "pb_decode.h"
#include "message.pb.h"

/**
 * [SyncSeq (напр., "ROV")][Length (2 байта)][Type][ProtobufData][CRC (2 байта)]
 */
#define SYNC_SEQ "ROV"                            // Последовательность символов синхронизации пакета данных
#define SYNC_LEN 3                                // Кол-во байт синхропоследовательности
#define LENGTH_BYTES 2                            // Кол-во байт блока полезных данных PAYLOAD
#define TYPE_BYTES 2                              // Кол-во байт типа сообщения (внутри первые байты блока полезных данных PAYLOAD)
#define CRC_BYTES 2                               // Кол-во байт секции контрольной суммы CRC16
#define HEADER_SIZE (SYNC_LEN + LENGTH_BYTES)     // длина заголовка пакета
#define MAX_PACKET_SIZE MAX_UART_DATA_PACKET_SIZE // (HEADER_SIZE + PAYLOAD + CRC_BYTES)
#define MIN_PACKET_SIZE (HEADER_SIZE + TYPE_BYTES + CRC_BYTES)
#define MAX_PAYLOAD_SIZE (MAX_PACKET_SIZE - HEADER_SIZE - CRC_BYTES)

// #define TELEMETRY_UPDATE_INTERVAL_MS 10 // интервал обновления телеметрии (100 Гц)

typedef enum
{
    EQUIPMENT_POWER_CONTROL_TASK_ID,
    PRESSURE_SENSOR_TASK_ID,
    NONE_TASK_ID
} TaskID_t;

typedef enum CommandType_Typedef
{
    CMD_NONE,
    CMD_UPDATE_FIRMWARE,   // UF: Update Firmware. Обновление прошивки
    CMD_GET_TELEMETRY,     // GT: Get Telemetry. Запрос телеметрии
    CMD_THRUSTERS_CONTROL, // TC: Thrusters Control. Управление движителями ДРК
    CMD_HARDWARE_CONTROL,  // HC: Hardware Control. Управление группой периферийных ШИМ-устройств
} CommandType_t;

// Структура для хранения типов команд
typedef struct
{
    const char type[2];    // Тип команды (2 символа)
    CommandType_t command; // Соответствующая команда
} CommandMapping_t;

uint8_t *packetParsing(uint8_t *pData, size_t data_len, size_t *_payload_len);
CommandType_t getCommandType(uint8_t *_payload_pt, size_t _payload_len);
uint8_t *getDataPointFromPayload(uint8_t *_payload_pt, size_t _payload_len, size_t *_data_len);
size_t calculatePacketSize(size_t data_len);
uint16_t buildPacket(uint8_t *packet_buffer,
                     const uint8_t *data_in_payload,
                     size_t data_len_in_payload,
                     const char message_type[2]);

#endif