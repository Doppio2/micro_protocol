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

void print_telemetry_data(Telemetry *telemetry)
{
    printf("Telemetry.msg_id: %d\n", telemetry->msg_id);
    printf("Telemetry.has_power_monitor: %d\n", telemetry->has_power_monitor);
    printf("Telemetry.power_monitor.voltage: %f\n", telemetry->power_monitor.voltage);
    printf("Telemetry.power_monitor_current: %f\n", telemetry->power_monitor.current);
}

void print_raw_packet(Micro_Protocol_Build_Context *packet)
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
    // Build packet example.

    // 1. Сначала инициализируем структуру, которую будем сериализировать.
    Telemetry telemetry = Telemetry_init_zero;

    // 2. Заполняем ее нужными данными. В данном случае функция get_test_telemetry_data() генерирует для нас 
    // Случайный набор данных. Практики они не несут.
    get_test_telemetry_data(&telemetry);

    // 3. Далее нам нужно инициализировать структуру Micro_Protocol_Packet. В некоторых компиляторах
    // по-умолчанию включено предупреждение об инициализации через {0}. В этом случае используейте
    // макрос MicroProtocolPacketInitZero.
    Micro_Protocol_Build_Context packet = {0};

    //
    //
    // 4. Даллее нам нужно указать структуре какие буферы ей использовать для всех данных (packet_buffer) 
    // и для данных, которые выдаст нам protobuf.
    // В библиотеке есть макрос MicroProtocolInitBufferByDefault, который использует заранее созданные
    // статичные буферы для packet_buffer и payload_buffer. 
    // их определение выглядит следующим образом:
    // 
    // static u8 MICRO_PROTOCOL_DEFAULT_PACKET_BUFFER[MICRO_PROTOCOL_MAX_UART_DATA_PACKET_SIZE] = {0};
    // static u8 MICRO_PROTOCOL_DEFAULT_PAYLOAD_BUFFER[MAX_PAYLOAD_SIZE] = {0};
    //
    // Если вы хотите использовать вручную настроенный буфер, то заранее создайте его и заполните поле через:
    // 
    // packet.packet_buffer   =  ваш_буфер для packet   
    // packet.protobuf_buffer =  ваш_буфер для prortobuf.
    // packet.protobuf_buffer_size  = размер вашего буфера для protobuf.
    //
    // Примечание: поле .packet_buffer.len заполняется автоматически и является по-сути результатом функции для сборки пакета в бинарный формат
    // В то время как protobuf_buffer_size нужно указывать вручную, т.к функции pb_encode нужна информация и том, сколько байт нужно кодировать.
    // 
    //
    MicroProtocolInitBuffersByDefault(packet);


    // 4. Дальше нам нужно заполнить эту структуру остальными данными.
    // Далется это через макрос MicroProtocolFillPacket. 
    // Он не зависит от того, какой тип структуры вы хотите сериализировать т.к передаете название этого типа.
    MicroProtocolFillPacket(packet,                                     // Передаем только что созданный пакет.
                            Telemetry,                                  // Тип структуры, которую будем кодировать.
                            telemetry,                                  // Структура с данными.
                            CMD_GT);                                    // Тип команды. Варианты типов прописаны в HEADER.

    // 5. Самая функция сборки пакета в бинарный вид. 
    // На самом деле это тоже макрос. Он обертывает реальную функцию micro_protocol_build_packet.
    // Сама функция заполняет на основе поля status структуры Micro_Protocol_Packet 
    // кодирает либо сообщение с ошибкой, либо нужную структуру. 
    MicroProtocolBuildPacket(packet);

    // Вывод.
    print_raw_packet(&packet);

    // Парсинг данных.

    // Получаем наши данные. (В идеяле тот должен пример с передачей данных 
    // по сокетам, но т.к пока этого нет, просто использует только что созданный.

    // Инициализируем штуки, которые нам нужны для парсинга данных.
    size_t payload_len = 0;
    uint8_t *payload_pt = micro_protocol_packet_parsing(packet.packet_buffer, packet.packet_buffer_len, &payload_len);
    size_t data_len = 0;
    uint8_t *data_pt = micro_protocol_get_data_point_from_payload(payload_pt, payload_len, &data_len); // data from PAYLOAD

    for(int index = 0; index < data_len; index++)
    {
        printf("%02X ", data_pt[index]);
    }
    printf("\n");


    // Декодируем там и т.д
    pb_istream_t istream;
    if(data_pt != NULL)
    {
        istream = pb_istream_from_buffer(data_pt, data_len);
    }

    Telemetry telemetry_output = Telemetry_init_zero;

    if (data_pt != NULL)
    {
        if (pb_decode(&istream, Telemetry_fields, &telemetry_output) == true)
        {
            print_telemetry_data(&telemetry_output);
        }
    }

    return 0;
}
