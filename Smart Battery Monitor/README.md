Smart Battery Monitor — ESPHome (1–10 батарей)
На основе видео-проекта с ESP32 + TP4056 я подготовил полноценное решение для мониторинга от 1 до 10 батарей через ESPHome. Ниже — полный код, схема, список компонентов и принцип работы.
📋 Список компонентов
Таблица
Компонент	Количество	Примечание
ESP32 DevKit V1	1	Главный контроллер
ADS1115 16-бит ADC	3	4 канала каждый, I2C. Адреса: 0x48, 0x49, 0x4A
Резисторы 10 кОм	20	Делители напряжения (по 2 на батарею)
Резисторы 10 кОм (pull-up)	2	Для I2C (SDA, SCL) — если модуль ADS1115 не имеет встроенных
Керамические конденсаторы 100 нФ	10	Фильтрация помех на входах ADC
Батарейные холдеры 18650	10	Или под ваш тип батарей
TP4056	10	Модуль зарядки (1 на каждую ячейку, если зарядка индивидуальная)
Макетная плата / PCB	1	Для сборки
Провода Dupont	~50	Для соединений
Источник питания 5V	1	Для ESP32 (через USB или внешний)
Важно: TP4056 предназначен только для одной Li-ion ячейки (4.2V). Если вы планируете собрать последовательный пакет 10S, вместо 10 TP4056 используйте BMS 10S с балансировкой.
🔌 Схема подключения
plain
┌─────────────────────────────────────────────────────────────────┐
│                         ESP32 DevKit V1                         │
│                                                                 │
│  3.3V ──┬──────────────────────────────────────┬── ADS1115 #1  │
│         │                                      │   (0x48)      │
│  GND  ──┼──────────────────────────────────────┼── GND         │
│         │                                      │               │
│  GPIO21 ─┬─ SDA ───────────────────────────────┼── SDA         │
│  GPIO22 ─┴─ SCL ───────────────────────────────┼── SCL         │
│                                                │               │
│  3.3V ──┬──────────────────────────────────────┼── ADS1115 #2  │
│  GND  ──┼──────────────────────────────────────┼── (0x49)      │
│  SDA  ──┤ (параллельно)                        │               │
│  SCL  ──┘                                      │               │
│                                                │               │
│  3.3V ──┬──────────────────────────────────────┼── ADS1115 #3  │
│  GND  ──┼──────────────────────────────────────┼── (0x4A)      │
│  SDA  ──┤ (параллельно)                        │               │
│  SCL  ──┘                                      │               │
└─────────────────────────────────────────────────────────────────┘

Для КАЖДОЙ батареи (1–10):

Батарея (+) ───[10k]───┬──[10k]─── GND
                       │
                       └───[100nF]─── GND
                       │
                       └─── A0 (или A1/A2/A3) ADS1115

Коэффициент делителя: 2.0 (напряжение на ADC = ½ от батареи)
Распределение каналов ADS1115
Таблица
Модуль	Адрес	Канал	Батарея
ADS1115 #1	0x48	A0	Батарея 1
ADS1115 #1	0x48	A1	Батарея 2
ADS1115 #1	0x48	A2	Батарея 3
ADS1115 #1	0x48	A3	Батарея 4
ADS1115 #2	0x49	A0	Батарея 5
ADS1115 #2	0x49	A1	Батарея 6
ADS1115 #2	0x49	A2	Батарея 7
ADS1115 #2	0x49	A3	Батарея 8
ADS1115 #3	0x4A	A0	Батарея 9
ADS1115 #3	0x4A	A1	Батарея 10

