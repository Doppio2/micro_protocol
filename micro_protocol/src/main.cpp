// NOTE(denis): В примере они просто включены как файлы. 
#include "micro_protocol.h"
#include "micro_protocol.c"

#include "stdio.h"

// Не знаю как это сделать синтаксически, но щас проверим.
#define MicroProtocolInitZero ((Micro_Protocol_Packet){(MICRO_PROTOCOL_DEFAULT_BUFFER), 0})

typedef struct Micro_Protocol_Packet
{
    u8 *bytes;
    size_t len;
    /*
    u8 *protobuf_buffer;
    pb_ostream_t protobuf_ostream;

    Telemetry telemetry;
    */

} Micro_Protocol_Packet;

void get_test_telemetry_data(Telemetry *telemetry)
{
    // Вручную заполняем структуру.
    telemetry->msg_id = 1;
    telemetry->has_power_monitor = true;

    // Power monitor.
    telemetry->power_monitor.voltage = 20;
    telemetry->power_monitor.current = 50;
    telemetry->power_monitor.paf_1 = PAF_Status_PAF_OK;
    telemetry->power_monitor.paf_1 = PAF_Status_PAF_OK;
    telemetry->power_monitor.paf_1 = PAF_Status_PAF_OK;
    telemetry->power_monitor.paf_1 = PAF_Status_PAF_OK;
    telemetry->power_monitor.paf_1 = PAF_Status_PAF_OK;
}

void print_raw_packet(Micro_Protocol_Packet *packet)
{
    printf("Packet(%d bytes)", (int)packet->len);
    for(int index = 0; index < packet->len; index++)
    {
        printf("%02X ", packet->bytes[index]);
    }

    printf("\n");
}

int main()
{
    // Init этап для protobuf и 
    // Желательно это объеденить в однку команду.
    Telemetry telemetry = Telemetry_init_zero;
    Micro_Protocol_Packet packet = MicroProtocolInitZero;

    // Этап с заполнением структуры телеметрии.
    get_test_telemetry_data(&telemetry);

    // Вызываем протобаф, чтобы закодировать буфер.
	u8 protobuf_buffer[MAX_PAYLOAD_SIZE] = {0};

    pb_ostream_t ostream = pb_ostream_from_buffer(protobuf_buffer, sizeof(protobuf_buffer));
    bool status = pb_encode(&ostream, Telemetry_fields, &telemetry);

    if(status)
    {
        printf("pb_encode was succesfull\n");
        packet.len = micro_protocol_build_packet(packet.bytes, protobuf_buffer, ostream.bytes_written, CMD_GT);
    }
    else
    {
        // TODO(denis): придумать как можно нормально передавать ошибку.
        // Мне не очень нравится, как это сейчас выглядит.
        packet.len = micro_protocol_build_packet(packet.bytes, (const u8*)"ERROR\r\n", strlen("ERROR\r\n"), CMD_GT);
    }
    
    // Здесь должен быть код передачи данных на другое устройство.
    print_raw_packet(&packet);

    return 0;
}
