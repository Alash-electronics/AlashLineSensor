// AlashLineSensor: линейка датчиков отражения (QTR, TCRT5000, модули из 3-8 датчиков) для робота-следующего-по-линии:
// калибровка, нормировка 0..1000, положение линии, потеря линии, перекрёстки, чёрная и белая линия, аналоговые и цифровые датчики.
// Без зависимостей. MIT License. Alash Electronics, https://github.com/Alash-electronics/AlashLineSensor
#ifndef ALASH_LINESENSOR_H
#define ALASH_LINESENSOR_H

#include <Arduino.h>

class AlashLineSensor {
 public:
  enum Error : uint8_t {
    ERR_OK = 0,
    ERR_BAD_COUNT,         // датчиков должно быть от 1 до 16
    ERR_NO_MEMORY,         // не хватило памяти под калибровку
    ERR_NOT_CALIBRATED,    // калибровка не выполнена: нужны значения над белым и над чёрным
    ERR_FLAT,              // на датчике нет разницы между белым и чёрным: проверьте высоту и питание
    ERR_BAD_VALUE          // параметр вне допустимого
  };
  enum Language : uint8_t { LANG_RU = 0, LANG_KK = 1, LANG_EN = 2 };

  // pins: массив пинов (аналоговых или цифровых), count: сколько датчиков (1..16)
  AlashLineSensor(const uint8_t *pins, uint8_t count);
  ~AlashLineSensor();
  AlashLineSensor(const AlashLineSensor &) = delete;
  AlashLineSensor &operator=(const AlashLineSensor &) = delete;

  bool begin();                                     // настроить пины, выделить память; false при ошибке (см. error())
  void setEmitterPin(uint8_t pin, bool activeHigh = true);   // вывод включения подсветки (LEDON у QTR): включается на время измерения
  void setDigital(bool digital) { _digital = digital; }       // цифровые датчики (HIGH на тёмном, как у модулей TCRT5000 с компаратором): калибровка не нужна
  void setAdcMax(uint16_t maxValue) { _adcMax = maxValue; }   // максимум АЦП: 1023 (Uno, ESP8266 с делителем) или 4095 (ESP32)
  void setSamples(uint8_t n) { _samples = n < 1 ? 1 : (n > 16 ? 16 : n); }   // сколько измерений усреднять
  void setWhiteLine(bool white) { _white = white; }           // белая линия на тёмном фоне
  void setInvert(bool invert) { _invert = invert; }           // если ваш модуль на тёмном даёт МЕНЬШЕЕ число (у QTR и большинства модулей TCRT5000 на тёмном число больше)
  void setThreshold(uint16_t t) { _threshold = t > 1000 ? 1000 : t; }   // порог «датчик видит линию» (0..1000), по умолчанию 500

  // ---- Калибровка: поводите робота над линией и фоном ---------------------------------------
  void calibrateStart();                            // обнулить накопленные минимум и максимум
  void calibrateStep();                             // один шаг: вызывайте в цикле, пока водите датчик
  bool calibrate(uint16_t ms = 3000);               // блокирующая калибровка (шаг примерно раз в 5 мс)
  bool isCalibrated() const { return _calibrated; }
  uint16_t calMin(uint8_t i) const { return i < _n ? _min[i] : 0; }
  uint16_t calMax(uint8_t i) const { return i < _n ? _max[i] : 0; }
  void setCalibration(uint8_t i, uint16_t minValue, uint16_t maxValue);   // записать готовые числа (из Calibrate) в скетч

  // ---- Чтение ---------------------------------------------------------------------------------------
  void readRaw(uint16_t *out);                      // сырые значения АЦП
  void read(uint16_t *out);                         // нормированные 0..1000: 1000 = «чёрное» (или белое при setWhiteLine)
  uint16_t position();                              // положение линии 0..(count-1)*1000; при потере линии крайнее значение с той стороны, где она была
  int16_t offset();                                 // от центра: -(count-1)*500 .. +(count-1)*500, 0 когда линия посередине
  bool lineDetected() const { return _seen; }       // линия под хотя бы одним датчиком при последнем чтении
  bool lineLost() const { return !_seen; }
  bool crossing() const { return _crossing; }       // линия под (почти) всеми датчиками: перекрёсток или стартовая полоса
  bool allClear() const { return _clear; }          // линия не видна ни одним датчиком
  uint8_t count() const { return _n; }
  uint8_t activeCount() const { return _active; }   // сколько датчиков видят линию

  // ---- Диагностика ------------------------------------------------------------------
  Error error() const { return _err; }
  void clearError() { _err = ERR_OK; }
  void setLanguage(Language lang) { _lang = lang; }
  const __FlashStringHelper *errorText();
  void diagnose(Print &out);                        // показывает сырые, нормированные значения и подсказывает

 private:
  uint16_t sample(uint8_t i);
  const uint8_t *_pins; uint8_t _n;
  uint16_t *_min = nullptr, *_max = nullptr;
  uint16_t _adcMax = 1023, _threshold = 500;
  uint8_t _samples = 4, _emitter = 255, _active = 0;
  bool _emitterHigh = true, _digital = false, _white = false, _invert = false, _calibrated = false, _begun = false;
  bool _seen = false, _crossing = false, _clear = true, _lastRight = false;
  uint16_t _lastPos = 0;
  Error _err = ERR_OK; Language _lang = LANG_RU;
};

#endif