⚙️ Полный код ESPHome
Сохраните как battery_monitor.yaml:
```yaml
# ============================================================
# SMART BATTERY MONITOR — ESPHome
# Поддержка от 1 до 10 батарей Li-ion 18650
# ============================================================

# --- Настройки устройства ---
esphome:
  name: "smart-battery-monitor"        # Имя устройства в сети
  friendly_name: "Smart Battery Monitor" # Отображаемое имя
  comment: "Мониторинг 1-10 батарей через ADS1115"

# --- Платформа ESP32 ---
esp32:
  board: esp32dev                      # Модель платы: ESP32 DevKit V1
  framework:
    type: arduino                      # Используем Arduino-фреймворк

# --- Логирование ---
logger:
  level: INFO                          # Уровень логов: INFO (можно DEBUG для отладки)

# --- API для подключения к Home Assistant ---
api:
  encryption:
    key: !secret api_encryption_key    # Ключ шифрования (хранится в secrets.yaml)

# --- OTA обновления ---
ota:
  - platform: esphome
    password: !secret ota_password     # Пароль для обновления по воздуху

# --- Wi-Fi ---
wifi:
  ssid: !secret wifi_ssid              # SSID сети
  password: !secret wifi_password      # Пароль Wi-Fi
  ap:                                  # Режим точки доступа при отсутствии Wi-Fi
    ssid: "BatteryMonitor Fallback"
    password: !secret fallback_password

# --- Веб-сервер (локальный дашборд) ---
web_server:
  port: 80                             # Порт веб-интерфейса
  version: 2                           # Версия 2 (современный UI)

# --- I2C шина для связи с ADS1115 ---
i2c:
  sda: GPIO21                          # Пин данных I2C
  scl: GPIO22                          # Пин тактирования I2C
  scan: true                           # Автоматически сканировать устройства на шине
  frequency: 400kHz                    # Частота шины: 400 кГц (быстрый режим)

# --- 3 модуля ADS1115 с разными I2C-адресами ---
ads1115:
  - address: 0x48                      # Адрес первого модуля (ADDR → GND)
    id: ads1115_1                      # ID для ссылок в сенсорах
  - address: 0x49                      # Адрес второго модуля (ADDR → VCC)
    id: ads1115_2
  - address: 0x4A                      # Адрес третьего модуля (ADDR → SDA)
    id: ads1115_3

# ============================================================
# СЕНСОРЫ НАПРЯЖЕНИЯ (10 батарей)
# ============================================================
sensor:
  # --- БАТАРЕЯ 1 ---
  - platform: ads1115                  # Используем внешний ADC ADS1115
    ads1115_id: ads1115_1              # Привязка к модулю #1
    multiplexer: A0_GND                # Канал A0 относительно GND
    gain: 2.048                        # Диапазон измерения: ±2.048V
    name: "Battery 1 Voltage"          # Имя сенсора в Home Assistant
    id: bat1_voltage                   # Внутренний ID для ссылок
    unit_of_measurement: "V"           # Единица измерения: Вольты
    accuracy_decimals: 3               # Точность: 3 знака после запятой
    update_interval: 10s               # Интервал обновления: 10 секунд
    filters:
      - multiply: 2.0                 # Коэффициент делителя (10k+10k = 1:2)
      - throttle: 5s                   # Ограничение частоты отправки данных

  # --- БАТАРЕЯ 2 ---
  - platform: ads1115
    ads1115_id: ads1115_1
    multiplexer: A1_GND                # Канал A1
    gain: 2.048
    name: "Battery 2 Voltage"
    id: bat2_voltage
    unit_of_measurement: "V"
    accuracy_decimals: 3
    update_interval: 10s
    filters:
      - multiply: 2.0
      - throttle: 5s

  # --- БАТАРЕЯ 3 ---
  - platform: ads1115
    ads1115_id: ads1115_1
    multiplexer: A2_GND                # Канал A2
    gain: 2.048
    name: "Battery 3 Voltage"
    id: bat3_voltage
    unit_of_measurement: "V"
    accuracy_decimals: 3
    update_interval: 10s
    filters:
      - multiply: 2.0
      - throttle: 5s

  # --- БАТАРЕЯ 4 ---
  - platform: ads1115
    ads1115_id: ads1115_1
    multiplexer: A3_GND                # Канал A3
    gain: 2.048
    name: "Battery 4 Voltage"
    id: bat4_voltage
    unit_of_measurement: "V"
    accuracy_decimals: 3
    update_interval: 10s
    filters:
      - multiply: 2.0
      - throttle: 5s

  # --- БАТАРЕЯ 5 ---
  - platform: ads1115
    ads1115_id: ads1115_2              # Модуль #2
    multiplexer: A0_GND
    gain: 2.048
    name: "Battery 5 Voltage"
    id: bat5_voltage
    unit_of_measurement: "V"
    accuracy_decimals: 3
    update_interval: 10s
    filters:
      - multiply: 2.0
      - throttle: 5s

  # --- БАТАРЕЯ 6 ---
  - platform: ads1115
    ads1115_id: ads1115_2
    multiplexer: A1_GND
    gain: 2.048
    name: "Battery 6 Voltage"
    id: bat6_voltage
    unit_of_measurement: "V"
    accuracy_decimals: 3
    update_interval: 10s
    filters:
      - multiply: 2.0
      - throttle: 5s

  # --- БАТАРЕЯ 7 ---
  - platform: ads1115
    ads1115_id: ads1115_2
    multiplexer: A2_GND
    gain: 2.048
    name: "Battery 7 Voltage"
    id: bat7_voltage
    unit_of_measurement: "V"
    accuracy_decimals: 3
    update_interval: 10s
    filters:
      - multiply: 2.0
      - throttle: 5s

  # --- БАТАРЕЯ 8 ---
  - platform: ads1115
    ads1115_id: ads1115_2
    multiplexer: A3_GND
    gain: 2.048
    name: "Battery 8 Voltage"
    id: bat8_voltage
    unit_of_measurement: "V"
    accuracy_decimals: 3
    update_interval: 10s
    filters:
      - multiply: 2.0
      - throttle: 5s

  # --- БАТАРЕЯ 9 ---
  - platform: ads1115
    ads1115_id: ads1115_3              # Модуль #3
    multiplexer: A0_GND
    gain: 2.048
    name: "Battery 9 Voltage"
    id: bat9_voltage
    unit_of_measurement: "V"
    accuracy_decimals: 3
    update_interval: 10s
    filters:
      - multiply: 2.0
      - throttle: 5s

  # --- БАТАРЕЯ 10 ---
  - platform: ads1115
    ads1115_id: ads1115_3
    multiplexer: A1_GND
    gain: 2.048
    name: "Battery 10 Voltage"
    id: bat10_voltage
    unit_of_measurement: "V"
    accuracy_decimals: 3
    update_interval: 10s
    filters:
      - multiply: 2.0
      - throttle: 5s

  # ============================================================
  # ПРОЦЕНТ ЗАРЯДА (Template сенсоры)
  # Формула для Li-ion: 4.2V = 100%, 3.0V = 0%
  # ============================================================
  - platform: template
    name: "Battery 1 Percentage"
    id: bat1_percent
    unit_of_measurement: "%"
    accuracy_decimals: 1
    update_interval: 10s
    lambda: |-
      float v = id(bat1_voltage).state;  // Получаем напряжение батареи 1
      if (std::isnan(v)) return NAN;      // Если данных нет — возвращаем NaN
      if (v >= 4.2) return 100.0;         // Полный заряд
      if (v <= 3.0) return 0.0;           // Полный разряд (минимум)
      return (v - 3.0) / (4.2 - 3.0) * 100.0;  // Линейная интерполяция

  - platform: template
    name: "Battery 2 Percentage"
    id: bat2_percent
    unit_of_measurement: "%"
    accuracy_decimals: 1
    update_interval: 10s
    lambda: |-
      float v = id(bat2_voltage).state;
      if (std::isnan(v)) return NAN;
      if (v >= 4.2) return 100.0;
      if (v <= 3.0) return 0.0;
      return (v - 3.0) / (4.2 - 3.0) * 100.0;

  - platform: template
    name: "Battery 3 Percentage"
    id: bat3_percent
    unit_of_measurement: "%"
    accuracy_decimals: 1
    update_interval: 10s
    lambda: |-
      float v = id(bat3_voltage).state;
      if (std::isnan(v)) return NAN;
      if (v >= 4.2) return 100.0;
      if (v <= 3.0) return 0.0;
      return (v - 3.0) / (4.2 - 3.0) * 100.0;

  - platform: template
    name: "Battery 4 Percentage"
    id: bat4_percent
    unit_of_measurement: "%"
    accuracy_decimals: 1
    update_interval: 10s
    lambda: |-
      float v = id(bat4_voltage).state;
      if (std::isnan(v)) return NAN;
      if (v >= 4.2) return 100.0;
      if (v <= 3.0) return 0.0;
      return (v - 3.0) / (4.2 - 3.0) * 100.0;

  - platform: template
    name: "Battery 5 Percentage"
    id: bat5_percent
    unit_of_measurement: "%"
    accuracy_decimals: 1
    update_interval: 10s
    lambda: |-
      float v = id(bat5_voltage).state;
      if (std::isnan(v)) return NAN;
      if (v >= 4.2) return 100.0;
      if (v <= 3.0) return 0.0;
      return (v - 3.0) / (4.2 - 3.0) * 100.0;

  - platform: template
    name: "Battery 6 Percentage"
    id: bat6_percent
    unit_of_measurement: "%"
    accuracy_decimals: 1
    update_interval: 10s
    lambda: |-
      float v = id(bat6_voltage).state;
      if (std::isnan(v)) return NAN;
      if (v >= 4.2) return 100.0;
      if (v <= 3.0) return 0.0;
      return (v - 3.0) / (4.2 - 3.0) * 100.0;

  - platform: template
    name: "Battery 7 Percentage"
    id: bat7_percent
    unit_of_measurement: "%"
    accuracy_decimals: 1
    update_interval: 10s
    lambda: |-
      float v = id(bat7_voltage).state;
      if (std::isnan(v)) return NAN;
      if (v >= 4.2) return 100.0;
      if (v <= 3.0) return 0.0;
      return (v - 3.0) / (4.2 - 3.0) * 100.0;

  - platform: template
    name: "Battery 8 Percentage"
    id: bat8_percent
    unit_of_measurement: "%"
    accuracy_decimals: 1
    update_interval: 10s
    lambda: |-
      float v = id(bat8_voltage).state;
      if (std::isnan(v)) return NAN;
      if (v >= 4.2) return 100.0;
      if (v <= 3.0) return 0.0;
      return (v - 3.0) / (4.2 - 3.0) * 100.0;

  - platform: template
    name: "Battery 9 Percentage"
    id: bat9_percent
    unit_of_measurement: "%"
    accuracy_decimals: 1
    update_interval: 10s
    lambda: |-
      float v = id(bat9_voltage).state;
      if (std::isnan(v)) return NAN;
      if (v >= 4.2) return 100.0;
      if (v <= 3.0) return 0.0;
      return (v - 3.0) / (4.2 - 3.0) * 100.0;

  - platform: template
    name: "Battery 10 Percentage"
    id: bat10_percent
    unit_of_measurement: "%"
    accuracy_decimals: 1
    update_interval: 10s
    lambda: |-
      float v = id(bat10_voltage).state;
      if (std::isnan(v)) return NAN;
      if (v >= 4.2) return 100.0;
      if (v <= 3.0) return 0.0;
      return (v - 3.0) / (4.2 - 3.0) * 100.0;

  # --- Среднее напряжение всех батарей ---
  - platform: template
    name: "Average Battery Voltage"
    id: avg_voltage
    unit_of_measurement: "V"
    accuracy_decimals: 3
    update_interval: 10s
    lambda: |-
      float sum = 0.0;
      int count = 0;
      // Проверяем каждую батарею. Если напряжение > 0 — учитываем
      if (!std::isnan(id(bat1_voltage).state) && id(bat1_voltage).state > 0) { sum += id(bat1_voltage).state; count++; }
      if (!std::isnan(id(bat2_voltage).state) && id(bat2_voltage).state > 0) { sum += id(bat2_voltage).state; count++; }
      if (!std::isnan(id(bat3_voltage).state) && id(bat3_voltage).state > 0) { sum += id(bat3_voltage).state; count++; }
      if (!std::isnan(id(bat4_voltage).state) && id(bat4_voltage).state > 0) { sum += id(bat4_voltage).state; count++; }
      if (!std::isnan(id(bat5_voltage).state) && id(bat5_voltage).state > 0) { sum += id(bat5_voltage).state; count++; }
      if (!std::isnan(id(bat6_voltage).state) && id(bat6_voltage).state > 0) { sum += id(bat6_voltage).state; count++; }
      if (!std::isnan(id(bat7_voltage).state) && id(bat7_voltage).state > 0) { sum += id(bat7_voltage).state; count++; }
      if (!std::isnan(id(bat8_voltage).state) && id(bat8_voltage).state > 0) { sum += id(bat8_voltage).state; count++; }
      if (!std::isnan(id(bat9_voltage).state) && id(bat9_voltage).state > 0) { sum += id(bat9_voltage).state; count++; }
      if (!std::isnan(id(bat10_voltage).state) && id(bat10_voltage).state > 0) { sum += id(bat10_voltage).state; count++; }
      if (count == 0) return NAN;
      return sum / count;

  # --- Минимальное напряжение (для определения слабейшей батареи) ---
  - platform: template
    name: "Minimum Battery Voltage"
    id: min_voltage
    unit_of_measurement: "V"
    accuracy_decimals: 3
    update_interval: 10s
    lambda: |-
      float min_v = 999.0;
      if (!std::isnan(id(bat1_voltage).state) && id(bat1_voltage).state > 0 && id(bat1_voltage).state < min_v) min_v = id(bat1_voltage).state;
      if (!std::isnan(id(bat2_voltage).state) && id(bat2_voltage).state > 0 && id(bat2_voltage).state < min_v) min_v = id(bat2_voltage).state;
      if (!std::isnan(id(bat3_voltage).state) && id(bat3_voltage).state > 0 && id(bat3_voltage).state < min_v) min_v = id(bat3_voltage).state;
      if (!std::isnan(id(bat4_voltage).state) && id(bat4_voltage).state > 0 && id(bat4_voltage).state < min_v) min_v = id(bat4_voltage).state;
      if (!std::isnan(id(bat5_voltage).state) && id(bat5_voltage).state > 0 && id(bat5_voltage).state < min_v) min_v = id(bat5_voltage).state;
      if (!std::isnan(id(bat6_voltage).state) && id(bat6_voltage).state > 0 && id(bat6_voltage).state < min_v) min_v = id(bat6_voltage).state;
      if (!std::isnan(id(bat7_voltage).state) && id(bat7_voltage).state > 0 && id(bat7_voltage).state < min_v) min_v = id(bat7_voltage).state;
      if (!std::isnan(id(bat8_voltage).state) && id(bat8_voltage).state > 0 && id(bat8_voltage).state < min_v) min_v = id(bat8_voltage).state;
      if (!std::isnan(id(bat9_voltage).state) && id(bat9_voltage).state > 0 && id(bat9_voltage).state < min_v) min_v = id(bat9_voltage).state;
      if (!std::isnan(id(bat10_voltage).state) && id(bat10_voltage).state > 0 && id(bat10_voltage).state < min_v) min_v = id(bat10_voltage).state;
      if (min_v == 999.0) return NAN;
      return min_v;

  # --- Максимальное напряжение ---
  - platform: template
    name: "Maximum Battery Voltage"
    id: max_voltage
    unit_of_measurement: "V"
    accuracy_decimals: 3
    update_interval: 10s
    lambda: |-
      float max_v = 0.0;
      if (!std::isnan(id(bat1_voltage).state) && id(bat1_voltage).state > max_v) max_v = id(bat1_voltage).state;
      if (!std::isnan(id(bat2_voltage).state) && id(bat2_voltage).state > max_v) max_v = id(bat2_voltage).state;
      if (!std::isnan(id(bat3_voltage).state) && id(bat3_voltage).state > max_v) max_v = id(bat3_voltage).state;
      if (!std::isnan(id(bat4_voltage).state) && id(bat4_voltage).state > max_v) max_v = id(bat4_voltage).state;
      if (!std::isnan(id(bat5_voltage).state) && id(bat5_voltage).state > max_v) max_v = id(bat5_voltage).state;
      if (!std::isnan(id(bat6_voltage).state) && id(bat6_voltage).state > max_v) max_v = id(bat6_voltage).state;
      if (!std::isnan(id(bat7_voltage).state) && id(bat7_voltage).state > max_v) max_v = id(bat7_voltage).state;
      if (!std::isnan(id(bat8_voltage).state) && id(bat8_voltage).state > max_v) max_v = id(bat8_voltage).state;
      if (!std::isnan(id(bat9_voltage).state) && id(bat9_voltage).state > max_v) max_v = id(bat9_voltage).state;
      if (!std::isnan(id(bat10_voltage).state) && id(bat10_voltage).state > max_v) max_v = id(bat10_voltage).state;
      if (max_v == 0.0) return NAN;
      return max_v;

  # --- Дельта (разброс) между макс и мин ---
  - platform: template
    name: "Battery Voltage Delta"
    id: voltage_delta
    unit_of_measurement: "V"
    accuracy_decimals: 3
    update_interval: 10s
    lambda: |-
      float max_v = id(max_voltage).state;
      float min_v = id(min_voltage).state;
      if (std::isnan(max_v) || std::isnan(min_v)) return NAN;
      return max_v - min_v;

  # --- WiFi сигнал ---
  - platform: wifi_signal
    name: "WiFi Signal"
    update_interval: 60s
    unit_of_measurement: "dBm"
    accuracy_decimals: 0

# ============================================================
# БИНАРНЫЕ СЕНСОРЫ (Алерты)
# ============================================================
binary_sensor:
  # --- Низкий заряд Батареи 1 ---
  - platform: template
    name: "Battery 1 Low Alert"
    id: bat1_low
    device_class: battery           # Класс устройства: батарея
    lambda: |-
      if (std::isnan(id(bat1_voltage).state)) return false;
      return id(bat1_voltage).state < 3.2;  // Тревога если < 3.2V

  - platform: template
    name: "Battery 2 Low Alert"
    id: bat2_low
    device_class: battery
    lambda: |-
      if (std::isnan(id(bat2_voltage).state)) return false;
      return id(bat2_voltage).state < 3.2;

  - platform: template
    name: "Battery 3 Low Alert"
    id: bat3_low
    device_class: battery
    lambda: |-
      if (std::isnan(id(bat3_voltage).state)) return false;
      return id(bat3_voltage).state < 3.2;

  - platform: template
    name: "Battery 4 Low Alert"
    id: bat4_low
    device_class: battery
    lambda: |-
      if (std::isnan(id(bat4_voltage).state)) return false;
      return id(bat4_voltage).state < 3.2;

  - platform: template
    name: "Battery 5 Low Alert"
    id: bat5_low
    device_class: battery
    lambda: |-
      if (std::isnan(id(bat5_voltage).state)) return false;
      return id(bat5_voltage).state < 3.2;

  - platform: template
    name: "Battery 6 Low Alert"
    id: bat6_low
    device_class: battery
    lambda: |-
      if (std::isnan(id(bat6_voltage).state)) return false;
      return id(bat6_voltage).state < 3.2;

  - platform: template
    name: "Battery 7 Low Alert"
    id: bat7_low
    device_class: battery
    lambda: |-
      if (std::isnan(id(bat7_voltage).state)) return false;
      return id(bat7_voltage).state < 3.2;

  - platform: template
    name: "Battery 8 Low Alert"
    id: bat8_low
    device_class: battery
    lambda: |-
      if (std::isnan(id(bat8_voltage).state)) return false;
      return id(bat8_voltage).state < 3.2;

  - platform: template
    name: "Battery 9 Low Alert"
    id: bat9_low
    device_class: battery
    lambda: |-
      if (std::isnan(id(bat9_voltage).state)) return false;
      return id(bat9_voltage).state < 3.2;

  - platform: template
    name: "Battery 10 Low Alert"
    id: bat10_low
    device_class: battery
    lambda: |-
      if (std::isnan(id(bat10_voltage).state)) return false;
      return id(bat10_voltage).state < 3.2;

  # --- Общий алерт: любая батарея разряжена ---
  - platform: template
    name: "Any Battery Low Alert"
    id: any_low
    device_class: battery
    lambda: |-
      return id(bat1_low).state || id(bat2_low).state || id(bat3_low).state ||
             id(bat4_low).state || id(bat5_low).state || id(bat6_low).state ||
             id(bat7_low).state || id(bat8_low).state || id(bat9_low).state ||
             id(bat10_low).state;

# ============================================================
# ТЕКСТОВЫЙ СЕНСОР (статус системы)
# ============================================================
text_sensor:
  - platform: template
    name: "System Status"
    id: system_status
    update_interval: 10s
    lambda: |-
      int low_count = 0;
      if (id(bat1_low).state) low_count++;
      if (id(bat2_low).state) low_count++;
      if (id(bat3_low).state) low_count++;
      if (id(bat4_low).state) low_count++;
      if (id(bat5_low).state) low_count++;
      if (id(bat6_low).state) low_count++;
      if (id(bat7_low).state) low_count++;
      if (id(bat8_low).state) low_count++;
      if (id(bat9_low).state) low_count++;
      if (id(bat10_low).state) low_count++;
      
      if (low_count == 0) return std::string("OK");
      if (low_count == 1) return std::string("1 battery low");
      return std::to_string(low_count) + " batteries low";

# ============================================================
# КНОПКИ (для ручного обновления или тестов)
# ============================================================
button:
  - platform: restart
    name: "Restart Device"
    id: restart_btn
    device_class: restart
```

