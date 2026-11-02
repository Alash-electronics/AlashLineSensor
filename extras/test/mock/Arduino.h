// Имитация Arduino.h и линейки датчиков отражения для тестов AlashLineSensor.
// Линия шириной lineWidth мм проходит над датчиками, стоящими с шагом pitch мм; у каждого датчика свой коэффициент усиления и смещение.
// Поддерживает аналоговые, цифровые и RC-датчики (разряд конденсатора по времени), подсветку (общую, нечётную и чётную) и внешний свет.
#pragma once
#include <math.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <string>
class __FlashStringHelper;
#define F(x) (reinterpret_cast<const __FlashStringHelper *>(x))
#define PROGMEM
#define INPUT 0
#define OUTPUT 1
#define LOW 0
#define HIGH 1
inline uint32_t g_ms = 1000, g_us = 0;
inline uint32_t millis() { return g_ms; }
inline uint32_t micros() { return g_us; }
inline void delay(uint32_t ms) { g_ms += ms; g_us += ms * 1000; }
inline void delayMicroseconds(unsigned us) { g_us += us; }
struct Floor {
  float lineCenter = 0, lineWidth = 12, pitch = 10;     // мм; датчик i стоит в точке (i - (n-1)/2) * pitch
  int n = 5, adcMax = 1023;
  int dark = 900, light = 80;                           // сырое число над линией (тёмное: большее, как у QTR) и над фоном (до индивидуальных поправок)
  float gain[16], shift[16];
  bool whiteLine = false;
  int noise = 0; uint32_t seed = 1;
  bool autoSweep = false; int phase = 0;                // линия сама ходит над датчиками (для блокирующей калибровки)
  int ambient = 0;                                      // внешний свет: уменьшает показания (и с подсветкой, и без)
  int rcDarkUs = 2000, rcLightUs = 300;                 // время разряда конденсатора над тёмным и светлым
  int emitterReads = 0, reads = 0, wrongEmitter = 0;
  Floor() { for (int i = 0; i < 16; i++) { gain[i] = 1; shift[i] = 0; } }
};
inline Floor g_floor;
// состояние пинов
inline int g_pinMode[64], g_pinLevel[64]; inline bool g_charged[64]; inline uint32_t g_dischargeStart[64];
inline int g_commonEm = -1, g_oddEm = -1, g_evenEm = -1; inline bool g_emActiveHigh = true;
inline bool emOn(int pin) { return g_pinLevel[pin] == (g_emActiveHigh ? HIGH : LOW); }
inline bool emitterOnFor(int i) {
  if (g_commonEm < 0 && g_oddEm < 0 && g_evenEm < 0) return true;                     // без подсветки датчики «работают всегда»
  if (g_commonEm >= 0 && emOn(g_commonEm)) return true;
  if (g_oddEm >= 0 && i % 2 == 0 && emOn(g_oddEm)) return true;                       // первый датчик (индекс 0) считается нечётным
  if (g_evenEm >= 0 && i % 2 == 1 && emOn(g_evenEm)) return true;
  return false;
}
inline void mockReset() { memset(g_pinMode, 0, sizeof g_pinMode); memset(g_pinLevel, 0, sizeof g_pinLevel); memset(g_charged, 0, sizeof g_charged); g_commonEm = g_oddEm = g_evenEm = -1; g_emActiveHigh = true; g_us = 1000000; }
inline void pinMode(uint8_t p, uint8_t m) {
  if (p >= 20 && p < 36 && m == INPUT && g_pinMode[p] == OUTPUT && g_pinLevel[p] == HIGH) { g_charged[p] = true; g_dischargeStart[p] = g_us; }   // RC: конденсатор заряжен, пошёл разряд
  if (m == OUTPUT) g_charged[p] = false;
  g_pinMode[p] = m;
}
inline void digitalWrite(uint8_t p, uint8_t v) { g_pinLevel[p] = v; }
inline bool onLine(int i) {
  float x = (i - (g_floor.n - 1) / 2.0f) * g_floor.pitch;
  return fabsf(x - g_floor.lineCenter) <= g_floor.lineWidth / 2;
}
inline int analogRead(uint8_t pin) {
  Floor &f = g_floor; int i = pin - 20; f.reads++;
  if (f.autoSweep && i == 0) { f.phase++; f.lineCenter = -45 + 90.0f * ((f.phase / 4) % 100) / 99.0f; }
  bool on = emitterOnFor(i);
  if (g_commonEm >= 0 || g_oddEm >= 0 || g_evenEm >= 0) { if (on) f.emitterReads++; }
  if ((g_oddEm >= 0 || g_evenEm >= 0) && g_commonEm < 0) {                            // при чередовании подсвечен только «свой» датчик: соседняя подсветка не должна гореть
    if ((i % 2 == 0 && g_evenEm >= 0 && emOn(g_evenEm)) || (i % 2 == 1 && g_oddEm >= 0 && emOn(g_oddEm))) f.wrongEmitter++;
  }
  bool dark = onLine(i); if (f.whiteLine) dark = !dark;
  float v;
  if (on) v = (dark ? f.dark : f.light) * f.gain[i] + f.shift[i] - f.ambient;
  else v = f.adcMax * 1023.0f / f.adcMax - f.ambient;                                  // без подсветки датчик видит только внешний свет
  f.seed = f.seed * 1664525u + 1013904223u;
  if (f.noise) v += ((int)((f.seed >> 16) % (2 * f.noise + 1)) - f.noise);
  v = v * f.adcMax / 1023.0f;
  return v < 0 ? 0 : (v > f.adcMax ? f.adcMax : (int)v);
}
inline int digitalRead(uint8_t pin) {
  int i = pin - 20;
  if (g_charged[pin]) {                                                                // RC: HIGH, пока конденсатор не разрядился
    g_us += 4;                                                                         // один проход цикла опроса занимает время
    bool dark = onLine(i); if (g_floor.whiteLine) dark = !dark;
    uint32_t decay = (uint32_t)((dark ? g_floor.rcDarkUs : g_floor.rcLightUs) * g_floor.gain[i]);
    return (g_us - g_dischargeStart[pin] >= decay) ? LOW : HIGH;
  }
  bool d = onLine(i); if (g_floor.whiteLine) d = !d; return d ? HIGH : LOW;
}
class Print {
 public:
  std::string text;
  void put(const char *s) { printf("%s", s); text += s; }
  void print(const char *s) { put(s); }
  void print(const __FlashStringHelper *s) { put((const char *)s); }
  void println(const __FlashStringHelper *s) { put((const char *)s); put("\n"); }
  void print(long v) { char b[24]; snprintf(b, sizeof b, "%ld", v); put(b); }
  void println(long v) { print(v); put("\n"); }
  void println(const char *s = "") { put(s); put("\n"); }
};
