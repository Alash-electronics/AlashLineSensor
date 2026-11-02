// [ru] AlashLineSensor: калибровка и перенос чисел в скетч. Водите линейку над линией и фоном (кнопка не нужна, 5 секунд после включения),
//      потом скетч напечатает строки setCalibration(...). Вставьте их в setup() вместо calibrate(): калибровать при каждом запуске не придётся.
// [kk] AlashLineSensor: калибрлеу және сандарды скетчке көшіру. Линейканы сызық пен фон үстінде жүргізіңіз (түймесіз, қосқаннан кейін 5 секунд),
//      сосын скетч setCalibration(...) жолдарын басып шығарады. Оларды calibrate() орнына setup() ішіне қойыңыз: әр іске қосуда калибрлеудің қажеті жоқ.
// [en] AlashLineSensor: calibration and moving the numbers into the sketch. Sweep the bar over the line and the background (no button needed, 5 seconds after power-on),
//      then the sketch prints setCalibration(...) lines. Paste them into setup() instead of calibrate(): you will not have to calibrate on every start.
#if defined(ARDUINO_ARCH_ESP8266)
// ESP8266 has a single analog input (A0), a sensor bar needs several: use Uno, ESP32, or digital sensor modules with setDigital(true).
// У ESP8266 один аналоговый вход (A0), а линейке нужно несколько: возьмите Uno, ESP32 или цифровые модули датчиков с setDigital(true).
void setup() {
  Serial.begin(115200);
  Serial.println(F("ESP8266 has one analog input. Use Uno or ESP32, or digital sensor modules with setDigital(true)."));
}
void loop() {}
#else
#include <AlashLineSensor.h>

#if defined(ARDUINO_ARCH_ESP32)
const uint8_t PINS[5] = {36, 39, 34, 35, 32};
const uint16_t ADC_MAX = 4095;
#else
const uint8_t PINS[5] = {A0, A1, A2, A3, A4};
const uint16_t ADC_MAX = 1023;
#endif

AlashLineSensor line(PINS, 5);

void setup() {
  Serial.begin(115200);
  line.setAdcMax(ADC_MAX);
  line.begin();
  Serial.println(F("Sweep the bar over the line and the floor for 5 seconds..."));
  if (!line.calibrate(5000)) { Serial.println(line.errorText()); return; }
  for (uint8_t i = 0; i < line.count(); i++) {
    Serial.print(F("line.setCalibration(")); Serial.print(i); Serial.print(F(", "));
    Serial.print(line.calMin(i)); Serial.print(F(", ")); Serial.print(line.calMax(i)); Serial.println(F(");"));
  }
}

void loop() {}
#endif
