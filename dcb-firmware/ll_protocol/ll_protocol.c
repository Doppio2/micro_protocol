#include "ll_protocol.h"
#include <stdio.h>

// Таблица соответствий типов командам
static const CommandMapping_t command_map[] = {
	{{'G', 'T'}, CMD_GET_TELEMETRY},	 // Get Telemetry
	{{'U', 'F'}, CMD_UPDATE_FIRMWARE},	 // Update Firmware
	{{'T', 'C'}, CMD_THRUSTERS_CONTROL}, // Thrusters Control
	{{'H', 'C'}, CMD_HARDWARE_CONTROL},	 // Hardware Control
};
#define COMMAND_COUNT (sizeof(command_map) / sizeof(CommandMapping_t))

uint16_t crc16(const uint8_t *data, size_t length);
uint8_t *find_sync_sequence(uint8_t *buffer, size_t buffer_len);

/**
 * @brief Парсинг пакета данных в соответствии со структурой:
 * [SyncSeq (напр., "ROV")][Length (2 байта)][Type][ProtobufData][CRC (2 байта)]
 * |		    	HEADER			        |		PAYLOAD  	 |
 * @param pData указатель на пришедший пакета
 * @param packet_len длина пришедшего пакета
 * @param _payload_len запишется длина блока данных PAYLOAD
 * @return указатель на начало блока PAYLOAD. NULL в случае ошибки
 */
uint8_t *packetParsing(uint8_t *pData, size_t data_len, size_t *_payload_len)
{
	/* Парсинг пакета */
	/* Порядок байтов: little endian */

	/* Проверка длины принятых байт */
	if (data_len < MIN_PACKET_SIZE || data_len > MAX_PACKET_SIZE)
		return NULL;

	// Поиск синхропосылки
	uint8_t *sync_pos = find_sync_sequence(pData, data_len);
	if (!sync_pos)
		return NULL; // Синхропосылка не найдена

	// printf("PACKET_FOUND. LENGTH %d\r\n", data_len);

	/* Новое начало отсчета пакета в приемном буфере */
	const uint8_t *packet_start = sync_pos;
	/* Размер доступных байт пакета */
	size_t packet_available = data_len - (sync_pos - pData);
	/**
	 * Для считывания длины сообщения проверяем необходимый размер
	 * (он должен включать синхропоследовательность + 2 байта длины = HEADER)
	 */
	if (packet_available < HEADER_SIZE)
		return NULL;

	/* Читаем длину блока полезных данных PAYLOAD, включающий тип пакета и бинарные данные в формате Protobuf */
	uint16_t payload_len = (packet_start[4] << 8) | packet_start[3]; // Length_LowByte_Length_HighByte
	size_t total_packet_len = HEADER_SIZE + payload_len + CRC_BYTES;

	// printf("PAYLOAD_LEN %d\r\n", payload_len);

	/* Проверка правильности размера доступной длины пакета. Если меньне - false */
	if (packet_available < total_packet_len)
		return NULL;

	/* Читаем CRC */
	uint8_t *crc_start = packet_start + HEADER_SIZE + payload_len;
	/* CRC_LowByte_CRC_HighByte */
	uint16_t received_crc = (crc_start[1] << 8) | crc_start[0];

	/* Вычисляем CRC пакета (заголовок + данные) */
	uint16_t calculated_crc = crc16(packet_start, HEADER_SIZE + payload_len);

	/* Проверка соответствия CRC */
	if (received_crc != calculated_crc)
		return NULL;

	// printf("CRC_OK\r\n");

	/* Сохраняем указатель и длину на блок PAYLOAD */
	uint8_t *_payload_pt = packet_start + HEADER_SIZE; // после HEADER
	*_payload_len = payload_len;

	return _payload_pt;
}

/**
 * @brief Извлекает из полезной нагрузки типа сообщения (команды) и возвращает его.
 * Тип сообщений передается в символьном виде
 * @param _payload_pt указатель на блок полезных данных
 * @param _payload_len длина PAYLOAD
 * @return CommandType_t
 */
CommandType_t getCommandType(uint8_t *_payload_pt, size_t _payload_len)
{
	if (_payload_len < 2)
		return CMD_NONE;

	for (size_t i = 0; i < COMMAND_COUNT; i++)
		if (memcmp(_payload_pt, command_map[i].type, 2) == 0)
			return command_map[i].command;

	return CMD_NONE;
}

