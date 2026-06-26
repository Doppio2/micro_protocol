#if !defined(_MICRO_PROTOCOL_H)
#define _MICRO_PROTOCOL_H

#include "stdint.h"
#include "stddef.h"

#include "nanopb/message.pb.h"
#include "nanopb/pb_encode.h"
#include "nanopb/pb_decode.h"
#include "nanopb/pb_common.h"

/* Macros */
// NOTE(denis): я не знаю, куда это еще полжоить, 
// но зависимость от main.h как будто бы не самая лучшая идея в любом случае.
// Так же я не знаю пока что лучше, делать два отдельных буфера 
// для чтения и записи или 1 большой для обоих. Пока оставлю как в оригинале.
// Префикс добавил, чтоб не было конфликтов имен в основном коде программы.
#define MICRO_PROTOCOL_MAX_UART_DATA_PACKET_SIZE 512                          // Максимальный размер пакета данных по UART(rx/tx)

#define SYNC_SEQ      "ROV"   // Последовательность символов синхронизации пакета данных
#define SYNC_SEQ_BYTES  3     // Кол-во байт синхропоследовательности
#define LENGTH_BYTES    2     // Кол-во байт блока полезных данных PAYLOAD
#define TYPE_BYTES      2     // Кол-во байт типа сообщения (внутри первые байты блока полезных данных PAYLOAD)
#define CRC16_BYTES     2

#define HEADER_SIZE (SYNC_SEQ_BYTES + LENGTH_BYTES)                    // Длина заголовка пакета
#define MAX_PACKET_SIZE MICRO_PROTOCOL_MAX_UART_DATA_PACKET_SIZE       // Максимальная длина всего пакета.
#define MIN_PACKET_SIZE (HEADER_SIZE + TYPE_BYTES + CRC16_BYTES)       // Минимальная длинна пакета(protobuf данные отсутствуют) 
#define MAX_PAYLOAD_SIZE (MAX_PACKET_SIZE - HEADER_SIZE - CRC16_BYTES) // Максимальный допустимый размер protobuf данные + TYPE_LENGTH.

// Типы сообщения в s16 в little endian.
#define COMMAND_TYPE_CMD_NONE 0
#define COMMAND_TYPE_CMD_GET_TELEMETRY     ((u16)(('T' << 8) | 'G'))     // GT: Get Telemetry. Запрос телеметрии
#define COMMAND_TYPE_CMD_UPDATE_FIRMWARE   ((u16)(('F' << 8) | 'U'))     // UF: Update Firmware. Обновление прошивки
#define COMMAND_TYPE_CMD_THRUSTERS_CONTROL ((u16)(('C' << 8) | 'T'))     // TC: Thrusters Control. Управление движителями ДРК
#define COMMAND_TYPE_CMD_HARDWARE_CONTROL  ((u16)(('C' << 8) | 'H'))     // HC: Hardware Control. Управление группой периферийных ШИМ-устройств

// NOTE(denis): хэлперы, чтоб много не писать. Могу сразу сразу оставить нижний вариант.
// Сделал больше для документации.
#define CMD_NONE COMMAND_TYPE_CMD_NONE 
#define CMD_GT   COMMAND_TYPE_CMD_GET_TELEMETRY     
#define CMD_UF   COMMAND_TYPE_CMD_UPDATE_FIRMWARE   
#define CMD_TC   COMMAND_TYPE_CMD_THRUSTERS_CONTROL 
#define CMD_HC   COMMAND_TYPE_CMD_HARDWARE_CONTROL  

// Использовать, если нужны значения буфером packet и payload по-умолчанию.
#define MicroProtocolInitBuffersByDefault(packet)                                  \
(                                                                                  \
    (packet).packet_buffer = MICRO_PROTOCOL_DEFAULT_PACKET_BUFFER,                 \
    (packet).protobuf_buffer = MICRO_PROTOCOL_DEFAULT_PAYLOAD_BUFFER,              \
    (packet).protobuf_buffer_size = sizeof(MICRO_PROTOCOL_DEFAULT_PAYLOAD_BUFFER)  \
)                                                                                  \

