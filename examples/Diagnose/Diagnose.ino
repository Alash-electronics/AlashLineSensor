// [ru] AlashLineSensor: линейка ведёт себя странно? Скетч покажет сырые и нормированные значения каждого датчика, калибровку и подскажет, что проверить.
//      Язык подсказок: LANG_RU, LANG_KK или LANG_EN. Подключение как в примере Basic.
// [kk] AlashLineSensor: линейка біртүрлі жұмыс істей ме? Скетч әр датчиктің шикі және нормаланған мәндерін, калибрлеуді көрсетеді және нені тексеру керегін айтады.
//      Кеңес тілі: LANG_RU, LANG_KK немесе LANG_EN. Қосу Basic мысалындағыдай.
// [en] AlashLineSensor: the bar behaves strangely? The sketch shows the raw and normalized values of every sensor, the calibration and tells what to check.
//      Tips language: LANG_RU, LANG_KK or LANG_EN. Wiring as in the Basic example.
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
#else
const uint8_t PINS[5] = {A0, A1, A2, A3, A4};
#endif

AlashLineSensor line(PINS, 5);

void setup() {
  Serial.begin(115200);
#if defined(ARDUINO_ARCH_ESP32)
  line.setAdcMax(4095);
#endif
  line.setLanguage(AlashLineSensor::LANG_RU);
  line.begin();
  line.calibrate(3000);                              // пока водите линейку над линией и фоном
  line.diagnose(Serial);
}

void loop() {}
#endif
