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

// Использовать, если нужны значения буфером packet и payload по-умолчанию.
#define MicroProtocolInitWriteBuffers(packet)                                      \
(                                                                                  \
    (packet).packet_buffer = MICRO_PROTOCOL_DEFAULT_PACKET_BUFFER,                 \
    (packet).protobuf_buffer = MICRO_PROTOCOL_DEFAULT_PAYLOAD_BUFFER,              \
    (packet).protobuf_buffer_size = sizeof(MICRO_PROTOCOL_DEFAULT_PAYLOAD_BUFFER)  \
)                                                                                  \

// Сборка пакета.
// NOTE(denis): лучше написать inline функцию.
#define MicroProtocolBuildPacket(packet, type, data_struct, command)                                                                                                                                                   \
(                                                                                                                                                                                          \
    ((packet).command_type) = (command),                                                                             \
    ((packet).protobuf_ostream = pb_ostream_from_buffer((packet).protobuf_buffer, (packet).protobuf_buffer_size)),   \
    ((packet).status = pb_encode(&(packet).protobuf_ostream, type##_fields, &(data_struct))),                        \
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

#define MicroProtocolDeserialize(deserialized, type, data_struct, payload) \
do                                                                         \
{                                                                          \
    if ((payload).data != NULL)                                            \
    {                                                                      \
        (deserialized).istream = pb_istream_from_buffer(                   \
            (payload).data, (payload).data_len);                           \
        (deserialized).status = pb_decode(                                 \
            &(deserialized).istream, type##_fields, &(data_struct));       \
    }                                                                      \
} while (0)

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

/* Constants */
const u16 CMD_NONE = 0x0000; 
const u16 CMD_GT   = 0x5447;   // GT: Get Telemetry. Запрос телеметрии
const u16 CMD_UF   = 0x4655;   // UF: Update Firmware. Обновление прошивки
const u16 CMD_TC   = 0x4354;   // TC: Thrusters Control. Управление движителями ДРК
const u16 CMD_HC   = 0x4348;   // HC: Hardware Control. Управление группой периферийных ШИМ-устройств

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
    u8 *data;
    size_t len;
} Micro_Protocol_Packet;

typedef struct Micro_Protocol_Payload
{
    u8 *data;
    size_t data_len;
    u16 cmd_type;
} Micro_Protocol_Payload;

typedef struct Micro_Protocol_Deserialize_Result 
{
    bool status;
    pb_istream_t istream;        // Private.
} Micro_Protocol_Deserialize_Result; 

/* Functions */
inline u16 crc16(const u8 *data, size_t len);
static inline uint8_t *find_sync_sequence(uint8_t *buffer, size_t buffer_len);
int micro_protocol_packet_parsing(Micro_Protocol_Payload *payload, Micro_Protocol_Packet packet);
inline int micro_protocol_get_data_point_from_payload(u8 **payload, size_t *payload_len);
size_t micro_protocol_get_packet_size(size_t protobuf_data);
size_t micro_protocol_build_packet(u8 *packet_buffer, const u8 *protobuf_data,
                                   size_t protobuf_data_len, u16 message_type);
inline size_t micro_protocol_get_packet_size(size_t data_len);


#endif 