// NOTE(denis): Заполнение структуры
#define MicroProtocolFillPacket(packet, type, data_struct, command)                                                  \
(                                                                                                                    \
    ((packet).command_type) = (command),                                                                             \
    ((packet).protobuf_ostream = pb_ostream_from_buffer((packet).protobuf_buffer, (packet).protobuf_buffer_size)),   \
    ((packet).status = pb_encode(&(packet).protobuf_ostream, type##_fields, &(data_struct)))                         \
)

// Сборка пакета.
#define MicroProtocolBuildPacket(packet)                                                                                                                                                   \
(                                                                                                                                                                                          \
    ((packet).status)                                                                                                                                                                      \
    ?                                                                                                                                                                                      \
    (                                                                                                                                                                                      \
        (packet).packet_buffer_len = micro_protocol_build_packet((packet).packet_buffer, (packet).protobuf_buffer, (packet).protobuf_ostream.bytes_written, (packet).command_type)         \
    )                                                                                                                                                                                      \
    :                                                                                                                                                                                      \
    (                                                                                                                                                                                      \
        (packet).packet_buffer_len = micro_protocol_build_packet((packet).packet_buffer, (const u8*)"ERROR\r\n", strlen("ERROR\r\n"), (packet).command_type)                               \
    )                                                                                                                                                                                      \
)

// NOTE(denis): как вариант можно написать так. Я пока думаю.
// Как лучше реально не знаю.
/*
#define MicroProtocolFillPacket(packet, packet_buffer, protobuf_buffer, type, data_struct, command)         \
do                                                                                                          \
{                                                                                                           \
    (packet).command_type = (command);                                                                      \
    (packet).bytes = (packet_buffer);                                                                         \
    (packet).protobuf_buffer = (protobuf_buffer);                                                            \
    (packet).protobuf_ostream = pb_ostream_from_buffer((packet).protobuf_buffer, sizeof((protobuf_buffer))); \
    (packet).status = pb_encode(&(packet).protobuf_ostream, type##_fields, &(data_struct)); \
} while(0)

#define MicroProtocolInitBuffersByDefault(packet) \
do { \
    (packet).packet_buffer = MICRO_PROTOCOL_DEFAULT_PACKET_BUFFER; \
    (packet).protobuf_buffer = MICRO_PROTOCOL_DEFAULT_PAYLOAD_BUFFER; \
} while(0)
*/

/* Types */
typedef int8_t      s8;
typedef int16_t     s16;
typedef int32_t     s32;
typedef int64_t     s64;

typedef uint8_t     u8;
typedef uint16_t    u16;
typedef uint32_t    u32;
typedef uint64_t    u64;

typedef float       f32;
typedef double      f64;

static u8 MICRO_PROTOCOL_DEFAULT_PACKET_BUFFER[MICRO_PROTOCOL_MAX_UART_DATA_PACKET_SIZE] = {0};
static u8 MICRO_PROTOCOL_DEFAULT_PAYLOAD_BUFFER[MAX_PAYLOAD_SIZE] = {0};

typedef struct Micro_Protocol_Build_Context
{
    u8 *packet_buffer;                   // Буфер для всего пакета.
    u8 *protobuf_buffer;                 // Буфер для protobuf.
    size_t packet_buffer_len;                       
    size_t protobuf_buffer_size;         // Для protobuf нужна информация о размере буфера. При создании структуры нужно заранее ее заполнить.
    pb_ostream_t protobuf_ostream;       // Private.
    u16 command_type;                    
    bool status;                         // Private.
} Micro_Protocol_Build_Context;

typedef struct Micro_Protocol_Packet
{
    u8 *Data;
    size_t Len;
} Micro_Protocol_Packet;

/* Functions */
// CRC16
u16 crc16(const u8 *data, size_t len);
u16 crc16_lookup_table(const u8 *data, size_t len);
u8 *micro_protocol_packet_parsing(u8 *packet, size_t packet_len, 
                                  size_t *payload_len);
u8 *micro_protocol_get_data_point_from_payload(u8 *payload, size_t payload_len,
                                               size_t protobuf_data_len);
size_t micro_protocol_get_packet_size(size_t protobuf_data);
size_t micro_protocol_build_packet(u8 *packet_buffer, const u8 *protobuf_data,
                                   size_t protobuf_data_len, u16 message_type);


#endif 
