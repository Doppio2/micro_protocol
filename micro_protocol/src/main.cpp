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
    micro_protocol_init_write_buffers(packet);

    // 5. Теперь мы можем собрать пакет. Функция MicroProtocolBuildPacket(которая является макросом-оберткой для функции micro_protocol_build_packet.
    // Заполняет структуру packet нужными данными и собирает пакет, который затем можно отправить на устройство.
    // P.S: В данный момент это работает как макрос, но есть вариант сделать из этого inline функцию.
    micro_protocol_build_packet(packet, Telemetry, telemetry, CMD_GT);

    // Вывод.
    print_raw_packet(&packet);

    // Парсинг данных.

    // 1. Получаем наши данные. (В идеяле тот должен пример с передачей данных 
    // по сокетам, но т.к пока этого нет, просто использует только что созданный.
    // Пока что имитируем передачу.
    Micro_Protocol_Packet recieved_packet = {0};
    recieved_packet.data = packet.packet_buffer;
    recieved_packet.len = packet.packet_buffer_len;

    // 2. Вызываем функцию для парсинга. Перед этим нулем нужно инициализировать структуру Micro_Protocol_Payload
    Micro_Protocol_Payload payload = {0};
    micro_protocol_packet_parsing(&payload, recieved_packet); 

    // 3. Дессериализуем.
    // Создаем структуру для данных и структуру для вспомогательных данных сереализации.
    Telemetry telemetry_output = Telemetry_init_zero;
    Micro_Protocol_Deserialize_Result deserialized = {0};

    // 4. Вызываем макрос, перадв ему вспомогательную стуктуру, тип структуры, в которой будуот хранится данные.
    // Структуру с данными и payload.
    micro_protocol_deserialize(deserialized, Telemetry, telemetry_output, payload);

    // Отправляем данные.
    if(deserialized.status)
    {
        print_telemetry_data(&telemetry_output);
    }

    return 0;
}
