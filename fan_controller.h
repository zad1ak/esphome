// =============================================
// fan_controller.h - Управление вентилятором Lummy LU-FN105
// Для ESPHome и Wemos D1 Mini
// Репозиторий: https://github.com/zad1ak/esphome
// =============================================

#ifndef FAN_CONTROLLER_H
#define FAN_CONTROLLER_H

#include "esphome.h"

// =============================================
// КЛАСС УПРАВЛЕНИЯ ВЕНТИЛЯТОРОМ
// =============================================
class FanController : public Component {
  public:
    // ---------- НАСТРОЙКА ПИНОВ ПРИ ЗАГРУЗКЕ ----------
    void setup() override {
      // Настройка пинов для светодиодов (LED)
      pinMode(D1, OUTPUT); // Mode LED - индикация режима
      pinMode(D2, OUTPUT); // Timer LED - индикация таймера
      pinMode(D3, OUTPUT); // Speed LED - индикация скорости

      // Настройка пинов управления двигателем
      pinMode(D4, OUTPUT); // IN1 - управление направлением
      pinMode(D5, OUTPUT); // IN2 - управление направлением
      pinMode(D6, OUTPUT); // PWM (EN) - управление скоростью

      // Настройка пинов кнопок как входов с подтяжкой к питанию
      pinMode(D7, INPUT_PULLUP); // Mode button - переключение режимов
      pinMode(D8, INPUT_PULLUP); // Timer button - настройка таймера
      pinMode(D0, INPUT_PULLUP); // On/Speed button - включение/скорость
      pinMode(A0, INPUT_PULLUP); // Off button - выключение

      // Инициализация двигателя (выключен)
      digitalWrite(D4, LOW);
      digitalWrite(D5, LOW);
      digitalWrite(D6, LOW);      // PWM = 0 (выключен)

      // Инициализация переменных состояния
      current_speed = SPEED_OFF;   // Начинаем с выключенного состояния
      current_mode = MODE_NORMAL;  // Режим по умолчанию - обычный
      timer_minutes = 0;           // Таймер выключен
      last_button_time = 0;        // Время последнего нажатия кнопки
      last_breeze_change = 0;      // Время последнего изменения в режиме BREEZE
      breeze_slow = false;         // Флаг "медленного" ветра
      breeze_counter = 0;          // Счетчик циклов BREEZE
      timer_end = 0;               // Время окончания таймера
      timer_active = false;        // Активен ли таймер

      // Настройка аппаратного таймера для прерываний
      timer1_attachInterrupt(timerCallback);  // Привязываем функцию
      timer1_enable(TIM_DIV16, TIM_EDGE, TIM_LOOP); // Режим таймера
      timer1_write(500000);                   // Интервал 0.5 секунды
    }

    // ---------- ОСНОВНОЙ ЦИКЛ РАБОТЫ ----------
    void loop() override {
      // Обработка кнопок с антидребезгом (задержка 200 мс)
      if (millis() - last_button_time > 200) {
        // Проверяем состояния кнопок (LOW = нажата)
        if (digitalRead(D7) == LOW) handleModeButton();    // Режим
        if (digitalRead(D8) == LOW) handleTimerButton();   // Таймер
        if (digitalRead(D0) == LOW) handleSpeedButton();   // Вкл/Скорость
        if (digitalRead(A0) == LOW) handleOffButton();     // Выкл
      }

      // Обновление состояния вентилятора
      updateFan();
    }

  private:
    // ---------- ПЕРЕЧИСЛЕНИЯ (ENUM) ----------
    // Скорости вентилятора
    enum Speed { SPEED_OFF, SPEED_LOW, SPEED_MEDIUM, SPEED_HIGH };
    // Режимы работы
    enum Mode { MODE_NORMAL, MODE_BREEZE, MODE_OFF };

    // ---------- ПЕРЕМЕННЫЕ СОСТОЯНИЯ ----------
    Speed current_speed = SPEED_OFF;          // Текущая скорость
    Mode current_mode = MODE_NORMAL;          // Текущий режим
    int timer_minutes = 0;                    // Время таймера в минутах
    unsigned long last_button_time = 0;       // Для антидребезга
    unsigned long last_breeze_change = 0;     // Время последнего изменения в режиме BREEZE
    bool breeze_slow = false;                 // Флаг "медленного" ветра
    int breeze_counter = 0;                   // Счетчик циклов BREEZE
    unsigned long timer_end = 0;              // Время окончания таймера
    bool timer_active = false;                // Активен ли таймер

    // ---------- ОБРАБОТЧИК ПРЕРЫВАНИЙ ТАЙМЕРА ----------
    static void timerCallback() {
      // Здесь можно добавить логику для периодических задач
      // Например, проверку окончания таймера
      // Проверка происходит в основном цикле updateFan()
    }

    // ---------- ОБРАБОТЧИКИ КНОПОК ----------
    // Кнопка MODE: переключение режимов NORMAL -> BREEZE -> OFF
    void handleModeButton() {
      last_button_time = millis();
      if (current_speed == SPEED_OFF) return; // Не работаем, если выключен

      switch (current_mode) {
        case MODE_NORMAL:
          current_mode = MODE_BREEZE;  // Переключаем на режим ветра
          breeze_counter = 0;           // Сбрасываем счетчик
          break;
        case MODE_BREEZE:
          current_mode = MODE_OFF;      // Переключаем в режим OFF
          current_speed = SPEED_OFF;    // Выключаем скорость
          stopFan();                    // Останавливаем мотор
          break;
        case MODE_OFF:
          current_mode = MODE_NORMAL;   // Возвращаем в обычный режим
          current_speed = SPEED_LOW;    // Устанавливаем минимальную скорость
          break;
      }
    }

