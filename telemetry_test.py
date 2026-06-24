import serial
import struct
import crcmod
import binascii
import sys
import time
import subprocess
from colorama import Fore, Style, init
import message_pb2

# Инициализация Colorama
init(autoreset=True)

# Константы
SYNC_SEQ = b'ROV'
SYNC_LEN = len(SYNC_SEQ)
LENGTH_FIELD_SIZE = 2
TYPE_FIELD_SIZE = 2
HEADER_SIZE = SYNC_LEN + LENGTH_FIELD_SIZE
CRC_SIZE = 2
MAX_PAYLOAD_SIZE = 1024

# CRC-16/Modbus
crc16 = crcmod.mkCrcFun(0x18005, rev=True, initCrc=0xFFFF, xorOut=0x0000)

def calculate_crc(data):
    return struct.pack('<H', crc16(data))

def create_packet(packet_type, data=b''):
    payload = packet_type.encode('utf-8') + data
    length = len(payload)
    if length > MAX_PAYLOAD_SIZE:
        raise ValueError(f"Payload слишком длинный: {length} байт. Максимум {MAX_PAYLOAD_SIZE}.")
    
    header = SYNC_SEQ + struct.pack('<H', length) + packet_type.encode('utf-8')
    crc = calculate_crc(header + data)
    packet = header + data + crc
    return packet

def is_ascii_data(data):
    """Проверяет, являются ли данные читаемой ASCII-строкой"""
    try:
        decoded = data.decode('ascii')
        # Проверяем, что строка не содержит управляющих символов
        if all(32 <= ord(c) < 127 for c in decoded):
            return True, decoded
        else:
            return False, None
    except UnicodeDecodeError:
        return False, None
    
def decode_protobuf_data(data):
    """Десериализует Protobuf данные на основе известных типов"""
    try:
        # Пробуем разные типы сообщений

        #power = message_pb2.PowerSwitchControl()
        #if power.ParseFromString(data):
        #    state_str = message_pb2.ChannelPowerSwitchStatus.Name(power.state)
        #    return f"PowerSwitch: Channel={power.channel}, State={state_str}"

        telemetry = message_pb2.Telemetry()
        if telemetry.ParseFromString(data):
            serial_status = message_pb2.ChannelPowerSwitchStatus.Name(telemetry.serial_dev_power_ch.status)
            usb1_status = message_pb2.ChannelPowerSwitchStatus.Name(telemetry.usb1_dev_power_ch.status)

            return (
                f"Telemetry:\n"
                f"  ID: {telemetry.msg_id}\n"
                f"  Timestamp: {telemetry.timestamp}\n"
                f"  Serial Dev Power: {serial_status}, Current: {telemetry.serial_dev_power_ch.current} A\n"
                f"  USB1 Dev Power: {usb1_status}, Current: {telemetry.usb1_dev_power_ch.current} A\n"
                f"  Pressure: {telemetry.pressure} Pa"
            )

        return "[Unknown Protobuf message]"
    except Exception as e:
        return f"[Protobuf error: {e}]"

def decode_packet(data):
    if len(data) < HEADER_SIZE:
        return None, None  # Слишком короткий ответ

    if data[:3] != b'ROV':
        return None, None  # Нет синхропосылки

    try:
        length = struct.unpack('<H', data[3:5])[0]
        payload_start = HEADER_SIZE
        payload_end = payload_start + length
        total_length = payload_end + CRC_SIZE

        if len(data) < total_length:
            return None, None  # Не хватает данных для полного пакета

        header_type = data[5:7].decode('ascii', errors='replace')  # Тип из заголовка
        payload = data[payload_start:payload_end]
        received_crc = data[payload_end:payload_end + CRC_SIZE]

        # Вычисляем CRC от HEADER + PAYLOAD
        crc_data = data[:payload_end]
        calculated_crc = calculate_crc(crc_data)
        crc_valid = received_crc == calculated_crc

        if not crc_valid:
            print(f"{Fore.RED}[CRC Error] Полученный CRC: {received_crc.hex()}, ожидается: {calculated_crc.hex()}{Style.RESET_ALL}")
            return None, None

        # Проверяем, является ли payload читаемой строкой
        is_ascii, ascii_str = is_ascii_data(payload)
        if is_ascii:
            return header_type, f"[ASCII] {ascii_str}"

        # Если не ASCII — пробуем как Protobuf
        if len(payload) >= 2:
            payload_type = payload[:2].decode('ascii', errors='replace')
            protobuf_data = payload[2:]

            decoded = decode_protobuf_data(protobuf_data)
            full_type = f"{header_type}/{payload_type}"  # Например: "CM/PS"
            return full_type, decoded
        else:
            return header_type, "[Empty or invalid payload]"

    except Exception as e:
        print(f"{Fore.RED}[Ошибка разбора пакета]: {e}{Style.RESET_ALL}")
        return None, None

