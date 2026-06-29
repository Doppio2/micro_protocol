#include "assert.h"

// NOTE(denis): я думаю как лучше положить эти файлики.
// Пока что они будут тут включены просто как source файлы.
// Но это временно.

// Source.
#include "nanopb/message.pb.c"
#include "nanopb/pb_encode.c"
#include "nanopb/pb_decode.c"
#include "nanopb/pb_common.c"

// NOTE(denis): я понятия не имею, как это работает.
// Но оно работает....
static const u16 crc_table[256] = 
{
    0x0000,0xC0C1,0xC181,0x0140,0xC301,0x03C0,0x0280,0xC241,
    0xC601,0x06C0,0x0780,0xC741,0x0500,0xC5C1,0xC481,0x0440,
    0xCC01,0x0CC0,0x0D80,0xCD41,0x0F00,0xCFC1,0xCE81,0x0E40,
    0x0A00,0xCAC1,0xCB81,0x0B40,0xC901,0x09C0,0x0880,0xC841,
    0xD801,0x18C0,0x1980,0xD941,0x1B00,0xDBC1,0xDA81,0x1A40,
    0x1E00,0xDEC1,0xDF81,0x1F40,0xDD01,0x1DC0,0x1C80,0xDC41,
    0x1400,0xD4C1,0xD581,0x1540,0xD701,0x17C0,0x1680,0xD641,
    0xD201,0x12C0,0x1380,0xD341,0x1100,0xD1C1,0xD081,0x1040,
    0xF001,0x30C0,0x3180,0xF141,0x3300,0xF3C1,0xF281,0x3240,
    0x3600,0xF6C1,0xF781,0x3740,0xF501,0x35C0,0x3480,0xF441,
    0x3C00,0xFCC1,0xFD81,0x3D40,0xFF01,0x3FC0,0x3E80,0xFE41,
    0xFA01,0x3AC0,0x3B80,0xFB41,0x3900,0xF9C1,0xF881,0x3840,
    0x2800,0xE8C1,0xE981,0x2940,0xEB01,0x2BC0,0x2A80,0xEA41,
    0xEE01,0x2EC0,0x2F80,0xEF41,0x2D00,0xEDC1,0xEC81,0x2C40,
    0xE401,0x24C0,0x2580,0xE541,0x2700,0xE7C1,0xE681,0x2640,
    0x2200,0xE2C1,0xE381,0x2340,0xE101,0x21C0,0x2080,0xE041,
    0xA001,0x60C0,0x6180,0xA141,0x6300,0xA3C1,0xA281,0x6240,
    0x6600,0xA6C1,0xA781,0x6740,0xA501,0x65C0,0x6480,0xA441,
    0x6C00,0xACC1,0xAD81,0x6D40,0xAF01,0x6FC0,0x6E80,0xAE41,
    0xAA01,0x6AC0,0x6B80,0xAB41,0x6900,0xA9C1,0xA881,0x6840,
    0x7800,0xB8C1,0xB981,0x7940,0xBB01,0x7BC0,0x7A80,0xBA41,
    0xBE01,0x7EC0,0x7F80,0xBF41,0x7D00,0xBDC1,0xBC81,0x7C40,
    0xB401,0x74C0,0x7580,0xB541,0x7700,0xB7C1,0xB681,0x7640,
    0x7200,0xB2C1,0xB381,0x7340,0xB101,0x71C0,0x7080,0xB041,
    0x5000,0x90C1,0x9181,0x5140,0x9301,0x53C0,0x5280,0x9241,
    0x9601,0x56C0,0x5780,0x9741,0x5500,0x95C1,0x9481,0x5440,
    0x9C01,0x5CC0,0x5D80,0x9D41,0x5F00,0x9FC1,0x9E81,0x5E40,
    0x5A00,0x9AC1,0x9B81,0x5B40,0x9901,0x59C0,0x5880,0x9841,
    0x8801,0x48C0,0x4980,0x8941,0x4B00,0x8BC1,0x8A81,0x4A40,
    0x4E00,0x8EC1,0x8F81,0x4F40,0x8D01,0x4DC0,0x4C80,0x8C41,
    0x4400,0x84C1,0x8581,0x4540,0x8701,0x47C0,0x4680,0x8641,
    0x8201,0x42C0,0x4380,0x8341,0x4100,0x81C1,0x8081,0x4040
};

