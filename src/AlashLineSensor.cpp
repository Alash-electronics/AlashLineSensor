// MIT License. Alash Electronics.
#include "AlashLineSensor.h"
#include <stdlib.h>

#define T(ru, kk, en) (_lang == LANG_KK ? F(kk) : _lang == LANG_EN ? F(en) : F(ru))

AlashLineSensor::AlashLineSensor(const uint8_t *pins, uint8_t count) : _pins(pins), _n(count) {}

AlashLineSensor::~AlashLineSensor() { free(_min); free(_max); }

bool AlashLineSensor::begin() {
  if (_n < 1 || _n > 16 || (_type != CUSTOM && !_pins) || (_type == CUSTOM && !_reader)) { _err = ERR_BAD_COUNT; return false; }
  free(_min); free(_max);
  _min = (uint16_t *)malloc(_n * sizeof(uint16_t));
  _max = (uint16_t *)malloc(_n * sizeof(uint16_t));
  if (!_min || !_max) { free(_min); free(_max); _min = _max = nullptr; _err = ERR_NO_MEMORY; return false; }
  if (_type == RC) _adcMax = _rcTimeout;
  for (uint8_t i = 0; i < _n; i++) { _min[i] = _adcMax; _max[i] = 0; if (_pins && _type != RC) pinMode(_pins[i], INPUT); }
  _begun = true;
  emitters(3, false);                          // подсветка выключена, пока не измеряем
  _calibrated = false; _err = ERR_OK; _lastPos = 0; _lastRight = false;
  return true;
}

void AlashLineSensor::setRcTimeout(uint16_t us) {
  if (us < 200 || us > 32000) { _err = ERR_BAD_VALUE; return; }
  _rcTimeout = us;
  if (_type == RC) _adcMax = us;
}

void AlashLineSensor::setEmitterPin(uint8_t pin, bool activeHigh) {
  _emitter = pin; _emitterHigh = activeHigh;
  if (_begun) { pinMode(_emitter, OUTPUT); emitters(1, false); }
}

void AlashLineSensor::setEmitterPins(uint8_t oddPin, uint8_t evenPin, bool activeHigh) {
  _emitterOdd = oddPin; _emitterEven = evenPin; _emitterHigh = activeHigh;
  if (_begun) { pinMode(_emitterOdd, OUTPUT); pinMode(_emitterEven, OUTPUT); emitters(3, false); }
}

// which: бит 0 = общая подсветка и нечётные, бит 1 = чётные. 1 = общая/нечётные, 2 = чётные, 3 = все.
void AlashLineSensor::emitters(uint8_t which, bool on) {
  uint8_t level = (on == _emitterHigh) ? HIGH : LOW;
  if (_emitter != 255 && (which & 1)) { pinMode(_emitter, OUTPUT); digitalWrite(_emitter, level); }
  if (_emitterOdd != 255 && (which & 1)) { pinMode(_emitterOdd, OUTPUT); digitalWrite(_emitterOdd, level); }
  if (_emitterEven != 255 && (which & 2)) { pinMode(_emitterEven, OUTPUT); digitalWrite(_emitterEven, level); }
}

// Одно значение датчика: среднее по нескольким измерениям (аналоговый вход)
uint16_t AlashLineSensor::sample(uint8_t i) {
  uint32_t s = 0;
  for (uint8_t k = 0; k < _samples; k++) s += analogRead(_pins[i]);
  return (uint16_t)(s / _samples);
}

// RC-датчики: зарядить конденсатор (пин на выход, HIGH), перевести пин на вход и мерить, за сколько он разрядится до LOW; тёмное разряжается дольше
void AlashLineSensor::readRc(uint16_t *out, uint8_t start, uint8_t step) {
  for (uint8_t i = start; i < _n; i += step) { pinMode(_pins[i], OUTPUT); digitalWrite(_pins[i], HIGH); out[i] = _rcTimeout; }
  delayMicroseconds(10);
  for (uint8_t i = start; i < _n; i += step) pinMode(_pins[i], INPUT);     // без подтяжки: конденсатор разряжается через фототранзистор
  uint32_t t0 = micros();
  uint8_t pending = 0;
  for (uint8_t i = start; i < _n; i += step) pending++;
  while (pending) {
    uint32_t dt = micros() - t0;
    if (dt >= _rcTimeout) break;
    for (uint8_t i = start; i < _n; i += step)
      if (out[i] == _rcTimeout && digitalRead(_pins[i]) == LOW) { out[i] = (uint16_t)dt; pending--; }
  }
}

