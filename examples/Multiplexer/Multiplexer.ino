// [ru] AlashLineSensor: своя функция чтения. Датчики подключены не напрямую, а через мультиплексор CD4051 (8 каналов на один аналоговый вход)
//      или любым другим способом (АЦП ADS1115 по I2C, расширитель пинов): вы пишете функцию, которая возвращает значение датчика с номером i,
//      а библиотека делает остальное (калибровка, положение линии). Подключение CD4051: адресные входы A, B, C на пины 2, 3, 4, выход COM на A0.
// [kk] AlashLineSensor: өз оқу функциясы. Датчиктер тікелей емес, CD4051 мультиплексоры (бір аналогтық кіріске 8 арна) арқылы немесе кез келген басқа тәсілмен
//      (I2C арқылы ADS1115 АЦП, пин кеңейткіш) қосылған: сіз i нөмірлі датчик мәнін қайтаратын функция жазасыз, ал кітапхана қалғанын істейді (калибрлеу, сызық орны).
//      CD4051 қосу: A, B, C адрес кірістері 2, 3, 4 пиндерге, COM шығысы A0-ге.
// [en] AlashLineSensor: your own read function. The sensors are not wired directly but through a CD4051 multiplexer (8 channels onto one analog input)
//      or any other way (an ADS1115 ADC over I2C, a pin expander): you write a function that returns the value of sensor i,
//      and the library does the rest (calibration, line position). CD4051 wiring: the address inputs A, B, C to pins 2, 3, 4, the COM output to A0.
#include <AlashLineSensor.h>

#if defined(ARDUINO_ARCH_ESP8266) || defined(ARDUINO_ARCH_ESP32)
const uint8_t ADDR[3] = {16, 17, 18};
const uint8_t ANALOG_IN = 35;
#else
const uint8_t ADDR[3] = {2, 3, 4};
const uint8_t ANALOG_IN = A0;
#endif

// Вернуть сырое значение датчика с номером i (0..7): выбрать канал мультиплексора и прочитать вход
uint16_t readSensor(uint8_t i) {
  for (uint8_t b = 0; b < 3; b++) digitalWrite(ADDR[b], (i >> b) & 1);
  delayMicroseconds(20);                         // дать сигналу установиться
  return analogRead(ANALOG_IN);
}

AlashLineSensor line(8);                         // 8 датчиков, пины не нужны

void setup() {
  Serial.begin(115200);
  for (uint8_t b = 0; b < 3; b++) pinMode(ADDR[b], OUTPUT);
  line.setReader(readSensor);
#if defined(ARDUINO_ARCH_ESP32)
  line.setAdcMax(4095);
#else
  line.setAdcMax(1023);                          // максимум вашей функции чтения
#endif
  if (!line.begin()) { Serial.println(line.errorText()); return; }
  Serial.println(F("Calibration: sweep the bar over the line and the floor..."));
  line.calibrate(3000);
}

void loop() {
  uint16_t pos = line.position();                // 0 .. 7000
  if (line.lineLost()) Serial.println(F("line lost")); else Serial.println(pos);
  delay(100);
}