/**
 * @brief Находит синхропосылку в переданном пакете.
 *
 * @param[in]  data 		        Данные, из которых нужно считать контрольную сумму.
 * @param[in]  data_len 	        Длинна данных.
 * @return u16              	    Контрольная сумма.
 */
inline u16 crc16(const u8 *data, size_t len)
{
	u16 crc = 0xFFFF; // Инициализация CRC

    for(size_t i = 0; i < len; i++)
    {
        u8 table_index = (crc ^ data[i]) & 0xFF;
        crc = (crc >> 8) ^ crc_table[table_index];
    }

    return crc;
}

/**
 * @brief Находит синхропосылку в переданном пакете.
 *
 * @param[in]  buffer 		        Буфер данных, в котором нужно найти синхропосылку
 * @param[in]  buffer_len 	        Длинна буфера.
 * @return u8*              	    Указатель на синхропосылку в буфере.
 */
static inline u8 *find_sync_sequence(uint8_t *buffer, size_t buffer_len)
{
    if(buffer_len < SYNC_SEQ_BYTES)
    {
        return NULL;
    }

    // NOTE(denis): я не уверен, заинлайнит ли компилятор memcpy,
    // поэтому я просто лучше проверю вручную.
    for(size_t index = 0; 
        index <= buffer_len - SYNC_SEQ_BYTES; 
        index++)
    {
        if(buffer[index] == SYNC_SEQ[0] &&
           buffer[index+1] == SYNC_SEQ[1] &&
           buffer[index+2] == SYNC_SEQ[2])
        {
            return &buffer[index];
        }
    }

    return NULL;
}

/**
 * @brief Формирует пакет данных согласно протоколу.
 *
 * @param[out] packet_buffer     	Указатель на буфер, куда будет записан пакет
 * @param[in]  protobuf_data 		Указатель на данные блока полезной нагрузки (без типа пакета)
 * @param[in]  protobuf_data_len 	Длина данных полезной нагрузки (без типа пакета)
 * @param[in]  message_type      	Тип сообщения в 16 ричном числе(например,COMMAND_TYPE_CMD_UPDATE_FIRMWARE)
 * @return size_t                	Общая длина сформированного пакета.
 */
size_t _micro_protocol_build_packet(u8 *packet_buffer, const u8 *protobuf_data,
                                   size_t protobuf_data_len, u16 message_type)
{
    u8 *buffer = packet_buffer; 

	// 1. Записываем синхропосылку
	memcpy(buffer, SYNC_SEQ, SYNC_SEQ_BYTES);
	buffer += SYNC_SEQ_BYTES;

    // 2. Записываем длину Payload(protobuf_data_len + message_type_len. messaeg_type всегда 2 байта).
    u16 payload_len = protobuf_data_len + TYPE_BYTES;
    if(payload_len > MAX_PAYLOAD_SIZE)
    {
        // TODO(denis): Не знаю, используют ли в embeded assert.
        return 0;
    }
    buffer[0] = (u8)(payload_len & 0xFF);
    buffer[1] = (u8)((payload_len >> 8) & 0xFF);
    buffer += LENGTH_BYTES;

    // 3. Записываем тип команды.
    // message_type изначально хранится как little endian число 'T' и 'G'.
    // после преобразования снизу в буфере будет 'G', 'T'.
    buffer[0] = u8(message_type & 0xFF);
    buffer[1] = u8((message_type >> 8) & 0xFF);
    buffer += LENGTH_BYTES;

    // 4. Копируем полезаные данные payload(protobuf_data).
    if(protobuf_data_len > 0 && protobuf_data != NULL)
    {
        memcpy(buffer, protobuf_data, protobuf_data_len);
        buffer += protobuf_data_len;
    }

    // 5. Вычисляем CRC16 контрольную сумму от всего загаловка + полезной нагрузки.
    u16 crc = crc16(packet_buffer, (buffer - packet_buffer));

	// 6. Записываем CRC в конец пакета.
	buffer[0] = (u8)(crc & 0xFF);		   // Low byte CRC
	buffer[1] = (u8)((crc >> 8) & 0xFF);   // High byte CRC
	buffer += CRC16_BYTES;

	return (u16)(buffer - packet_buffer); // Возвращаем общую длину пакета
}