// Один проход по датчикам start, start+step, ... в нужном типе
void AlashLineSensor::readOnce(uint16_t *out, uint8_t start, uint8_t step) {
  if (_type == RC) { readRc(out, start, step); return; }
  for (uint8_t i = start; i < _n; i += step) {
    if (_type == CUSTOM) out[i] = _reader(i);
    else if (_type == DIGITAL) out[i] = digitalRead(_pins[i]) ? _adcMax : 0;
    else out[i] = sample(i);
  }
}

void AlashLineSensor::readRaw(uint16_t *out) {
  bool hasEmitter = _emitter != 255 || _emitterOdd != 255 || _emitterEven != 255;
  if (!hasEmitter || _mode == READ_ON) {
    if (hasEmitter) { emitters(3, true); delayMicroseconds(200); }
    readOnce(out, 0, 1);
    if (hasEmitter) emitters(3, false);
  } else if (_mode == READ_OFF) {
    emitters(3, false); delayMicroseconds(200);
    readOnce(out, 0, 1);
  } else if (_mode == READ_ODD_EVEN && _emitterOdd != 255 && _emitterEven != 255) {
    emitters(1, true); delayMicroseconds(200); readOnce(out, 0, 2); emitters(1, false);      // нечётные по счёту человека: индексы 0, 2, 4 (первый датчик = нечётный)
    emitters(2, true); delayMicroseconds(200); readOnce(out, 1, 2); emitters(2, false);
  } else {                                                                                    // ON_AND_OFF: вычесть внешний свет
    uint16_t off[16];
    emitters(3, true); delayMicroseconds(200); readOnce(out, 0, 1); emitters(3, false);
    delayMicroseconds(1200);
    readOnce(off, 0, 1);
    for (uint8_t i = 0; i < _n; i++) {
      int32_t v = (int32_t)out[i] + _adcMax - off[i];     // как у QTR: чтение с подсветкой плюс максимум минус чтение без неё
      out[i] = (uint16_t)(v < 0 ? 0 : (v > _adcMax ? _adcMax : v));
    }
  }
}

void AlashLineSensor::calibrateStart() {
  if (!_begun) { _err = ERR_BAD_COUNT; return; }
  for (uint8_t i = 0; i < _n; i++) { _min[i] = _adcMax; _max[i] = 0; }
  _calibrated = false;
}

void AlashLineSensor::calibrateStep() {
  if (!_begun) { _err = ERR_BAD_COUNT; return; }
  uint16_t v[16];
  readRaw(v);
  for (uint8_t i = 0; i < _n; i++) {
    if (v[i] < _min[i]) _min[i] = v[i];
    if (v[i] > _max[i]) _max[i] = v[i];
  }
  // калибровка готова, когда у каждого датчика есть заметная разница между светлым и тёмным (не меньше 5 % шкалы)
  bool ok = true;
  for (uint8_t i = 0; i < _n; i++) if (_max[i] < _min[i] || (uint32_t)(_max[i] - _min[i]) * 20 < _adcMax) ok = false;
  _calibrated = ok;
}

bool AlashLineSensor::calibrate(uint16_t ms) {
  if (!_begun) { _err = ERR_BAD_COUNT; return false; }
  calibrateStart();
  uint32_t t0 = millis();
  while ((uint32_t)(millis() - t0) < ms) { calibrateStep(); delay(5); }
  if (!_calibrated) _err = ERR_FLAT;
  else _err = ERR_OK;
  return _calibrated;
}

void AlashLineSensor::setCalibration(uint8_t i, uint16_t minValue, uint16_t maxValue) {
  if (!_begun || i >= _n || maxValue <= minValue) { _err = ERR_BAD_VALUE; return; }
  _min[i] = minValue; _max[i] = maxValue;
  bool ok = true;
  for (uint8_t k = 0; k < _n; k++) if (_max[k] <= _min[k]) ok = false;    // все датчики получили числа
  _calibrated = ok;
}

