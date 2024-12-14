#include <TimerOne.h>

/*
  28.01.18 Модифицировано под дисплей 1602 klykov.net vk.com/ms262 instagram.com/klykovnet
  
  02.02.2019 Доработано: Oldroid123 https://vk.com/bichkov123, Integer Integral - https://vk.com/integerintegral
  НАСТРОЙКА АВТООТКЛЮЧЕНИЯ ПО ВРЕМЕНИ - 174 СТРОКА! ЧТОБЫ ОТКЛЮЧИТЬ - УДАЛЯЙТЕ 174-181 СТРОКУ!
  Щелчок по одной кнопке - смена режимов "GPU/CPU" и "GPUmem/RAM", щелчок по другой - смена режимов "FANspeed/t1,t2" и "Mother, HDD, time"
  1. Удалены режимы с графиками
  2. Вывод времени, когда нет подключения
  3. Автоотключение дисплея в период с полуночи до 7:00 (легко можно изменить)
  4. На каждую кнопку настроены два режима. То есть, кликаем по кнопке "1" - меняются режимы "ГПУ/ЦПУ" и "ГПУмем и РАМ", по кнопке 2 - аналогично два других
  5. После "Холодного" старта, когда ещё нет информации с ПК о времени, дисплей находится в "спячке", не показывает данных. Загорается при обнаружении подключения
  Для работы необходима модифицированная версия ОНМ, лежит в этой же папке
  
  Общее описание:
  Блок электроники для крутого моддинга вашего ПК, возможности:
  - Вывод основных параметров железа на внешний LCD дисплей
  - Температура: CPU, GPU, материнская плата, самый горячий HDD
  - Уровень загрузки: CPU, GPU, RAM, видеопамять
  - Температура с внешних датчиков (DS18B20)
  - Текущий уровень скорости внешних вентиляторов
  - Управление большим количеством 12 вольтовых 2, 3, 4 проводных вентиляторов
  - Автоматическое управление скоростью пропорционально температуре
  - Ручное управление скоростью из интерфейса программы
  - Управление RGB светодиодной лентой
  - Управление цветом пропорционально температуре (синий - зелёный - жёлтый - красный)
  - Ручное управление цветом из интерфейса программы
  Программа HardwareMonitorPlus  https://github.com/AlexGyver/PCdisplay
  - Запустить OpenHardwareMonitor.exe
  - Options/Serial/Run - запуск соединения с Ардуиной
  - Options/Serial/Config - настройка параметров работы
    - PORT address - адрес порта, куда подключена Ардуина
    - TEMP source - источник показаний температуры (процессор, видеокарта, максимум проц+видео, датчик 1, датчик 2)
    - FAN min, FAN max - минимальные и максимальные обороты вентиляторов, в %
    - TEMP min, TEMP max - минимальная и максимальная температура, в градусах Цельсия
    - Manual FAN - ручное управление скоростью вентилятора в %
    - Manual COLOR - ручное управление цветом ленты
    - LED brightness - управление яркостью ленты
    - CHART interval - интервал обновления графиков
   Что идёт в порт: 0-CPU temp, 1-GPU temp, 2-mother temp, 3-max HDD temp, 4-CPU load, 5-GPU load, 6-RAM use, 7-GPU memory use,
   8-maxFAN, 9-minFAN, 10-maxTEMP, 11-minTEMP, 12-manualFAN, 13-manualCOLOR, 14-fanCtrl, 15-colorCtrl, 16-brightCtrl, 17-LOGinterval, 18-tempSource, 19 - time
*/
// ------------------------ НАСТРОЙКИ ----------------------------
// настройки пределов скорости и температуры по умолчанию (на случай отсутствия связи)
// ------------------------ НАСТРОЙКИ ----------------------------

// ----------------------- ПИНЫ ---------------------------
#define SENSOR_PIN 14       // датчик температуры
int RECV_PIN = 11;          // пульт
// ----------------------- ПИНЫ ---------------------------

// -------------------- БИБЛИОТЕКИ ---------------------
#include <OneWire.h>            // библиотека протокола датчиков
#include <DallasTemperature.h>  // библиотека датчика
#include <string.h>             // библиотека расширенной работы со строками
#include <Wire.h>               // библиотека для соединения
#include <LiquidCrystal_I2C.h>  // библтотека дислея

// -------------------- БИБЛИОТЕКИ ---------------------

// -------- АВТОВЫБОР ОПРЕДЕЛЕНИЯ ДИСПЛЕЯ-------------
// Если кончается на 4Т - это 0х27. Если на 4АТ - 0х3f
#if (DRIVER_VERSION)
LiquidCrystal_I2C lcd(0x27, 16, 2);
#else
LiquidCrystal_I2C lcd(0x3f, 16, 2);
#endif
// -------- АВТОВЫБОР ОПРЕДЕЛЕНИЯ ДИСПЛЕЯ-------------

#define printByte(args)  write(args);
#define TEMPERATURE_PRECISION 9
// настройка датчиков
OneWire oneWire(SENSOR_PIN);
DallasTemperature sensors(&oneWire);
DeviceAddress Thermometer1, Thermometer2;


// значок градуса!!!! lcd.write(223);
byte degree[8] = {0b11100,  0b10100,  0b11100,  0b00000,  0b00000,  0b00000,  0b00000,  0b00000};