// TODO(denis): определиться какой код возвращать при ошибках.
// Мб сделать enum с типами ошибок.
/**
 * @brief Парсинг пакета данных в соответствии со структурой:
 * [SyncSeq (напр., "ROV")][Length (2 байта)][Type][ProtobufData][CRC (2 байта)]
 * |		    	HEADER			        |		PAYLOAD  	 |
 * @param payload     Указатель на структуру, содержащую данные payload и тип запроса.
 * @param packet      Структура, содержащая пакет, который нужно обработать.
 * @return            статус парсинга. 
 */

int micro_protocol_packet_parsing(Micro_Protocol_Payload *payload, 
                                  Micro_Protocol_Packet packet)
{
    printf("Bytes read: %zu\n", packet.len);
    // Порядок байтов Little endian.

    // 1. Проверяем длинну принятых байт.
    if(packet.len < MIN_PACKET_SIZE || packet.len > MAX_PACKET_SIZE)
    {
        return 0;
    }

    // 2. Ищем синхропосылку.
    u8 *sync_pos = find_sync_sequence(packet.data, packet.len);
    if(!sync_pos)
    {
        return 0;
    }
    u8 *current_packet_pos = sync_pos;      // Обновляем указатель на начало данных до начала синхропосылки.

    // 3. Размер доступных байт пакета.
    size_t packet_bytes_available = packet.len - (sync_pos - packet.data);
    // 4. Проверяем текущую длинну пакета. Она должна быть размером с HEADER_SIZE
	if(packet_bytes_available < HEADER_SIZE)
    {
		return 0;
    }

	// 5. Читаем длину блока полезных данных PAYLOAD, включающий тип пакета и бинарные данные в формате Protobuf
    // payload_len мы передали как указатель в функцию
    payload->data_len = (current_packet_pos[4] << 8) | current_packet_pos[3];      // Length_LowByte_Length_HighByte
	size_t total_packet_len = HEADER_SIZE + payload->data_len + CRC16_BYTES;

    // 6. Если нам доступно меньше, чем общая длинна пакета, то 
    // возвращаем NULL.
    if(packet_bytes_available < total_packet_len)
    {
        return 0;
    }

    // 6. Записываем тип команды.
    payload->cmd_type = (current_packet_pos[6] << 8) | current_packet_pos[5];

    // 7. Вычисляем и получаем crc пакета.
    u8 *crc_start = current_packet_pos + HEADER_SIZE + payload->data_len;
	u16 received_crc = (crc_start[1] << 8) | crc_start[0];

    // 8. Вычисляем crc пакета. Контрольная сумма должна совпадать.
    u16 calcuated_crc = crc16(current_packet_pos, HEADER_SIZE + payload->data_len);
    if(received_crc != calcuated_crc)
    {
        return 0;
    }

    // 9. Возвращаем указатель на начало блока payload.
    payload->data = current_packet_pos + HEADER_SIZE; 

    // 10. Сдвигаем все данные в структуре, чтобы они указывали на данные protobuf буфера.
    micro_protocol_get_data_point_from_payload(&payload->data, &payload->data_len);

    return 1;
}

// TODO(denis): так же решить, что делать с ошибками.
/**
 * @brief Извлекает указатель на данныеиз полезной нагрузки данные в соответствии со структурой пакета
 * @param payload указатель на указатель на блок полезных данных
 * @param payload_len длина PAYLOAD
 * @param статус ошибки.
 */
inline int micro_protocol_get_data_point_from_payload(u8 **payload, size_t *payload_len)
{
    if(*payload_len <= 2 || *payload == NULL)
    {
        return 0;
    }

    size_t temp_data_len = *payload_len;
    *payload_len = temp_data_len - TYPE_BYTES;
    *payload += TYPE_BYTES;

    return 1;
}

// NOTE(denis): в примере окружения было 0 вызовов.
// Я не знаю используется ли это где-то в проекте или нет.
/**
 * @brief Рассчитывает длину пакета согласно протоколу
 * @param data_len длина данных в полезной нагрузке (без типа пакета)
 * @return длина пакета. 0 в случае ошибки
 */
inline size_t micro_protocol_get_packet_size(size_t data_len)
{
    size_t len = HEADER_SIZE + (TYPE_BYTES + data_len) + CRC16_BYTES;
    if(len > MAX_PACKET_SIZE)
    {
        return 0;
    }

    return 0;
}