// 0 = «светлое», 1000 = «тёмное» (линия); с белой линией наоборот. Предполагается, что тёмное даёт БОЛЬШЕЕ число (как у QTR); иначе setInvert(true)
void AlashLineSensor::read(uint16_t *out) {
  uint16_t raw[16];
  readRaw(raw);
  _active = 0;
  for (uint8_t i = 0; i < _n; i++) {
    uint32_t v;
    if (_type == DIGITAL) v = raw[i] ? 1000 : 0;
    else if (!_begun || !_calibrated || _max[i] <= _min[i]) { _err = ERR_NOT_CALIBRATED; v = (uint32_t)raw[i] * 1000UL / (_adcMax ? _adcMax : 1); }
    else {
      uint16_t r = raw[i] < _min[i] ? _min[i] : (raw[i] > _max[i] ? _max[i] : raw[i]);
      v = (uint32_t)(r - _min[i]) * 1000UL / (_max[i] - _min[i]);
    }
    if (_white != _invert) v = 1000 - v;       // белая линия или обратная полярность модуля
    out[i] = (uint16_t)v;
    if (v >= _threshold) _active++;
  }
  _seen = _active > 0;
  _clear = !_seen;
  _crossing = _n > 2 && _active >= _n - (_n > 5 ? 1 : 0);     // у линейки из 3-5 датчиков все, из 6 и больше все, кроме одного
}

// Взвешенное среднее по датчикам, как у QTR: 0 под первым датчиком, (n-1)*1000 под последним.
uint16_t AlashLineSensor::position() {
  uint16_t v[16];
  read(v);
  uint32_t sum = 0, weighted = 0;
  for (uint8_t i = 0; i < _n; i++) {
    if (v[i] < 50) continue;                                   // шум фона не считаем
    sum += v[i]; weighted += (uint32_t)v[i] * i * 1000UL;
  }
  uint16_t last = (uint16_t)((_n - 1) * 1000);
  if (sum == 0 || !_seen) return _lastRight ? last : 0;        // линия потеряна: вернуть ту сторону, где её видели последний раз
  uint16_t pos = (uint16_t)(weighted / sum);
  _lastRight = pos > last / 2;
  _lastPos = pos;
  return pos;
}

int16_t AlashLineSensor::offset() { return (int16_t)((int32_t)position() - (int32_t)(_n - 1) * 500); }

const __FlashStringHelper *AlashLineSensor::errorText() {
  switch (_err) {
    case ERR_OK: return F("OK");
    case ERR_BAD_COUNT:
      return T("Число датчиков должно быть от 1 до 16 и массив пинов задан; вызовите begin() до калибровки и чтения.",
               "Датчиктер саны 1-ден 16-ға дейін болуы және пиндер массиві берілуі керек; калибрлеу мен оқудан бұрын begin() шақырыңыз.",
               "The number of sensors must be 1 to 16 and the pin array given; call begin() before calibrating and reading.");
    case ERR_NO_MEMORY:
      return T("Не хватило памяти под калибровку: уменьшите число датчиков.", "Калибрлеуге жад жетпеді: датчиктер санын азайтыңыз.", "Not enough memory for calibration: use fewer sensors.");
    case ERR_NOT_CALIBRATED:
      return T("Калибровка не выполнена: значения нормируются грубо. Вызовите calibrate() и поводите датчики над линией и фоном (примерно 3 секунды), либо запишите готовые числа через setCalibration().",
               "Калибрлеу орындалмаған: мәндер дөрекі нормаланады. calibrate() шақырып, датчиктерді сызық пен фон үстінде жүргізіңіз (шамамен 3 секунд) не дайын сандарды setCalibration() арқылы жазыңыз.",
               "Calibration is not done: the values are normalized roughly. Call calibrate() and sweep the sensors over the line and the background (about 3 seconds), or enter ready numbers with setCalibration().");
    case ERR_FLAT:
      return T("Калибровка не удалась: у одного или нескольких датчиков нет разницы между светлым и тёмным. Поводите линейку именно над линией и над фоном. Проверьте высоту над полом (3-9 мм), питание подсветки и контрастность линии.",
               "Калибрлеу сәтсіз: бір немесе бірнеше датчикте ашық пен қараңғы арасында айырмашылық жоқ. Линейканы дәл сызық пен фон үстінде жүргізіңіз. Еденнен биіктікті (3-9 мм), жарықтандыру қуатын және сызық контрастын тексеріңіз.",
               "Calibration failed: one or more sensors see no difference between light and dark. Sweep the bar over both the line and the background. Check the height above the floor (3-9 mm), the emitter power and the line contrast.");
    case ERR_BAD_VALUE:
      return T("Номер датчика вне диапазона или максимум не больше минимума.", "Датчик нөмірі аралықтан тыс немесе максимум минимумнан үлкен емес.", "The sensor index is out of range or the maximum is not above the minimum.");
  }
  return F("");
}