char inData[108];       // массив входных значений (СИМВОЛЫ)
int PCdata[26];        // массив численных значений показаний с компьютера

byte blocks, halfs;
byte index = 0;
int display_mode = 0;
String string_convert;
unsigned long timeout, blink_timer, plot_timer;
boolean lightState, reDraw_flag = 1, updateDisplay_flag, updateTemp_flag, timeOut_flag = 1;
int duty, LEDcolor;
int k, b, R, G, B, Rf, Gf, Bf;
byte mainTemp;
byte lines[] = {4, 5, 7, 6};
String perc;
unsigned long sec;
unsigned int mins, hrs;
byte temp1, temp2;
boolean btn1_sig, btn2_sig, btn1_flag, btn2_flag;


//tmElements_t tm;

void setup() {
  Serial.begin(9600);

  pinMode(R_PIN, OUTPUT);
  pinMode(G_PIN, OUTPUT);
  pinMode(B_PIN, OUTPUT);
  digitalWrite(R_PIN, 0);
  digitalWrite(G_PIN, 0);
  digitalWrite(B_PIN, 0);
  pinMode(BTN1, INPUT_PULLUP);
  pinMode(BTN2, INPUT_PULLUP);
  sensors.begin();
  sensors.getAddress(Thermometer1, 0);
  sensors.getAddress(Thermometer2, 1);
  sensors.setResolution(Thermometer1, TEMPERATURE_PRECISION);
  sensors.setResolution(Thermometer2, TEMPERATURE_PRECISION);
  // инициализация дисплея
  lcd.init();
  lcd.backlight();
  lcd.clear();            // очистить дисплей
  delay(2000);
  lcd.clear();            // очистить дисплей

  delay(2000);               // на 2 секунды
  lcd.clear();               // очистить дисплей

  PCdata[10] = tempMAX;
  PCdata[11] = tempMIN;

  //setTime(1);  // устанавливаем начальное время
}
// 8-maxFAN, 9-minFAN, 10-maxTEMP, 11-minTEMP, 12-mnlFAN

// ------------------------------ ОСНОВНОЙ ЦИКЛ -------------------------------
void loop() {
  parsing();                          // парсим строки с компьютера
  getTemperature();                   // получить значения с датчиков температуры

  updateDisplay();                    // обновить показания на дисплее
  timeoutTick();                      // проверка таймаута

}
//это надо
void getTemperature() {
  if (updateTemp_flag) {
    sensors.requestTemperatures();
    temp1 = sensors.getTempC(Thermometer1);
    temp2 = sensors.getTempC(Thermometer2);
    updateTemp_flag = 0;
  }
}

void parsing() {
  while (Serial.available() > 0) {
    char aChar = Serial.read();
    if (aChar != 'E') {
      inData[index] = aChar;
      index++;
      inData[index] = '\0';
    } else 
    {
      char *p = inData;
      char *str;
      index = 0;
      String value = "";
      while ((str = strtok_r(p, ";", &p)) != NULL) {
        string_convert = str;
        PCdata[index] = string_convert.toInt();
        index++;
      }
      index = 0; //время можно удалить
      updateDisplay_flag = 1;
      updateTemp_flag = 1;
    }
    timeout = millis();
    timeOut_flag = 1;
  }
}

void updateDisplay() {
  if (updateDisplay_flag) {
    if (reDraw_flag) {
      lcd.clear();
      draw_labels_11();
      
      reDraw_flag = 0;
    }
    draw_stats_11();

    updateDisplay_flag = 0;
  }
}

void draw_stats_11() {
  lcd.setCursor(4, 0); lcd.print(PCdata[0]); lcd.write(223);
  lcd.setCursor(4, 1); lcd.print(PCdata[1]); lcd.write(223);
}
void draw_labels_11() {//нужно не забыыть удалить нагрузку
  lcd.createChar(0, degree);
  lcd.setCursor(0, 0);
  lcd.print("CPU:");
  lcd.setCursor(0, 1);
  lcd.print("GPU:");
}

void timeoutTick() {
  if (millis() - timeout > 5000) {
    if (timeOut_flag == 1) {
      lcd.clear();
      lcd.setCursor(2, 0);
      lcd.print("DISCONNECTED");
      timeOut_flag = 0;
      reDraw_flag = 1;
    }
    char lcd_time_i[10];
    char lcd_date_i[12];
    lcd.setCursor(0, 1);
    lcd.print(lcd_time_i);
    lcd.setCursor(6, 1);
    lcd.print(lcd_date_i);
  }
}


void debug() {
  lcd.clear();
  lcd.setCursor(0, 0);
  for (int j = 0; j < 5; j++) {
    lcd.print(PCdata[j]); lcd.print("  ");
  }
  lcd.setCursor(0, 1);
  for (int j = 6; j < 10; j++) {
    lcd.print(PCdata[j]); lcd.print("  ");
  }
  lcd.setCursor(0, 2);
  for (int j = 10; j < 15; j++) {
    lcd.print(PCdata[j]); lcd.print("  ");
  }
  lcd.setCursor(0, 3);
  for (int j = 15; j < 18; j++) {
    lcd.print(PCdata[j]); lcd.print("  ");
  }
}
