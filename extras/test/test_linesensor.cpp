// Тесты AlashLineSensor без железа: имитация линейки над полом с линией, разные усиления датчиков, шум, АЦП на 10 и 12 бит.
//   g++ -std=c++17 -Wall -fsanitize=address,undefined -I mock -I ../../src test_linesensor.cpp ../../src/AlashLineSensor.cpp && ./a.out
#include <stdio.h>
#include "Arduino.h"
#include "AlashLineSensor.h"

static int fails = 0;
#define CHECK(cond) do { if (!(cond)) { printf("FAIL line %d: %s\n", __LINE__, #cond); fails++; } } while (0)
typedef AlashLineSensor L;
static const uint8_t PINS[8] = {20, 21, 22, 23, 24, 25, 26, 27};

// «Поводить над линией и фоном»: линия смещается по всем положениям
static void sweep(L &s, int steps = 80) { s.calibrateStart(); for (int k = 0; k < steps; k++) { g_floor.lineCenter = -45 + 90.0f * k / (steps - 1); s.calibrateStep(); } }
static void reset(int n = 5) { g_floor = Floor(); g_floor.n = n; mockReset(); g_ms += 100000; }

int main() {
  // 1) begin и ошибки параметров
  { reset(); L bad(PINS, 0); CHECK(!bad.begin() && bad.error() == L::ERR_BAD_COUNT);
    L big(PINS, 17); CHECK(!big.begin() && big.error() == L::ERR_BAD_COUNT);
    L np(nullptr, 3); CHECK(!np.begin());
    L s(PINS, 5); CHECK(s.begin() && s.count() == 5 && !s.isCalibrated());
    s.calibrateStep(); CHECK(!s.isCalibrated());                                  // одно измерение над фоном ещё не калибровка
    L raw(PINS, 5); raw.calibrateStep(); CHECK(raw.error() == L::ERR_BAD_COUNT);   // до begin()
  }

  // 2) калибровка водит линию над всеми датчиками и находит минимум и максимум
  { reset(); for (int i = 0; i < 5; i++) { g_floor.gain[i] = 0.7f + 0.08f * i; g_floor.shift[i] = 5 * i; }
    L s(PINS, 5); s.begin(); sweep(s);
    CHECK(s.isCalibrated());
    for (int i = 0; i < 5; i++) { CHECK(abs((int)s.calMin(i) - (int)(80 * g_floor.gain[i] + g_floor.shift[i])) <= 1 && abs((int)s.calMax(i) - (int)(900 * g_floor.gain[i] + g_floor.shift[i])) <= 1); }
    CHECK(s.calMin(9) == 0 && s.calMax(9) == 0);
  }

  // 3) нормировка 0..1000 несмотря на разные усиления; чёрная линия = 1000
  { reset(); for (int i = 0; i < 5; i++) { g_floor.gain[i] = 0.6f + 0.1f * i; g_floor.shift[i] = 8 * i; }
    L s(PINS, 5); s.begin(); sweep(s);
    g_floor.lineCenter = 0; uint16_t v[16]; s.read(v);
    CHECK(v[2] >= 995 && v[0] <= 5 && v[4] <= 5 && v[1] <= 5 && v[3] <= 5);
    g_floor.lineCenter = -200; s.read(v); for (int i = 0; i < 5; i++) CHECK(v[i] <= 5);       // линии нет нигде: всё около нуля
  }

  // 4) положение линии: монотонно растёт при движении линии, ровно посередине даёт центр, плавно между датчиками
  { reset(); L s(PINS, 5); s.begin(); sweep(s);
    uint16_t prev = 0; bool mono = true;
    for (int k = -18; k <= 18; k++) { g_floor.lineCenter = k * 2.0f; uint16_t p = s.position(); if (k > -18 && p + 1 < prev) mono = false; prev = p; }
    CHECK(mono);
    g_floor.lineCenter = 0; CHECK(abs((int)s.position() - 2000) <= 20 && abs(s.offset()) <= 20);
    g_floor.lineCenter = -20; CHECK(abs((int)s.position() - 0) <= 200);                                  // под крайним левым датчиком
    g_floor.lineCenter = 20; CHECK(abs((int)s.position() - 4000) <= 200 && s.offset() > 1800);
    g_floor.lineCenter = 5; uint16_t p = s.position(); CHECK(p > 2000 && p < 3000);                       // между центральным и следующим
    CHECK(s.lineDetected() && !s.lineLost() && !s.allClear());
  }

  // 5) потеря линии: возвращается сторона, где её видели последний раз
  { reset(); L s(PINS, 5); s.begin(); sweep(s);
    g_floor.lineCenter = 18; s.position(); g_floor.lineCenter = 300;
    CHECK(s.position() == 4000 && s.lineLost() && s.allClear());                                         // ушла вправо
    g_floor.lineCenter = -18; s.position(); g_floor.lineCenter = -300;
    CHECK(s.position() == 0 && s.lineLost() && s.offset() == -2000);
  }

  // 6) перекрёсток: линия под всеми датчиками (широкая полоса)
  { reset(); L s(PINS, 5); s.begin(); sweep(s);
    g_floor.lineWidth = 200; g_floor.lineCenter = 0; uint16_t v[16]; s.read(v);
    CHECK(s.crossing() && s.activeCount() == 5);
    g_floor.lineWidth = 12; g_floor.lineCenter = 0; s.read(v); CHECK(!s.crossing() && s.activeCount() == 1);
    reset(8); L e(PINS, 8); e.begin(); sweep(e); g_floor.lineWidth = 200; e.read(v); CHECK(e.crossing());
  }

  // 7) белая линия на тёмном фоне
  { reset(); g_floor.whiteLine = true; L s(PINS, 5); s.setWhiteLine(true); s.begin(); sweep(s);
    g_floor.lineCenter = 10; uint16_t p = s.position(); CHECK(p > 2400 && p < 3600 && s.lineDetected());
    uint16_t v[16]; s.read(v); CHECK(v[3] >= 990 || v[2] >= 990);
  }

  // 8) шум и усреднение: больше измерений, меньше разброс положения
  { reset(); L s(PINS, 5); s.begin(); sweep(s); g_floor.noise = 120; g_floor.lineCenter = 3;
    auto spread = [&](uint8_t n) { s.setSamples(n); int lo = 99999, hi = 0; for (int k = 0; k < 200; k++) { int p = s.position(); if (p < lo) lo = p; if (p > hi) hi = p; } return hi - lo; };
    int a = spread(1), b = spread(16); CHECK(b < a);
    s.setSamples(0); CHECK(s.position() <= 4000);                                                         // 0 превращается в 1
    s.setSamples(200);
  }

  // 9) АЦП на 12 бит (ESP32)
  { reset(); g_floor.adcMax = 4095; L s(PINS, 5); s.setAdcMax(4095); s.begin(); sweep(s);
    CHECK(s.isCalibrated() && s.calMax(0) > 3000); g_floor.lineCenter = 0; CHECK(abs((int)s.position() - 2000) <= 20);
  }

  // 10) цифровые датчики: калибровка не нужна
  { reset(); L s(PINS, 5); s.setDigital(true); s.begin();
    g_floor.lineCenter = 10; uint16_t v[16]; s.read(v); CHECK(s.lineDetected() && (v[3] == 1000 || v[2] == 1000) && v[0] == 0);
    CHECK(abs((int)s.position() - 2500) <= 600);
  }

  // 11) подсветка включается только на время измерения (общая, активный HIGH и активный LOW)
  { reset(); g_commonEm = 7; g_emActiveHigh = true; L s(PINS, 5); s.begin(); s.setEmitterPin(7, true); sweep(s, 20);
    CHECK(!emOn(7) && g_floor.emitterReads == g_floor.reads);                                  // все чтения шли при включённой подсветке
    g_floor.emitterReads = g_floor.reads = 0; uint16_t v[16]; s.read(v); CHECK(g_floor.emitterReads == g_floor.reads && g_floor.reads > 0 && !emOn(7));
    reset(); g_commonEm = 7; g_emActiveHigh = false; L t(PINS, 5); t.begin(); t.setEmitterPin(7, false); t.read(v); CHECK(g_floor.emitterReads == g_floor.reads && !emOn(7) && g_pinLevel[7] == HIGH);   // активный LOW: выключена уровнем HIGH
  }

  // 11б) режим вычитания внешнего света: яркий свет занижает показания, ON_AND_OFF возвращает прежние числа
  { reset(); g_commonEm = 7; L s(PINS, 5); s.begin(); s.setEmitterPin(7, true); sweep(s);
    g_floor.lineCenter = 0; g_floor.ambient = 60; uint16_t a[16], b[16];
    s.setReadMode(L::READ_ON); s.readRaw(a);
    s.setReadMode(L::READ_ON_AND_OFF); s.readRaw(b);
    CHECK(a[2] == 900 - 60 && abs((int)b[2] - 900) <= 1 && abs((int)b[0] - 80) <= 1);        // с поправкой те же 900 и 80, что и без внешнего света
    CHECK(g_floor.emitterReads > 0 && !emOn(7));
    s.setReadMode(L::READ_OFF); s.readRaw(a); CHECK(a[0] == 1023 - 60 && a[2] == 1023 - 60);  // без подсветки видно только внешний свет
  }

  // 11в) чередование нечётных и чётных подсветок: соседняя подсветка не горит
  { reset(); g_oddEm = 8; g_evenEm = 9; L s(PINS, 5); s.begin(); s.setEmitterPins(8, 9, true); s.setReadMode(L::READ_ODD_EVEN); sweep(s);
    CHECK(g_floor.wrongEmitter == 0 && !emOn(8) && !emOn(9));
    g_floor.lineCenter = 0; uint16_t v[16]; s.read(v); CHECK(v[2] >= 995 && v[1] <= 5 && v[3] <= 5);
    L t(PINS, 5); t.begin(); t.setReadMode(L::READ_ODD_EVEN); t.readRaw(v);                    // режим без пинов подсветки не падает
  }

  // 11г) RC-датчики (QTR-RC): время разряда конденсатора, тёмное дольше
  { reset(); L s(PINS, 5); s.setSensorType(L::SENSOR_RC); s.begin(); CHECK(s.isCalibrated() == false);
    g_floor.lineCenter = 0; uint16_t raw[16]; s.readRaw(raw);
    CHECK(raw[2] >= 1900 && raw[2] <= 2100 && raw[0] >= 250 && raw[0] <= 400);                 // над линией около 2000 мкс, над фоном около 300
    sweep(s); g_floor.lineCenter = 0; uint16_t v[16]; s.read(v); CHECK(v[2] >= 990 && v[0] <= 10 && abs((int)s.position() - 2000) <= 30);
    g_floor.rcDarkUs = 9000; s.readRaw(raw); CHECK(raw[2] == 2500);                            // дольше таймаута: считается совсем чёрным
    s.setRcTimeout(1000); CHECK(s.error() == L::ERR_OK); s.setRcTimeout(50); CHECK(s.error() == L::ERR_BAD_VALUE);
    reset(); g_floor.rcDarkUs = 1800; L w(PINS, 5); w.setSensorType(L::SENSOR_RC); w.setRcTimeout(3000); w.begin(); w.readRaw(raw); CHECK(raw[2] >= 1700 && raw[2] <= 1900);
    CHECK(w.calMax(0) == 0 && w.calMin(0) == 3000);                                            // максимум шкалы равен таймауту
  }

  // 11д) своя функция чтения (мультиплексор, АЦП по I2C и т. д.), без пинов
  { reset(); static int phys[6]; static int calls = 0;
    struct H { static uint16_t rd(uint8_t i) { calls++; return (uint16_t)phys[i]; } };
    L s(6); s.setReader(H::rd); s.setAdcMax(4095); CHECK(s.begin() && s.count() == 6);
    for (int i = 0; i < 6; i++) phys[i] = 3500; phys[3] = 400;
    s.calibrateStart(); for (int i = 0; i < 6; i++) { s.setCalibration(i, 400, 3500); }
    for (int i = 0; i < 6; i++) phys[i] = 400; phys[2] = 3500;                               // тёмное даёт большее число
    uint16_t v[16]; s.read(v); CHECK(v[2] == 1000 && v[0] == 0 && calls >= 6);
    CHECK(s.position() == 2000);
    L bad(6); CHECK(!bad.begin() && bad.error() == L::ERR_BAD_COUNT);                         // своя функция не задана
    L mixed(PINS, 5); mixed.setSensorType(L::SENSOR_CUSTOM); CHECK(mixed.begin());                   // CUSTOM без функции не включается: остаётся аналоговым
  }

  // 12) калибровка не удалась (нет контраста) и ручной ввод готовых чисел
  { reset(); g_floor.dark = 500; g_floor.light = 505; L s(PINS, 5); s.begin();
    CHECK(!s.calibrate(500) && s.error() == L::ERR_FLAT && !s.isCalibrated());
    uint16_t v[16]; s.read(v); CHECK(s.error() == L::ERR_NOT_CALIBRATED);                                  // читает грубо, но говорит, что калибровки нет
    reset(); L m(PINS, 3); m.begin();
    m.setCalibration(0, 80, 900); m.setCalibration(1, 80, 900); CHECK(!m.isCalibrated());
    m.setCalibration(2, 100, 800); CHECK(m.isCalibrated());
    m.clearError(); m.setCalibration(9, 0, 10); CHECK(m.error() == L::ERR_BAD_VALUE); m.clearError(); m.setCalibration(1, 500, 500); CHECK(m.error() == L::ERR_BAD_VALUE);
    reset(); g_floor.autoSweep = true; L ok(PINS, 5); ok.begin(); uint32_t t = g_ms; CHECK(ok.calibrate(1000) && g_ms - t >= 1000 && ok.error() == L::ERR_OK);
  }

  // 12б) модуль с обратной полярностью (на тёмном число меньше): setInvert(true) возвращает нормальные значения
  { reset(); g_floor.dark = 80; g_floor.light = 900; L s(PINS, 5); s.setInvert(true); s.begin(); sweep(s);
    g_floor.lineCenter = 0; uint16_t v[16]; s.read(v); CHECK(v[2] >= 995 && v[0] <= 5 && abs((int)s.position() - 2000) <= 20);
    reset(); g_floor.dark = 80; g_floor.light = 900; g_floor.whiteLine = true; L w(PINS, 5); w.setInvert(true); w.setWhiteLine(true); w.begin(); sweep(w);   // оба переворота компенсируют друг друга
    g_floor.lineCenter = 0; w.read(v); CHECK(v[2] <= 5 || v[2] >= 995);
  }

  // 13) тексты ошибок на трёх языках и диагностика
  { for (int e = L::ERR_BAD_COUNT; e <= L::ERR_BAD_VALUE; e++) {
      reset(); L s(PINS, 5);
      if (e == L::ERR_BAD_COUNT) { L b(PINS, 0); b.begin(); b.setLanguage(L::LANG_RU); CHECK(b.error() == e); const char *ru = (const char *)b.errorText(); b.setLanguage(L::LANG_KK); const char *kk = (const char *)b.errorText(); b.setLanguage(L::LANG_EN); const char *en = (const char *)b.errorText(); CHECK(strlen(ru) > 20 && strlen(kk) > 20 && strlen(en) > 20 && strcmp(ru, kk) && strcmp(kk, en)); continue; }
      s.begin();
      if (e == L::ERR_NO_MEMORY) { s.setLanguage(L::LANG_EN); }
      else if (e == L::ERR_NOT_CALIBRATED) { uint16_t v[16]; s.read(v); }
      else if (e == L::ERR_FLAT) { g_floor.dark = 500; g_floor.light = 500; s.calibrate(100); }
      else s.setCalibration(9, 0, 1);
      if (e == L::ERR_NO_MEMORY) continue;                                                                  // нехватка памяти не воспроизводится на хосте
      CHECK(s.error() == e);
      s.setLanguage(L::LANG_RU); const char *ru = (const char *)s.errorText(); s.setLanguage(L::LANG_KK); const char *kk = (const char *)s.errorText(); s.setLanguage(L::LANG_EN); const char *en = (const char *)s.errorText();
      CHECK(strlen(ru) > 20 && strlen(kk) > 20 && strlen(en) > 20 && strcmp(ru, kk) && strcmp(kk, en) && strcmp(ru, en));
    }
    reset(); L ok(PINS, 5); CHECK(strcmp((const char *)ok.errorText(), "OK") == 0);
    reset(); L d(PINS, 5); d.begin(); d.setLanguage(L::LANG_EN); sweep(d); Print p; d.diagnose(p);
    CHECK(p.text.find("Sensors: 5") != std::string::npos && p.text.find("#4  raw=") != std::string::npos && p.text.find("Calibration: done") != std::string::npos && p.text.find("3-9 mm") != std::string::npos);
    reset(); L d2(PINS, 5); d2.begin(); d2.setLanguage(L::LANG_KK); Print p2; d2.diagnose(p2); CHECK(p2.text.find("ЖОҚ") != std::string::npos);
    reset(); g_floor.dark = 0; g_floor.light = 0; L z(PINS, 5); z.begin(); z.setLanguage(L::LANG_EN); Print p3; z.diagnose(p3); CHECK(p3.text.find("All sensors read 0") != std::string::npos);
    L nb(PINS, 5); Print p4; nb.diagnose(p4); CHECK(p4.text.find("begin()") != std::string::npos);
  }

  if (fails) { printf("FAILED: %d\n", fails); return 1; }
  printf("ALL TESTS PASSED\n");
  return 0;
}
