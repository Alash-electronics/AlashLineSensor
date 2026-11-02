// [ru] AlashLineSensor: линейка QTR-RC (или любые датчики с конденсатором): подключаются к ЦИФРОВЫМ пинам, значение это время разряда в микросекундах
//      (тёмное разряжается дольше). Работает на Uno, ESP32 и ESP8266, аналоговые входы не нужны. Подсветка (вывод LEDON) необязательна.
//      Подключение: VCC 5 В (ESP: 3.3 В), GND, выходы датчиков на цифровые пины 2, 3, 4, 5, 6 (ESP8266: D1, D2, D5, D6, D7), LEDON на пин 7.
// [kk] AlashLineSensor: QTR-RC линейкасы (немесе конденсаторы бар кез келген датчик): САНДЫҚ пиндерге қосылады, мән микросекундпен разряд уақыты
//      (қараңғы ұзағырақ разрядталады). Uno, ESP32 және ESP8266-да жұмыс істейді, аналогтық кірістер керек емес. Жарықтандыру (LEDON шықпасы) міндетті емес.
//      Қосу: VCC 5 В (ESP: 3.3 В), GND, датчик шығыстары 2, 3, 4, 5, 6 сандық пиндерге (ESP8266: D1, D2, D5, D6, D7), LEDON 7 пинге.
// [en] AlashLineSensor: a QTR-RC bar (or any sensors with a capacitor): connected to DIGITAL pins, the value is the discharge time in microseconds
//      (dark discharges longer). Works on Uno, ESP32 and ESP8266, no analog inputs needed. The emitter (the LEDON pin) is optional.
//      Wiring: VCC 5 V (ESP: 3.3 V), GND, sensor outputs to digital pins 2, 3, 4, 5, 6 (ESP8266: D1, D2, D5, D6, D7), LEDON to pin 7.
#include <AlashLineSensor.h>

#if defined(ARDUINO_ARCH_ESP8266)
const uint8_t PINS[5] = {5, 4, 14, 12, 13};      // D1, D2, D5, D6, D7
const uint8_t EMITTER = 0;                       // D3
#elif defined(ARDUINO_ARCH_ESP32)
const uint8_t PINS[5] = {16, 17, 18, 19, 21};
const uint8_t EMITTER = 22;
#else
const uint8_t PINS[5] = {2, 3, 4, 5, 6};
const uint8_t EMITTER = 7;
#endif

AlashLineSensor line(PINS, 5);

void setup() {
  Serial.begin(115200);
  line.setSensorType(AlashLineSensor::SENSOR_RC);       // значение = время разряда конденсатора
  line.setRcTimeout(2500);                       // дольше 2.5 мс считаем «совсем чёрное»
  line.setEmitterPin(EMITTER);                   // подсветка включается только на время чтения (уберите, если вывода LEDON нет)
  if (!line.begin()) { Serial.println(line.errorText()); return; }
  Serial.println(F("Calibration: sweep the bar over the line and the floor..."));
  line.calibrate(3000);
}

void loop() {
  uint16_t pos = line.position();
  if (line.lineLost()) Serial.println(F("line lost"));
  else { Serial.print(F("position: ")); Serial.println(pos); }
  delay(100);
}