    // Кнопка TIMER: циклическое изменение времени 0.5 -> 7.5 часов
    void handleTimerButton() {
      last_button_time = millis();
      if (current_speed == SPEED_OFF) return; // Таймер только при включенном

      // Цикл таймера: 0.5, 1, 1.5, ... 7.5 часов
      if (timer_minutes == 0) {
        timer_minutes = 30;          // 0.5 часа = 30 минут
      } else if (timer_minutes < 450) { // 450 минут = 7.5 часов
        timer_minutes += 30;          // Увеличиваем на 0.5 часа
      } else {
        timer_minutes = 0;            // Сбрасываем таймер
        timer_active = false;         // Деактивируем
      }

      // Если время установлено - активируем таймер
      if (timer_minutes > 0) {
        timer_active = true;
        timer_end = millis() + (timer_minutes * 60000); // Переводим в мс
      }
    }

    // Кнопка ON/SPEED: включение и циклическое изменение скорости
    void handleSpeedButton() {
      last_button_time = millis();
      if (current_speed == SPEED_OFF) {
        // Если выключен - включаем на минимальной скорости
        current_speed = SPEED_LOW;
        current_mode = MODE_NORMAL;   // Устанавливаем обычный режим
      } else {
        // Иначе переключаем скорость по кругу: LOW -> MEDIUM -> HIGH -> LOW
        switch (current_speed) {
          case SPEED_LOW:
            current_speed = SPEED_MEDIUM;
            break;
          case SPEED_MEDIUM:
            current_speed = SPEED_HIGH;
            break;
          case SPEED_HIGH:
            current_speed = SPEED_LOW;
            break;
          default:
            break;
        }
      }
    }

    // Кнопка OFF: полное выключение вентилятора
    void handleOffButton() {
      last_button_time = millis();
      current_speed = SPEED_OFF;      // Выключаем скорость
      current_mode = MODE_OFF;        // Устанавливаем режим OFF
      stopFan();                      // Останавливаем мотор
      timer_active = false;           // Деактивируем таймер
      timer_minutes = 0;              // Сбрасываем время таймера
    }

    // ---------- ФУНКЦИИ УПРАВЛЕНИЯ МОТОРОМ ----------
    // Остановка мотора
    void stopFan() {
      analogWrite(D6, 0);            // PWM = 0
      digitalWrite(D4, LOW);         // IN1 = LOW
      digitalWrite(D5, LOW);         // IN2 = LOW
    }

    // Основная функция обновления состояния вентилятора
    void updateFan() {
      // Проверка окончания таймера
      if (timer_active && millis() > timer_end) {
        timer_active = false;         // Деактивируем таймер
        timer_minutes = 0;            // Сбрасываем время
        current_speed = SPEED_OFF;    // Выключаем скорость
        stopFan();                    // Останавливаем мотор
        return;                       // Выходим из функции
      }

      // Если вентилятор выключен - останавливаем мотор и выходим
      if (current_speed == SPEED_OFF) {
        stopFan();
        return;
      }

      // ---------- УПРАВЛЕНИЕ СКОРОСТЬЮ ----------
      int speed_value = 0; // Значение PWM (0-1023)
      switch (current_speed) {
        case SPEED_LOW:
          speed_value = 170;   // ~33% от максимума
          break;
        case SPEED_MEDIUM:
          speed_value = 340;   // ~67% от максимума
          break;
        case SPEED_HIGH:
          speed_value = 512;   // 100% (максимум)
          break;
        default:
          break;
      }

      // ---------- РЕЖИМ BREEZE (ИМИТАЦИЯ ВЕТРА) ----------
      if (current_mode == MODE_BREEZE) {
        // Изменяем состояние каждые 2 секунды
        if (millis() - last_breeze_change > 2000) {
          last_breeze_change = millis();
          breeze_counter++;

          // Каждый 6-й цикл (12 секунд) меняем режим скорости
          if (breeze_counter % 6 == 0) {
            breeze_slow = !breeze_slow; // Переключаем: быстро/медленно
          }

          // Если режим "медленный" - уменьшаем скорость на 60%
          if (breeze_slow) {
            speed_value = speed_value * 0.4; // Оставляем 40% от текущей
          }
        }
      }

      // ---------- ПРИМЕНЕНИЕ УСТАНОВОК К МОТОРУ ----------
      analogWrite(D6, speed_value);    // Устанавливаем ШИМ
      digitalWrite(D4, HIGH);          // Включаем направление 1
      digitalWrite(D5, LOW);           // Выключаем направление 2

      // ---------- ОБНОВЛЕНИЕ LED ИНДИКАТОРОВ ----------
      digitalWrite(D1, current_mode != MODE_OFF ? HIGH : LOW); // Mode LED
      digitalWrite(D2, timer_active ? HIGH : LOW);             // Timer LED
      digitalWrite(D3, current_speed != SPEED_OFF ? HIGH : LOW); // Speed LED
    }
};

#endif // FAN_CONTROLLER_H