📤 Как выложить на GitHub и подключить к ESPHome
Шаг 1: Создайте репозиторий на GitHub
Зайдите на github.com → New Repository
Назовите, например: esphome-battery-monitor
Создайте файл battery_monitor.yaml и вставьте код выше
Сохраните (Commit)
Шаг 2: Получите прямую ссылку (Raw URL)
Откройте файл на GitHub → нажмите Raw → скопируйте URL. Будет так:
plain
https://raw.githubusercontent.com/ВАШ_НИК/esphome-battery-monitor/main/battery_monitor.yaml
Шаг 3: Импортируйте в вашу прошивку ESPHome
Создайте основной файл (например, esp32-battery.yaml):
```yaml
# Основной файл прошивки
esphome:
  name: "my-battery-station"
  friendly_name: "Battery Station"

esp32:
  board: esp32dev
  framework:
    type: arduino

# --- Импорт внешней конфигурации с GitHub ---
packages:
  battery_monitor:
    url: https://github.com/ВАШ_НИК/esphome-battery-monitor
    file: battery_monitor.yaml
    ref: main  # ветка

# --- Локальные настройки (Wi-Fi, API и т.д.) ---
wifi:
  ssid: !secret wifi_ssid
  password: !secret wifi_password

api:
  encryption:
    key: !secret api_key

ota:
  - platform: esphome
    password: !secret ota_password
```
Важно: В battery_monitor.yaml уберите блоки wifi, api, ota, logger, esphome, esp32 — они должны быть только в основном файле. Оставьте только i2c, ads1115, sensor, binary_sensor, text_sensor, button.
Структура репозитория на GitHub
plain
esphome-battery-monitor/
├── battery_monitor.yaml      # Основная логика (сенсоры)
├── README.md                 # Описание проекта
└── docs/
    └── schematic.png         # Схема подключения