def print_hex(data):
    hex_str = binascii.hexlify(data, ' ').decode('utf-8')
    print(f"{Fore.CYAN}Сформированный пакет (HEX):{Style.RESET_ALL}", hex_str)


def main(port, baud_rate):
    try:
        ser = serial.Serial(port, baud_rate, timeout=1)
    except Exception as e:
        print(f"{Fore.RED}[Ошибка] Не удалось открыть порт: {e}{Style.RESET_ALL}")
        return

    while True:
        print("\n--- Тестирование телеметрии (опрос в цикле) ---")
        interval = int(input(f"{Fore.YELLOW}Введите интервал опроса в мс (например, 5 мс): {Style.RESET_ALL}"))
        if interval < 2:
            print(f"{Fore.RED}[Ошибка] Интервал слишком маленький или введен неверно.{Style.RESET_ALL}")
            continue
        count = int(input(f"{Fore.YELLOW}Введите число опросов(например, 1000): {Style.RESET_ALL}"))
        if count == 0:
            print(f"{Fore.RED}[Ошибка] Число опросов не может быть равно нулю.{Style.RESET_ALL}")
            continue

        #print_hex(packet)

        send = input(f"{Fore.YELLOW}Начать опрос? (y/n) [Параметры: T={interval}, C={count}, TIME={interval*count/1000}]: {Style.RESET_ALL}").strip().lower()

        if send == 'y':
            dt = float(interval) / 1000
            for i in range(count):
                # Формируем пакет
                try:
                    packet = create_packet("GT", b'')
                except Exception as e:
                    print(f"{Fore.RED}[Ошибка создания пакета]: {e}{Style.RESET_ALL}")
                    continue

                ser.write(packet)
                #print(f"{Fore.GREEN}[+] Пакет отправлен{Style.RESET_ALL}")

                # Ожидаем ответ в течение таймаута
                response = b''
                timeout = 1.0
                start_time = time.time()

                
                time.sleep(dt)
                while (time.time() - start_time) < timeout:
                    if ser.in_waiting:
                        response += ser.read(ser.in_waiting)
                        break
                    time.sleep(dt)

                if response:
                    packet_type_receive, payload = decode_packet(response)
                    if packet_type_receive:
                        print(f"{Fore.CYAN}Получен ответ:{Style.RESET_ALL}")
                        print(f"  Тип команды: {Fore.YELLOW}{packet_type_receive}{Style.RESET_ALL}")
                        print(f"  Данные: {payload}")
                    else:
                        # Не пакет по протоколу — просто ASCII
                        try:
                            ascii_text = response.decode('ascii', errors='replace')
                            print(f"{Fore.CYAN}Получен ответ (ASCII):{Style.RESET_ALL}", ascii_text)
                        except Exception as e:
                            print(f"{Fore.RED}Невозможно декодировать ответ: {e}{Style.RESET_ALL}")
                else:
                    print(f"{Fore.RED}Ответ не получен.{Style.RESET_ALL}")

if __name__ == '__main__':
    if len(sys.argv) < 3:
        print(f"{Fore.RED}Использование:{Style.RESET_ALL}")
        print("  python script.py <порт> <скорость>")
        print("Пример Windows:")
        print("  python script.py COM4 9600")
        print("Пример Linux/Mac:")
        print("  python script.py /dev/ttyUSB0 115200")
        sys.exit(1)

    port = sys.argv[1]
    baud_rate = int(sys.argv[2])

    main(port, baud_rate)