/**
 * @brief Извлекает указатель на данныеиз полезной нагрузки данные в соответствии со структурой пакета
 * @param _payload_pt указатель на блок полезных данных
 * @param _payload_len длина PAYLOAD
 * @param _data_len запишется длина блока данных
 * @return указатель на данные
 */
uint8_t *getDataPointFromPayload(uint8_t *_payload_pt, size_t _payload_len, size_t *_data_len)
{
	if (_payload_len <= 2 || _payload_pt == NULL)
		return NULL;

	*_data_len = _payload_len - TYPE_BYTES;
	return _payload_pt + TYPE_BYTES;
}

/**
 * @brief Рассчитывает длину пакета согласно протоколу
 * @param data_len длина данных в полезной нагрузке (без типа пакета)
 * @return длина пакета. 0 в случае ошибки
 */
size_t calculatePacketSize(size_t data_len)
{
	size_t len = HEADER_SIZE + (TYPE_BYTES + data_len) + CRC_BYTES;
	if (len > MAX_PACKET_SIZE)
		return 0;
	return len;
}

/**
 * @brief Формирует пакет данных согласно протоколу.
 *
 * @param[out] packet_buffer     	Указатель на буфер, куда будет записан пакет
 * @param[in]  data_in_payload 		Указатель на данные блока полезной нагрузки (без типа пакета)
 * @param[in]  data_len_in_payload 	Длина данных полезной нагрузки (без типа пакета)
 * @param[in]  message_type      	Тип сообщения (2 символа, например "GT")
 * @return uint16_t              	Общая длина сформированного пакета
 */
uint16_t buildPacket(uint8_t *packet_buffer,
					 const uint8_t *data_in_payload,
					 size_t data_len_in_payload,
					 const char message_type[2])
{
	uint8_t *buf = packet_buffer;

	// 1. Записываем синхропосылку
	memcpy(buf, SYNC_SEQ, SYNC_LEN);
	buf += SYNC_LEN;

	// 2. Записываем длину PAYLOAD (2 байта, little-endian)
	/**
	 * Может возникнуть путанциа. Блок PAYLOAD в себе содержит а начле всегда два байта ТИПА СООБЩЕНИЯ.
	 * То есть, размер PAYLOAD от 2...MAX_PAYLOAD_SIZE.
	 * data_in_payload и data_len_in_payload относятся только части ДАННЫХ блока PAYLOAD.
	 */
	uint16_t payload_len = data_len_in_payload + TYPE_BYTES;
	if (payload_len > MAX_PAYLOAD_SIZE)
		return 0;
	buf[0] = (uint8_t)(payload_len & 0xFF);		   // Low byte
	buf[1] = (uint8_t)((payload_len >> 8) & 0xFF); // High byte
	buf += LENGTH_BYTES;

	// 3. Записываем тип команды
	memcpy(buf, message_type, TYPE_BYTES);
	buf += TYPE_BYTES;

	// 4. Копируем данные полезной нагрузки
	if (data_len_in_payload > 0 && data_in_payload != NULL)
	{
		memcpy(buf, data_in_payload, data_len_in_payload);
		buf += data_len_in_payload;
	}

	// 5. Вычисляем CRC16 от всего заголовка + полезной нагрузки
	uint16_t crc = crc16(packet_buffer, (buf - packet_buffer));

	// 6. Записываем CRC в конец пакета
	buf[0] = (uint8_t)(crc & 0xFF);		   // Low byte CRC
	buf[1] = (uint8_t)((crc >> 8) & 0xFF); // High byte CRC
	buf += CRC_BYTES;

	return (uint16_t)(buf - packet_buffer); // Возвращаем общую длину пакета
}

uint16_t crc16(const uint8_t *data, size_t length)
{
	uint16_t crc = 0xFFFF; // Инициализация CRC

	for (size_t i = 0; i < length; i++)
	{
		crc ^= (uint16_t)data[i]; // XOR с текущим байтом

		for (uint8_t j = 0; j < 8; j++)
		{
			if (crc & 0x0001)
			{				   // Если младший бит = 1
				crc >>= 1;	   // Сдвиг вправо
				crc ^= 0xA001; // XOR с полиномом
			}
			else
			{
				crc >>= 1; // Просто сдвиг
			}
		}
	}

	return crc;
}

uint8_t *find_sync_sequence(uint8_t *buffer, size_t buffer_len)
{
	for (size_t i = 0; i <= buffer_len - SYNC_LEN; i++)
	{
		if (memcmp(buffer + i, SYNC_SEQ, SYNC_LEN) == 0)
		{
			return buffer + i;
		}
	}
	return NULL;
}
