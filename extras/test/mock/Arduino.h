// Имитация Arduino.h и линейки датчиков отражения для тестов AlashLineSensor.
// Линия шириной width мм проходит над датчиками, стоящими с шагом pitch мм; у каждого датчика свой коэффициент усиления и смещение.
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
inline uint32_t g_ms = 1000;
inline uint32_t millis() { return g_ms; }
inline void delay(uint32_t ms) { g_ms += ms; }
inline void delayMicroseconds(unsigned) {}
struct Floor {
  float lineCenter = 0, lineWidth = 12, pitch = 10;     // мм; датчик i стоит в точке (i - (n-1)/2) * pitch
  int n = 5, adcMax = 1023;
  int dark = 900, light = 80;                           // сырое число над линией (тёмное, как у QTR большее) и над фоном (до индивидуальных поправок)
  float gain[16], shift[16];
  bool whiteLine = false, digital = false;
  bool emitterOn = false; bool emitterNeeded = false;
  int emitterReads = 0, reads = 0;
  int noise = 0;                                        // амплитуда шума
  uint32_t seed = 1;
  bool autoSweep = false; int phase = 0;                // линия сама ходит над датчиками (для блокирующей калибровки)
  Floor() { for (int i = 0; i < 16; i++) { gain[i] = 1; shift[i] = 0; } }
};
inline Floor g_floor;
inline int g_emitterPin = -1; inline bool g_emitterActiveHigh = true;
inline void pinMode(uint8_t, uint8_t) {}
inline void digitalWrite(uint8_t p, uint8_t v) { if ((int)p == g_emitterPin) g_floor.emitterOn = (v == HIGH) == g_emitterActiveHigh; }
inline bool onLine(int i) {
  float x = (i - (g_floor.n - 1) / 2.0f) * g_floor.pitch;
  return fabsf(x - g_floor.lineCenter) <= g_floor.lineWidth / 2;
}
inline int analogRead(uint8_t pin) {
  Floor &f = g_floor; int i = pin - 20; f.reads++;
  if (f.autoSweep && i == 0) { f.phase++; f.lineCenter = -45 + 90.0f * ((f.phase / 4) % 100) / 99.0f; }
  if (g_emitterPin >= 0) { if (f.emitterOn) f.emitterReads++; }
  bool dark = onLine(i); if (f.whiteLine) dark = !dark;
  float v = (dark ? f.dark : f.light) * f.gain[i] + f.shift[i];
  f.seed = f.seed * 1664525u + 1013904223u;
  if (f.noise) v += ((int)((f.seed >> 16) % (2 * f.noise + 1)) - f.noise);
  v = v * f.adcMax / 1023.0f;
  return v < 0 ? 0 : (v > f.adcMax ? f.adcMax : (int)v);
}
inline int digitalRead(uint8_t pin) { int i = pin - 20; bool d = onLine(i); if (g_floor.whiteLine) d = !d; return d ? HIGH : LOW; }
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
