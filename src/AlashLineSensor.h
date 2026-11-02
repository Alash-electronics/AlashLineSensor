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
  // Как читаются датчики: SENSOR_ANALOG (analogRead), SENSOR_DIGITAL (выход с компаратором, HIGH/LOW), SENSOR_RC (QTR-RC: конденсатор разряжается через фототранзистор, меряем время), SENSOR_CUSTOM (своя функция чтения)
  enum SensorType : uint8_t { SENSOR_ANALOG = 0, SENSOR_DIGITAL, SENSOR_RC, SENSOR_CUSTOM };
  // Что делает подсветка при чтении: ON (всегда включена на время чтения), OFF (читать только внешний свет), ON_AND_OFF (вычесть внешний свет: чтение с подсветкой плюс максимум минус чтение без неё),
  // ODD_EVEN (нечётные и чётные датчики по очереди со своими подсветками: меньше засветка соседями; нужны два пина подсветки)
  enum ReadMode : uint8_t { READ_ON = 0, READ_OFF, READ_ON_AND_OFF, READ_ODD_EVEN };
  typedef uint16_t (*Reader)(uint8_t index);   // своя функция: вернуть сырое значение датчика index (мультиплексор CD4051, АЦП ADS1115 по I2C, расширитель пинов, ...)

  // pins: массив пинов (аналоговых или цифровых), count: сколько датчиков (1..16)
  AlashLineSensor(const uint8_t *pins, uint8_t count);
  // Без пинов: данные даёт ваша функция, задайте её через setReader()
  explicit AlashLineSensor(uint8_t count) : _pins(nullptr), _n(count) { _type = SENSOR_CUSTOM; }
  ~AlashLineSensor();
  AlashLineSensor(const AlashLineSensor &) = delete;
  AlashLineSensor &operator=(const AlashLineSensor &) = delete;

  bool begin();                                     // настроить пины, выделить память; false при ошибке (см. error())
  void setEmitterPin(uint8_t pin, bool activeHigh = true);   // вывод включения подсветки (LEDON у QTR): включается на время измерения
  void setEmitterPins(uint8_t oddPin, uint8_t evenPin, bool activeHigh = true);   // отдельные подсветки нечётных и чётных датчиков (QTR второго поколения: CTRL ODD и CTRL EVEN)
  void setReadMode(ReadMode mode) { _mode = mode; }
  void setSensorType(SensorType t) { if (t != SENSOR_CUSTOM || _reader) _type = t; }
  void setDigital(bool digital) { _type = digital ? SENSOR_DIGITAL : SENSOR_ANALOG; }   // коротко: цифровые датчики (HIGH на тёмном, как у модулей TCRT5000 с компаратором): калибровка не нужна
  void setReader(Reader fn) { _reader = fn; if (fn) _type = SENSOR_CUSTOM; }
  void setRcTimeout(uint16_t us);                             // для RC: время в мкс, дольше которого считаем «совсем чёрное» (1000-3000, по умолчанию 2500)
  void setAdcMax(uint16_t maxValue) { _adcMax = maxValue; }   // максимум АЦП: 1023 (Uno), 4095 (ESP32) или максимум вашей функции чтения
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
  void readOnce(uint16_t *out, uint8_t start, uint8_t step);
  void readRc(uint16_t *out, uint8_t start, uint8_t step);
  void emitters(uint8_t which, bool on);
  const uint8_t *_pins; uint8_t _n;
  uint16_t *_min = nullptr, *_max = nullptr;
  uint16_t _adcMax = 1023, _threshold = 500;
  uint8_t _samples = 4, _emitter = 255, _emitterOdd = 255, _emitterEven = 255, _active = 0;
  SensorType _type = SENSOR_ANALOG; ReadMode _mode = READ_ON; Reader _reader = nullptr; uint16_t _rcTimeout = 2500;
  bool _emitterHigh = true, _white = false, _invert = false, _calibrated = false, _begun = false;
  bool _seen = false, _crossing = false, _clear = true, _lastRight = false;
  uint16_t _lastPos = 0;
  Error _err = ERR_OK; Language _lang = LANG_RU;
};

#endif
