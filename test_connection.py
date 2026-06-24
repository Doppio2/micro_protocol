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

def run_stm32flash(port, baud_rate):
    firmware_path = input(f"{Fore.YELLOW}Введите путь к файлу прошивки .bin: {Style.RESET_ALL}")
    command = [
        "stm32flash",
        "-w", firmware_path,
        "-v",
        "-g", "0x08000000",
        "-b", str(baud_rate),
        port
    ]
    reset_command = ["stm32flash", "-R", port]

    try:
        print(f"{Fore.GREEN}[+] Выполняется прошивка...{Style.RESET_ALL}")
        result = subprocess.run(command, check=True)
        if result.returncode == 0:
            print(f"{Fore.GREEN}[OK] Прошивка успешно загружена.{Style.RESET_ALL}")
        else:
            print(f"{Fore.RED}[Ошибка] Ошибка при загрузке прошивки.{Style.RESET_ALL}")
            print(f"{Fore.MAGENTA}[RESET] Попытка сбросить устройство...{Style.RESET_ALL}")
            subprocess.run(reset_command, check=True)
            print(f"{Fore.GREEN}[RESET OK] Устройство сброшено.{Style.RESET_ALL}")

    except FileNotFoundError as e:
        print(f"{Fore.RED}[Ошибка] Утилита stm32flash не найдена. Убедитесь, что она установлена.{Style.RESET_ALL}")
    except subprocess.CalledProcessError as e:
        print(f"{Fore.RED}[Ошибка] Ошибка выполнения команды: {e}{Style.RESET_ALL}")
        print(f"{Fore.MAGENTA}[RESET] Попытка сбросить устройство...{Style.RESET_ALL}")
        try:
            subprocess.run(reset_command, check=True)
            print(f"{Fore.GREEN}[RESET OK] Устройство сброшено.{Style.RESET_ALL}")
        except Exception as re:
            print(f"{Fore.RED}[Ошибка сброса] Не удалось сбросить устройство: {re}{Style.RESET_ALL}")

def main(port, baud_rate):
    try:
        ser = serial.Serial(port, baud_rate, timeout=1)
    except Exception as e:
        print(f"{Fore.RED}[Ошибка] Не удалось открыть порт: {e}{Style.RESET_ALL}")
        return

    while True:
        print("\n--- Создание нового пакета ---")
        packet_type = input(f"{Fore.YELLOW}Введите тип сообщения (2 символа, например 'ID'): {Style.RESET_ALL}").strip()
        if len(packet_type) != 2:
            print(f"{Fore.RED}[Ошибка] Тип должен быть ровно 2 символа.{Style.RESET_ALL}")
            continue

        include_data = input(f"{Fore.YELLOW}Добавить данные в пакет? (y/n): {Style.RESET_ALL}").strip().lower() == 'y'
        data = b''

        if include_data:
            user_data = input(f"{Fore.YELLOW}Введите данные (в HEX формате или текст): {Style.RESET_ALL}").strip()
            try:
                data = bytes.fromhex(user_data)
            except ValueError:
                data = user_data.encode('utf-8')

        # Формируем пакет
        try:
            packet = create_packet(packet_type, data)
        except Exception as e:
            print(f"{Fore.RED}[Ошибка создания пакета]: {e}{Style.RESET_ALL}")
            continue

        print_hex(packet)

        send = input(f"{Fore.YELLOW}Отправить пакет? (y/n): {Style.RESET_ALL}").strip().lower()
        if send == 'y':
            ser.write(packet)
            print(f"{Fore.GREEN}[+] Пакет отправлен{Style.RESET_ALL}")

            # Ожидаем ответ в течение 3 секунд
            response = b''
            timeout = 3.0
            start_time = time.time()

            while (time.time() - start_time) < timeout:
                if ser.in_waiting:
                    response += ser.read(ser.in_waiting)
                    break
                time.sleep(0.1)

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
                print(f"{Fore.RED}Ответ не получен за 3 секунды.{Style.RESET_ALL}")

            # Если это была команда Update Firmware - спросить о прошивке
            if packet_type.upper() == "UF":
                do_flash = input(f"{Fore.MAGENTA}Команда 'UF' отправлена. Запустить stm32flash для прошивки? (y/n): {Style.RESET_ALL}").strip().lower()
                if do_flash == 'y':
                    run_stm32flash(port, baud_rate)
                    continue  # после прошивки пропускаем ожидание ответа
                else:
                    print(f"{Fore.YELLOW}Прошивка отменена. Можно продолжить работу.{Style.RESET_ALL}")
        else:
            print(f"{Fore.MAGENTA}Отправка отменена.{Style.RESET_ALL}")

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