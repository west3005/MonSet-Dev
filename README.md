# MonSet-Dev

> **Ветка активной разработки** MonSet до продаваемой версии

[![Issues](https://img.shields.io/github/issues/west3005/MonSet-Dev)](https://github.com/west3005/MonSet-Dev/issues)
[![License: MIT](https://img.shields.io/badge/License-MIT-yellow.svg)](LICENSE)

## О проекте

MonSet — промышленное IoT-устройство для мониторинга технологических параметров на объектах ЖКХ и промышленности.

**Аппаратная платформа:**
- MCU: STM32F407VET6 (168 MHz, 512KB Flash, 192KB RAM)
- Сеть: W5500 (Ethernet) + SIM7020C (NB-IoT/Cat-M1)
- Датчики: DS3231 (RTC по I2C), Modbus RTU датчики (415-DI/ULM-31)
- Хранение: MicroSD карта (FAT32)
- Питание: основное 220V + резервный аккумулятор

**Что делает устройство:**
1. Считывает данные с датчиков по Modbus RTU (UART)
2. Синхронизирует время через NTP или DS3231
3. Логирует данные на SD-карту
4. Отправляет телеметрию на ThingsBoard (MQTT через Ethernet или NB-IoT)
5. Поддерживает удалённое управление и конфигурирование

## Архитектура прошивки

```
Core/
  Inc/
    app.hpp          # Главный класс App (координатор всех подсистем)
    config.hpp       # Конфигурация и параметры устройства
    power_manager.hpp # Управление питанием и перезагрузками
    modem_manager.hpp # Управление SIM7020C
    ethernet_manager.hpp # Управление W5500
    modbus_manager.hpp   # Opros датчиков по Modbus RTU
    sd_manager.hpp       # Работа с SD-картой
    rtc_manager.hpp      # DS3231 RTC
    mqtt_manager.hpp     # MQTT клиент
  Src/
    app.cpp
    power_manager.cpp
    modem_manager.cpp
    ... (реализации)
```

**Конечный автомат App:**
```
INIT → ETHERNET_INIT → NTP_SYNC → MODEM_INIT → SENSORS_POLL → SEND_DATA → SLEEP
                                                      ↑                        |
                                                      └────────────────────────┘
```

## Дорожная карта до продажи

| Неделя | Задачи | Статус |
|--------|--------|--------|
| W1 | Аудит, watchdog, PowerManager, SIM7020C, датчики | 🔄 |
| W2 | Runtime-конфиг, веб-интерфейс ESP8266, логирование | ⏳ |
| W3 | MQTT надёжность, TLS, OTA-обновление | ⏳ |
| W4 | Аккумулятор, энергосбережение, NB-IoT | ⏳ |
| W5 | Watchdog полный цикл, самодиагностика | ⏳ |
| W6 | Стресс-тест 72ч, производительность | ⏳ |
| W7 | Корпус, документация для монтажников | ⏳ |
| W8 | Сертификация, упаковка, версия 1.0 | ⏳ |

Подробные задачи: [Issues](https://github.com/west3005/MonSet-Dev/issues)

## Быстрый старт

### Требования
- STM32CubeIDE 1.13+
- ST-Link v2 или J-Link
- SD-карта FAT32 с файлом `config.json`

### Конфигурация (config.json на SD)
```json
{
  "device_id": "monset-001",
  "mqtt_host": "thingsboard.cloud",
  "mqtt_port": 1883,
  "mqtt_token": "YOUR_TOKEN",
  "modbus_baud": 9600,
  "sensors": [
    {"id": 1, "type": "415-DI", "address": 1},
    {"id": 2, "type": "ULM-31", "address": 2}
  ],
  "send_interval_sec": 60,
  "eth_dhcp": true,
  "ip": "192.168.1.100",
  "gateway": "192.168.1.1",
  "subnet": "255.255.255.0"
}
```

### Сборка и прошивка
```bash
# Клонировать репозиторий
git clone https://github.com/west3005/MonSet-Dev.git

# Открыть в STM32CubeIDE
# File → Open Projects from File System → выбрать папку

# Сборка: Ctrl+B
# Прошивка: Run → Run (ST-Link должен быть подключён)
```

## Критерии версии 1.0 (продаваемая)

- [ ] Стабильная работа 72+ часов без зависаний
- [ ] Автовосстановление после любого сбоя (watchdog)
- [ ] Веб-интерфейс настройки через Wi-Fi (ESP8266)
- [ ] Логирование на SD с ротацией файлов
- [ ] Надёжная отправка MQTT (очередь, повторные попытки)
- [ ] Документация для монтажника (PDF)
- [ ] Корпус DIN-рейка
- [ ] Тест при -20°C и +60°C

## Лицензия

MIT — см. [LICENSE](LICENSE)

---
*Этот репозиторий — рабочая ветка разработки. Стабильная версия: [west3005/MonSet](https://github.com/west3005/MonSet)*
