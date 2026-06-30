# Бинарный протокол для взаимодействия со встраиваемыми системами.
Протокол работает поверх nanopb(*порт protobuf встраиваемых систем* https://github.com/nanopb/nanopb).

## Quickstart
Все примеры лежат в src/ вместе с файлами библиотеки.

Для сборки пакета и его дальшейшей отправки нужно сделать следующее:
``` cpp

    // 1. Сначала инициализируем структуру, которую будем сериализировать.
    Telemetry telemetry = Telemetry_init_zero;
    
    // 2. Заполняем ее нужными данными.
    // В данном случае функция get_test_telemetry_data() генерирует для нас случайный набора данных.
    // Тело функции доступно в src/main.cpp
    get_test_telemetry_data(&telemetry);
 
    // 3. Далее нам нужно инициализировать структуру Micro_Protocol_Packet. В некоторых компиляторах
    Micro_Protocol_Build_Context packet = {0}; 
 
    // 4. Даллее нам нужно указать структуре какие буферы ей использовать для всех данных (packet_buffer) 
    // и для данных, которые выдаст нам protobuf.
    // В библиотеке есть макрос micro_protocol_init_write_buffer_default, который использует заранее созданный
    // статичный буфер.
    // Его определение выглядит следующим образом.
    // 
    // static u8 MICRO_PROTOCOL_DEFAULT_PACKET_BUFFER[MICRO_PROTOCOL_MAX_UART_DATA_PACKET_SIZE] = {0};
    //
    // Если вы хотите использовать какой-либо свой буфер, вы можете вызывать micro_protocol_init_write_buffer, передав туда ваш буфер и packet структуру.
    //
    micro_protocol_init_write_buffer(packet);
    
    micro_protocol_build_packet(packet, Telemetry, telemetry, CMD_GT); 
    
    // Далее можем отправлять наши данные.
```  

Для десериализации и дальнейшей отправки:

``` cpp
    // После получения наших данных

    // 2. Вызываем функцию для парсинга. 
    // Перед этим нулем нужно инициализировать структуру Micro_Protocol_Payload.
    Micro_Protocol_Payload payload = {0};
    micro_protocol_packet_parsing(&payload, recieved_packet); 

    // 3. Дессериализуем.
    // Создаем структуру для данных и структуру для вспомогательных данных сереализации.
    Telemetry telemetry_output = Telemetry_init_zero;
    Micro_Protocol_Deserialize_Result deserialized = {0};

    // 4. Вызываем функцию, передав ему вспомогательную стуктуру, тип структуры, в которой будут храниться данные, структуру с данными и payload.
    micro_protocol_deserialize(deserialized, Telemetry, telemetry_output, payload);

    // Если функция вернула положительный статус, то вызываем нашу функцию для отправки.
    if(deserialized.status)
    {
        // Отправляем данные.
    }
```

## Сборка
Вы можете подключить библиотеку как отдельный элемент сборки: 

``` bash
clang++ \
    main.cpp \
    micro_protocol.c \
    -o app
```

Если вы предпочитаете использовать unity build, то можете спокойной включить micro_protocol.h и micro_protocol.c в основной файл вашей программы.
``` cpp
// Header.
#include "micro_protocol.h"

// Source.
#include "micro_protocol.c"

int main()
{
    Telemetry telemetry = Telemetry_init_zero;

    get_test_telemetry_data(&telemetry);

    Micro_Protocol_Build_Context packet = {0};
    micro_protocol_init_write_buffers_default(packet); 
    micro_protocol_build_packet(packet, Telemetry, telemetry, CMD_GT); 
}
```

Если используете cmake:
``` cmake
cmake_minimum_required(VERSION 3.20)

project(MicroProtocol)

add_executable(app
    main.cpp
    micro_protocol.c
)

target_include_directories(app PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
)
```

### Сборка примера
Для сборки примера на linux:
``` bash
cd micro_protocol && build.sh

Для windows:
cd micro_protocol && build.bat.
```


## Проблемы.

Протокол не завершен на 100%. Есть возможности его сделать быстрее и лучше оформить api, но базовый функционал он выполняет.