void AlashLineSensor::diagnose(Print &out) {
  out.println(T("AlashLineSensor: диагностика", "AlashLineSensor: диагностика", "AlashLineSensor: diagnostics"));
  if (!_begun) { out.println(T("Линейка не запущена: вызовите begin() в setup().", "Линейка іске қосылмаған: setup() ішінде begin() шақырыңыз.", "The bar is not started: call begin() in setup().")); return; }
  out.print(T("Датчиков: ", "Датчиктер: ", "Sensors: ")); out.print((long)_n);
  out.print(_type == DIGITAL ? T("   цифровые", "   сандық", "   digital") : _type == RC ? F("   RC") : _type == CUSTOM ? T("   своя функция", "   өз функциясы", "   custom reader") : T("   аналоговые", "   аналогтық", "   analog"));
  out.print(T("   максимум АЦП: ", "   АЦП максимумы: ", "   ADC max: ")); out.println((long)_adcMax);
  uint16_t raw[16], norm[16];
  readRaw(raw);
  read(norm);
  for (uint8_t i = 0; i < _n; i++) {
    out.print(F("#")); out.print((long)i); out.print(F("  raw=")); out.print((long)raw[i]);
    out.print(F("  min=")); out.print((long)_min[i]); out.print(F("  max=")); out.print((long)_max[i]);
    out.print(F("  norm=")); out.println((long)norm[i]);
  }
  out.print(T("Калибровка: ", "Калибрлеу: ", "Calibration: ")); out.println(_calibrated ? T("есть", "бар", "done") : T("НЕТ", "ЖОҚ", "NOT DONE"));
  uint8_t stuckLow = 0, stuckHigh = 0;
  for (uint8_t i = 0; i < _n; i++) { if (raw[i] <= 2) stuckLow++; if (_type != DIGITAL && raw[i] + 2 >= _adcMax) stuckHigh++; }
  if (stuckLow == _n) out.println(T("Все датчики показывают 0: нет питания линейки или обрыв сигнальных проводов.", "Барлық датчик 0 көрсетеді: линейкада қуат жоқ немесе сигнал сымдары үзілген.", "All sensors read 0: no power to the bar or the signal wires are broken."));
  if (stuckHigh == _n) out.println(T("Все датчики показывают максимум: на пины приходит питание вместо сигнала или датчики закрыты чёрным.", "Барлық датчик максимум көрсетеді: пиндерге сигнал орнына қуат келеді немесе датчиктер қарамен жабылған.", "All sensors read the maximum: supply arrives instead of the signal or the sensors are covered by black."));
  out.println(T("Подсказки: держите линейку на высоте 3-9 мм от пола (дальше контраст падает). Калибруйте на той поверхности, где будет ехать робот, при том освещении, что будет на соревновании: яркое солнце и лампы с ИК-подсветкой меняют показания. Чёрная изолента даёт лучший контраст, чем маркер. Если робот ловит линию с запозданием, поднимите скорость опроса или уменьшите setSamples().",
                "Кеңестер: линейканы еденнен 3-9 мм биіктікте ұстаңыз (алыстаған сайын контраст төмендейді). Робот жүретін бетте, жарыс кезіндегі жарықтандыруда калибрлеңіз: күшті күн және ИК-жарықтандырғышы бар шамдар көрсеткіштерді өзгертеді. Қара оқшаулағыш таспа маркерден жақсы контраст береді. Робот сызықты кешігіп ұстаса, сұрау жылдамдығын көтеріңіз немесе setSamples() азайтыңыз.",
                "Tips: hold the bar 3-9 mm above the floor (contrast falls farther away). Calibrate on the surface the robot will drive on, under the light of the competition: bright sun and lamps with IR emission change the readings. Black insulating tape gives better contrast than a marker. If the robot catches the line late, poll faster or lower setSamples()."));
}
