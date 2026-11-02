// [ru] AlashLineSensor: самый простой пример. Линейка из 5 датчиков отражения, положение линии печатается 10 раз в секунду.
//
//      Подключение линейки (QTR-5A или 5 модулей TCRT5000): VCC к 5 В, GND к GND, выходы на A0-A4 (ESP32: GPIO 36, 39, 34, 35, 32).
//      Держите линейку на высоте 3-9 мм над поверхностью. Сначала калибровка: 3 секунды водите линейку над линией и фоном.
// [kk] AlashLineSensor: ең қарапайым мысал. 5 шағылу датчигінен тұратын линейка, сызық орны секундына 10 рет басылады.
//
//      Линейканы қосу (QTR-5A немесе 5 TCRT5000 модулі): VCC 5 В-қа, GND GND-ға, шығыстар A0-A4-ке (ESP32: GPIO 36, 39, 34, 35, 32).
//      Линейканы беттен 3-9 мм биіктікте ұстаңыз. Алдымен калибрлеу: 3 секунд линейканы сызық пен фон үстінде жүргізіңіз.
// [en] AlashLineSensor: the simplest example. A bar of 5 reflectance sensors, the line position is printed 10 times a second.
//
//      Bar wiring (QTR-5A or 5 TCRT5000 modules): VCC to 5 V, GND to GND, outputs to A0-A4 (ESP32: GPIO 36, 39, 34, 35, 32).
//      Hold the bar 3-9 mm above the surface. First the calibration: for 3 seconds sweep the bar over the line and the background.
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
  if (!line.begin()) { Serial.println(line.errorText()); return; }
  Serial.println(F("Calibration: sweep the bar over the line and the floor..."));
  if (!line.calibrate(3000)) Serial.println(line.errorText());
}

void loop() {
  uint16_t pos = line.position();                // 0 .. 4000, посередине 2000
  if (line.lineLost()) Serial.println(F("line lost"));
  else { Serial.print(F("position: ")); Serial.print(pos); Serial.print(F("   offset: ")); Serial.println(line.offset()); }
  delay(100);
}