🧠 Принцип работы
1. Измерение напряжения
Каждая батарея подключена к входу ADS1115 через делитель напряжения (два резистора по 10 кОм). Это нужно для:
Защиты ADC: ADS1115 работает от 3.3V, а батарея может давать до 4.2V. Делитель снижает напряжение в 2 раза.
Точности: ADS1115 — 16-битный ADC, он измеряет с шагом ~0.0625 мВ в режиме gain=2.048. После умножения на 2 (коэффициент делителя) получаем точность ~0.125 мВ на батарее.
2. Многоканальность
3 модуля ADS1115 подключены к одной I2C-шине с разными адресами:
0x48 — ADDR подключен к GND
0x49 — ADDR подключен к VCC
0x4A — ADDR подключен к SDA
Каждый модуль имеет 4 канала (A0–A3), итого 12 каналов — достаточно для 10 батарей + 2 резервных.
3. Расчёт процента заряда
Для Li-ion 18650 используется линейная аппроксимация:
4.2V = 100% (полный заряд)
3.0V = 0% (критический разряд, отключение)
Формула: (V - 3.0) / 1.2 * 100
Примечание: Реальная кривая разряда Li-ion нелинейная, но для простого мониторинга линейная модели достаточно. Для точности можно добавить lookup-table.
4. Агрегированные метрики
Система считает:
Среднее напряжение — общее состояние батарейного парка
Мин/Макс — поиск слабейшей/сильнейшей батареи
Дельта — разброс напряжений. Если дельта > 0.2V — батареи требуют балансировки
5. Алерты
Binary sensors срабатывают при падении напряжения ниже 3.2V (порог глубокого разряда). В Home Assistant можно настроить автоматизации:
Уведомление в Telegram
Включение зуммера
Отключение нагрузки через реле
6. Локальный дашборд
Веб-сервер ESPHome (web_server:) открывает страницу по IP-адресу ESP32. Там отображаются все напряжения, проценты и статус в реальном времени — аналогично видео-проекту.
🔧 Калибровка
После сборки измерьте реальное напряжение мультиметром и отредактируйте multiply в фильтрах. Если делитель не идеален:
```yaml
filters:
  - calibrate_linear:
      - 2.10 -> 4.15   # ADC показал 2.10V, мультиметр показал 4.15V
      - 1.50 -> 3.00   # ADC показал 1.50V, мультиметр показал 3.00V
```      
⚡ Расширение: управление зарядкой
Если хотите интегрировать TP4056 (как в оригинальном видео) для каждой ячейки:
Подключите CHRG пин TP4056 к GPIO ESP32 через оптрон (для гальванической развязки)
Добавьте binary_sensor типа gpio для индикации "Заряжается / Готово"
Для 10S-пакета используйте BMS 10S с UART/RS485 — тогда можно считывать данные напрямую через uart компонент ESPHome.
Если нужно, могу адаптировать код под последовательное соединение батарей (10S) с дифференциальными измерениями или добавить OLED-экран для локального отображения!
