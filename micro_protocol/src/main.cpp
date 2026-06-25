// NOTE(denis): В примере они просто включены как файлы. 
#include "stdio.h"

// Header.
#include "micro_protocol.h"

// Source.
#include "micro_protocol.c"

// NOTE(denis): На сколько я понимаю, то макрос, который содержит поля для типов называется всегда
// _fields. Micro Protocol оринетируется на это.
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
    printf("Packet(%d bytes)", (int)packet->packet_buffer_len);
    for(int index = 0; index < packet->packet_buffer_len; index++)
    {
        printf("%02X ", packet->packet_buffer[index]);
    }

    printf("\n");
}

int main()
{
    // TODO(denis): ДУМАТЬ КАК ЭТО ВСЕ ОФОРМИТЬ С ПОМОЩЬЮ МАКРОСОВ И Т.Д.
    // Init этап для protobuf и 
    // Желательно это объеденить в однку команду.
    Telemetry telemetry = Telemetry_init_zero;
    // Этап с заполнением структуры телеметрии.
    get_test_telemetry_data(&telemetry);

    // MICRO_PROTOCOL_DEFAULT_PACKET_BUFFER сейчас не работает в данный момент.
    Micro_Protocol_Packet packet = {0};                                 // Добавить макрос init_zero???
    MicroProtocolInitBuffersByDefault(packet);
    MicroProtocolFillPacket(packet,                                     // Передаем только что созданный пакет.
                            Telemetry,                                  // Тип структуры, которую будем кодировать.
                            telemetry,                                  // Структура с данными.
                            CMD_GT);                                    // Тип команды.


    // Это так же макрос. Он обертывает изначаtьный вызов micro_protocol_build_packet.
    // Изначально он нужен был для того, чтоб не писать лишнюю проверку if(status).
    // Макрос сам при случае ошибки отправит данные с ERROR.
    // Длинна записанного пакета хранится в packet.len
    MicroProtocolBuildPacket(packet);

    print_raw_packet(&packet);

    return 0;
}
