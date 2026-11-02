# AlashLineSensor

[Русский](#русский) | [Қазақша](#қазақша) | [English](#english)

---

## Русский

Простая библиотека для **линейки датчиков линии** (QTR, TCRT5000) робота, который едет по линии. Калибровка за 3 секунды, положение линии одним числом, отклонение от центра для руления, потеря линии и перекрёстки. Чёрная и белая линия, аналоговые и цифровые датчики. Без зависимостей. Сделана для соревнований и уроков робототехники Alash Electronics.

```cpp
#include <AlashLineSensor.h>
const uint8_t PINS[5] = {A0, A1, A2, A3, A4};
AlashLineSensor line(PINS, 5);

void setup() { line.begin(); line.calibrate(3000); }   // водите над линией и фоном
void loop() {
  int off = line.offset();            // -2000 .. +2000, 0 на линии
  // повернуть робота на off (см. пример FollowLine)
}
```

### Что умеет
- **`position()` и `offset()`:** положение линии 0..(n-1)*1000 по взвешенному среднему, отклонение от центра для руления.
- **Калибровка за 3 секунды:** `calibrate()` или `calibrateStep()` в своём цикле; готовые числа `setCalibration()` для скетча.
- **Потеря линии:** `lineLost()`, и `position()` возвращает сторону, где линию видели последний раз.
- **Перекрёстки:** `crossing()`, `activeCount()` для остановки и счёта полос.
- **Чёрная и белая линия, обратная полярность:** `setWhiteLine()`, `setInvert()`.
- **Любые датчики:** аналоговые, цифровые (с компаратором), RC (QTR-RC, на обычных цифровых пинах) и своя функция чтения для мультиплексора CD4051, ADS1115 и т. п.: `setSensorType()`, `setReader()`, `setAdcMax()`.
- **Подсветка:** общая или отдельно нечётная и чётная, активный HIGH или LOW, режимы `READ_ON`, `READ_OFF`, `READ_ON_AND_OFF` (вычитание внешнего света), `READ_ODD_EVEN`.
- **Меньше шума:** усреднение `setSamples()` и порог `setThreshold()`.
- **Видно, что происходит:** `diagnose(Serial)` показывает сырые, нормированные значения и подсказывает (высота, освещение, питание) на трёх языках.
- Работает на **Arduino AVR, ESP32, ESP8266**.

### Подключение
| Линейка | Uno / Nano | ESP32 |
|---|---|---|
| VCC | 5 В | 3.3 В (АЦП ESP32 выдерживает 3.3 В) |
| GND | GND | GND |
| OUT 1..5 | A0..A4 | GPIO 36, 39, 34, 35, 32 |
| LEDON (QTR) | любой пин, необязательно | любой пин |

Держите линейку на высоте 3-9 мм от поверхности. У большинства линеек на тёмном число больше; если у вашего модуля наоборот, вызовите `setInvert(true)`. Для ESP32 задайте `setAdcMax(4095)` и питайте линейку от 3.3 В (датчик на 5 В даст ESP32 выше допустимого). У ESP8266 один аналоговый вход: используйте RC-датчики на цифровых пинах (`setSensorType(SENSOR_RC)`), цифровые модули (`setDigital(true)`) или мультиплексор (`setReader()`).

### Чем отличается от других библиотек
Мы изучили QTRSensors (Pololu), Arduino-Line-Follower и библиотеки для TCRT5000:
- QTRSensors для линеек Pololu (RC-режим или аналоговый), большая, под лицензией LGPL / Pololu, не форкается в MIT; у нас короткая библиотека для любых аналоговых и цифровых линеек.
- Нет подсказок на русском и казахском и нет самопроверки; у нас `errorText()` и `diagnose()` на трёх языках.
- В большинстве примеров по TCRT5000 порог зашит числом; у нас калибровка и нормировка 0..1000.
- Нет запоминания стороны потери линии; у нас `position()` возвращает последнюю сторону.

### Примеры
`Basic`, `Calibrate`, `FollowLine`, `RC_Sensors`, `Multiplexer`, `Diagnose`

### Тесты
Без железа (с AddressSanitizer) на имитации линейки над полом с линией: калибровка с разными усилениями датчиков, нормировка 0..1000, положение линии растёт монотонно и точно в центре и по краям, потеря линии с запоминанием стороны, перекрёсток для 5 и 8 датчиков, белая линия, обратная полярность, шум и усреднение, АЦП на 12 бит, цифровые датчики, подсветка включается только на время измерения, неудачная калибровка и ручные числа, ошибки и диагностика: `extras/test`.

Подробное руководство: [docs/WIKI.md](docs/WIKI.md).

---

## Қазақша

Сызықпен жүретін робот **сызық датчиктері линейкасына** (QTR, TCRT5000) арналған қарапайым кітапхана. 3 секундта калибрлеу, сызық орны бір санмен, басқару үшін центрден ауытқу, сызықтың жоғалуы және қиылыстар. Қара және ақ сызық, аналогтық және сандық датчиктер. Тәуелділіксіз.

```cpp
#include <AlashLineSensor.h>
const uint8_t PINS[5] = {A0, A1, A2, A3, A4};
AlashLineSensor line(PINS, 5);

void setup() { line.begin(); line.calibrate(3000); }   // водите над линией и фоном
void loop() {
  int off = line.offset();            // -2000 .. +2000, 0 на линии
  // повернуть робота на off (см. пример FollowLine)
}
```

### Мүмкіндіктері
- **`position()` және `offset()`:** салмақталған орта бойынша 0..(n-1)*1000 сызық орны, басқару үшін центрден ауытқу.
- **3 секундта калибрлеу:** `calibrate()` немесе өз циклыңызда `calibrateStep()`; скетч үшін дайын сандар `setCalibration()`.
- **Сызықтың жоғалуы:** `lineLost()`, және `position()` сызық соңғы рет көрінген жақты қайтарады.
- **Қиылыстар:** `crossing()`, тоқтату және жолақтарды санау үшін `activeCount()`.
- **Қара және ақ сызық, кері полярлық:** `setWhiteLine()`, `setInvert()`.
- **Кез келген датчиктер:** аналогтық, сандық (компараторлы), RC (QTR-RC, қарапайым сандық пиндерде) және CD4051 мультиплексоры, ADS1115 т.б. үшін өз оқу функциясы: `setSensorType()`, `setReader()`, `setAdcMax()`.
- **Жарықтандыру:** ортақ немесе жеке тақ және жұп, белсенді HIGH немесе LOW, `READ_ON`, `READ_OFF`, `READ_ON_AND_OFF` (сыртқы жарықты алу), `READ_ODD_EVEN` режимдері.
- **Шу аз:** `setSamples()` орташалау және `setThreshold()` шегі.
- **Не болып жатқаны көрінеді:** `diagnose(Serial)` шикі, нормаланған мәндерді көрсетеді және үш тілде кеңес береді (биіктік, жарықтандыру, қуат).
- **Arduino AVR, ESP32, ESP8266** тақталарында жұмыс істейді.

### Қосу
| Линейка | Uno / Nano | ESP32 |
|---|---|---|
| VCC | 5 В | 3.3 В (АЦП ESP32 выдерживает 3.3 В) |
| GND | GND | GND |
| OUT 1..5 | A0..A4 | GPIO 36, 39, 34, 35, 32 |
| LEDON (QTR) | любой пин, необязательно | любой пин |

Линейканы беттен 3-9 мм биіктікте ұстаңыз. Көпшілік линейкада қараңғыда сан үлкен; модульде керісінше болса, `setInvert(true)` шақырыңыз. ESP32 үшін `setAdcMax(4095)` беріңіз және линейканы 3.3 В-тан қоректендіріңіз.

### Басқа кітапханалардан айырмашылығы
QTRSensors (Pololu), Arduino-Line-Follower және TCRT5000 кітапханаларын қарадық:
- QTRSensors Pololu линейкаларына арналған (RC режимі немесе аналогтық), үлкен, LGPL / Pololu лицензиясымен, MIT-ке форк жасалмайды; бізде кез келген аналогтық және сандық линейкаға арналған қысқа кітапхана.
- Орысша және қазақша кеңестер мен өзін-өзі тексеру жоқ; бізде үш тілде `errorText()` және `diagnose()`.
- TCRT5000 мысалдарының көпшілігінде шек санмен жазылған; бізде калибрлеу және 0..1000 нормалау.
- Сызық жоғалған жақты есте сақтау жоқ; бізде `position()` соңғы жақты қайтарады.

### Мысалдар
`Basic`, `Calibrate`, `FollowLine`, `RC_Sensors`, `Multiplexer`, `Diagnose`

### Тесттер
Құрылғысыз (AddressSanitizer-мен) сызығы бар едендегі линейка имитациясында: датчиктердің әртүрлі күшейтуімен калибрлеу, 0..1000 нормалау, сызық орны монотонды өседі және ортада мен шеттерде дәл, жақты есте сақтап сызықтың жоғалуы, 5 және 8 датчик үшін қиылыс, ақ сызық, кері полярлық, шу және орташалау, 12 биттік АЦП, сандық датчиктер, жарықтандыру тек өлшеу кезінде қосылады, сәтсіз калибрлеу, қателер: `extras/test`.

Толық нұсқаулық: [docs/WIKI.md](docs/WIKI.md).

---

## English

A simple library for a **line sensor bar** (QTR, TCRT5000) of a line-following robot. A calibration in 3 seconds, the line position as one number, the offset from the center for steering, line loss and crossings. Black and white lines, analog and digital sensors. No dependencies. Made for competitions and robotics lessons of Alash Electronics.

```cpp
#include <AlashLineSensor.h>
const uint8_t PINS[5] = {A0, A1, A2, A3, A4};
AlashLineSensor line(PINS, 5);

void setup() { line.begin(); line.calibrate(3000); }   // водите над линией и фоном
void loop() {
  int off = line.offset();            // -2000 .. +2000, 0 на линии
  // повернуть робота на off (см. пример FollowLine)
}
```

### Features
- **`position()` and `offset()`:** the line position 0..(n-1)*1000 by a weighted average, the offset from the center for steering.
- **Calibration in 3 seconds:** `calibrate()` or `calibrateStep()` in your own loop; ready numbers via `setCalibration()` for the sketch.
- **Line loss:** `lineLost()`, and `position()` returns the side where the line was last seen.
- **Crossings:** `crossing()`, `activeCount()` for stopping and counting stripes.
- **Black and white lines, reversed polarity:** `setWhiteLine()`, `setInvert()`.
- **Any sensors:** analog, digital (with a comparator), RC (QTR-RC, on ordinary digital pins) and your own read function for a CD4051 multiplexer, an ADS1115 and the like: `setSensorType()`, `setReader()`, `setAdcMax()`.
- **Emitter:** common or separate odd and even, active HIGH or LOW, the modes `READ_ON`, `READ_OFF`, `READ_ON_AND_OFF` (ambient light subtraction), `READ_ODD_EVEN`.
- **Less noise:** `setSamples()` averaging and the `setThreshold()` threshold.
- **See what happens:** `diagnose(Serial)` shows the raw and normalized values and gives tips (height, lighting, power) in three languages.
- Works on **Arduino AVR, ESP32, ESP8266**.

### Wiring
| Линейка | Uno / Nano | ESP32 |
|---|---|---|
| VCC | 5 В | 3.3 В (АЦП ESP32 выдерживает 3.3 В) |
| GND | GND | GND |
| OUT 1..5 | A0..A4 | GPIO 36, 39, 34, 35, 32 |
| LEDON (QTR) | любой пин, необязательно | любой пин |

Hold the bar 3-9 mm above the surface. Most bars give a larger number on dark; if your module is the other way round, call `setInvert(true)`. For ESP32 give `setAdcMax(4095)` and power the bar from 3.3 V (a 5 V sensor would put more than the allowed voltage on the ESP32).

### How it differs from other libraries
We studied QTRSensors (Pololu), Arduino-Line-Follower and TCRT5000 libraries:
- QTRSensors is for Pololu bars (RC mode or analog), large, under an LGPL / Pololu license, not forkable into MIT; ours is a short library for any analog and digital bar.
- No tips in Russian and Kazakh and no self-check; ours has `errorText()` and `diagnose()` in three languages.
- Most TCRT5000 examples hard-code a threshold number; ours has calibration and 0..1000 normalization.
- No memory of the side where the line was lost; ours `position()` returns the last side.

### Examples
`Basic`, `Calibrate`, `FollowLine`, `RC_Sensors`, `Multiplexer`, `Diagnose`

### Tests
Without hardware (with AddressSanitizer) on a simulated bar over a floor with a line: calibration with different sensor gains, 0..1000 normalization, the line position grows monotonically and is exact at the center and the edges, line loss with a remembered side, a crossing for 5 and 8 sensors, a white line, reversed polarity, noise and averaging, a 12-bit ADC, digital sensors, the emitter lit only during a measurement, a failed calibration and manual numbers, errors and diagnostics: `extras/test`.

Full guide: [docs/WIKI.md](docs/WIKI.md).

---

MIT License, Alash Electronics.
