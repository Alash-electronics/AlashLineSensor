# AlashLineSensor: руководство / нұсқаулық / guide

[Русский](#русский) | [Қазақша](#қазақша) | [English](#english)

---

## Русский

Простая библиотека для **линейки датчиков линии** (QTR, TCRT5000) робота, который едет по линии. Калибровка за 3 секунды, положение линии одним числом, отклонение от центра для руления, потеря линии и перекрёстки. Чёрная и белая линия, аналоговые и цифровые датчики. Без зависимостей. Сделана для соревнований и уроков робототехники Alash Electronics.

### Установка
Arduino IDE: Скетч, Подключить библиотеку, Управлять библиотеками, найти **AlashLineSensor** и нажать Установить. Или скачайте ZIP с GitHub: Скетч, Подключить библиотеку, Добавить .ZIP библиотеку.

### Платы
Arduino Uno / Nano / Mega (AVR), ESP32, ESP8266 и другие, где работает Arduino-ядро.

### Подключение
| Линейка | Uno / Nano | ESP32 |
|---|---|---|
| VCC | 5 В | 3.3 В (АЦП ESP32 выдерживает 3.3 В) |
| GND | GND | GND |
| OUT 1..5 | A0..A4 | GPIO 36, 39, 34, 35, 32 |
| LEDON (QTR) | любой пин, необязательно | любой пин |

Держите линейку на высоте 3-9 мм от поверхности. У большинства линеек на тёмном число больше; если у вашего модуля наоборот, вызовите `setInvert(true)`. Для ESP32 задайте `setAdcMax(4095)` и питайте линейку от 3.3 В (датчик на 5 В даст ESP32 выше допустимого). У ESP8266 один аналоговый вход: используйте RC-датчики на цифровых пинах (`setSensorType(SENSOR_RC)`), цифровые модули (`setDigital(true)`) или мультиплексор (`setReader()`).

### Быстрый старт
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

### Справочник по методам
| Метод | Что делает |
|---|---|
| `AlashLineSensor(pins, count)` | Линейка: массив пинов и число датчиков (1..16) |
| `begin()` | Настроить пины и память |
| `calibrate(ms) / calibrateStart() + calibrateStep()` | Калибровка водя линейку над линией и фоном |
| `setCalibration(i, min, max)` | Готовые числа калибровки |
| `position() / offset()` | Положение 0..(n-1)*1000 / отклонение от центра |
| `read(out) / readRaw(out)` | Нормированные 0..1000 / сырые значения |
| `lineDetected() / lineLost() / crossing() / allClear()` | Состояния линии |
| `setSensorType(SENSOR_ANALOG / SENSOR_DIGITAL / SENSOR_RC) / setReader(fn)` | Тип датчиков: аналоговые, цифровые, RC или своя функция чтения |
| `setWhiteLine() / setInvert() / setAdcMax()` | Белая линия, полярность, АЦП |
| `setSamples() / setThreshold() / setEmitterPin(s) / setReadMode()` | Усреднение, порог, подсветка и её режимы |
| `error() / errorText() / diagnose(Serial)` | Ошибка, сообщение, самопроверка на ru/kk/en |

### Если что-то не работает
**Положение линии скачет или всегда посередине**

Не выполнена калибровка (`calibrate()`), либо линейка слишком высоко (нужно 3-9 мм). Калибруйте на той поверхности, где поедет робот, водя линейку над линией и фоном. Запустите пример `Diagnose`: он покажет сырые числа каждого датчика.

**Робот едет в обратную сторону от линии**

У вашего модуля обратная полярность (на тёмном число меньше): вызовите `setInvert(true)`. Если линия белая на чёрном, добавьте `setWhiteLine(true)`. Знак поворота в `drive()` тоже проверьте: возможно, моторы подключены наоборот.

**Калибровка не удалась (ERR_FLAT)**

У одного или нескольких датчиков нет разницы между линией и фоном. Во время калибровки водите линейку именно над линией и над фоном, чтобы каждый датчик увидел и то, и другое. Проверьте питание подсветки и высоту.

**На ESP32 показания странные**

АЦП ESP32 12-битный: `setAdcMax(4095)`. Пины должны быть на ADC1 (GPIO 32-39): ADC2 не работает при включённом Wi-Fi. Линейку питайте от 3.3 В.

**Робот не замечает перекрёсток**

Порог по умолчанию 500 из 1000. Если линия блеклая, уменьшите `setThreshold(300)`. Для перекрёстка линия должна быть под (почти) всеми датчиками: на линейке из 3 датчиков это все три, на 6 и более все, кроме одного.

### Самопроверка
В библиотеке есть `diagnose(Serial)`: она печатает, что видит плата, и подсказывает, что проверить. Язык подсказок: `setLanguage(LANG_RU / LANG_KK / LANG_EN)`.

### Примеры
`Basic`, `Calibrate`, `FollowLine`, `RC_Sensors`, `Multiplexer`, `Diagnose`

---

## Қазақша

Сызықпен жүретін робот **сызық датчиктері линейкасына** (QTR, TCRT5000) арналған қарапайым кітапхана. 3 секундта калибрлеу, сызық орны бір санмен, басқару үшін центрден ауытқу, сызықтың жоғалуы және қиылыстар. Қара және ақ сызық, аналогтық және сандық датчиктер. Тәуелділіксіз.

### Орнату
Arduino IDE: Sketch, Include Library, Manage Libraries, **AlashLineSensor** табып, Install басыңыз. Немесе GitHub-тан ZIP жүктеп, Sketch, Include Library, Add .ZIP Library.

### Тақталар
Arduino Uno / Nano / Mega (AVR), ESP32, ESP8266 және Arduino ядросы бар басқалар.

### Қосу
| Линейка | Uno / Nano | ESP32 |
|---|---|---|
| VCC | 5 В | 3.3 В (АЦП ESP32 выдерживает 3.3 В) |
| GND | GND | GND |
| OUT 1..5 | A0..A4 | GPIO 36, 39, 34, 35, 32 |
| LEDON (QTR) | любой пин, необязательно | любой пин |

Линейканы беттен 3-9 мм биіктікте ұстаңыз. Көпшілік линейкада қараңғыда сан үлкен; модульде керісінше болса, `setInvert(true)` шақырыңыз. ESP32 үшін `setAdcMax(4095)` беріңіз және линейканы 3.3 В-тан қоректендіріңіз.

### Жылдам бастау
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

### Әдістер анықтамасы
| Әдіс | Не істейді |
|---|---|
| `AlashLineSensor(pins, count)` | Линейка: пиндер және саны |
| `begin()` | Пиндерді баптау |
| `calibrate(ms) / calibrateStart() + calibrateStep()` | Калибрлеу |
| `setCalibration(i, min, max)` | Дайын калибрлеу сандары |
| `position() / offset()` | Орын / центрден ауытқу |
| `read(out) / readRaw(out)` | Нормаланған / шикі |
| `lineDetected() / lineLost() / crossing() / allClear()` | Сызық күйлері |
| `setSensorType(SENSOR_ANALOG / SENSOR_DIGITAL / SENSOR_RC) / setReader(fn)` | Датчик түрі |
| `setWhiteLine() / setInvert() / setAdcMax()` | Ақ сызық, полярлық, сандық, АЦП |
| `setSamples() / setThreshold() / setEmitterPin(s) / setReadMode()` | Орташалау, шек, жарықтандыру |
| `error() / errorText() / diagnose(Serial)` | Қате, хабарлама, өзін-өзі тексеру |

### Бірдеңе жұмыс істемесе
**Сызық орны секіреді немесе әрқашан ортада**

Калибрлеу орындалмаған (`calibrate()`) немесе линейка тым биік (3-9 мм керек). Робот жүретін бетте, линейканы сызық пен фон үстінде жүргізіп калибрлеңіз. `Diagnose` мысалын іске қосыңыз.

**Робот сызықтан кері жаққа жүреді**

Модульде кері полярлық (қараңғыда сан аз): `setInvert(true)` шақырыңыз. Сызық қарада ақ болса, `setWhiteLine(true)` қосыңыз. `drive()` ішіндегі бұрылыс таңбасын да тексеріңіз: моторлар кері қосылған болуы мүмкін.

**Калибрлеу сәтсіз (ERR_FLAT)**

Бір немесе бірнеше датчикте сызық пен фон арасында айырмашылық жоқ. Калибрлеу кезінде әр датчик екеуін де көруі үшін линейканы дәл сызық пен фон үстінде жүргізіңіз. Жарықтандыру қуаты мен биіктікті тексеріңіз.

**ESP32-де көрсеткіштер біртүрлі**

ESP32 АЦП 12 биттік: `setAdcMax(4095)`. Пиндер ADC1-де болуы керек (GPIO 32-39): Wi-Fi қосулы кезде ADC2 жұмыс істемейді. Линейканы 3.3 В-тан қоректендіріңіз.

**Робот қиылысты байқамайды**

Әдепкі шек 1000-нан 500. Сызық күңгірт болса, `setThreshold(300)` азайтыңыз. Қиылыс үшін сызық (дерлік) барлық датчиктің астында болуы керек: 3 датчикті линейкада үшеуі де, 6 және одан көп болса біреуінен басқасы.

### Өзін-өзі тексеру
Кітапханада `diagnose(Serial)` бар: тақта не көргенін басып шығарады және нені тексеру керегін айтады. Кеңес тілі: `setLanguage(LANG_RU / LANG_KK / LANG_EN)`.

### Мысалдар
`Basic`, `Calibrate`, `FollowLine`, `RC_Sensors`, `Multiplexer`, `Diagnose`

---

## English

A simple library for a **line sensor bar** (QTR, TCRT5000) of a line-following robot. A calibration in 3 seconds, the line position as one number, the offset from the center for steering, line loss and crossings. Black and white lines, analog and digital sensors. No dependencies. Made for competitions and robotics lessons of Alash Electronics.

### Installation
Arduino IDE: Sketch, Include Library, Manage Libraries, search **AlashLineSensor** and click Install. Or download the ZIP from GitHub: Sketch, Include Library, Add .ZIP Library.

### Boards
Arduino Uno / Nano / Mega (AVR), ESP32, ESP8266 and others with an Arduino core.

### Wiring
| Линейка | Uno / Nano | ESP32 |
|---|---|---|
| VCC | 5 В | 3.3 В (АЦП ESP32 выдерживает 3.3 В) |
| GND | GND | GND |
| OUT 1..5 | A0..A4 | GPIO 36, 39, 34, 35, 32 |
| LEDON (QTR) | любой пин, необязательно | любой пин |

Hold the bar 3-9 mm above the surface. Most bars give a larger number on dark; if your module is the other way round, call `setInvert(true)`. For ESP32 give `setAdcMax(4095)` and power the bar from 3.3 V (a 5 V sensor would put more than the allowed voltage on the ESP32).

### Quick start
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

### Method reference
| Method | What it does |
|---|---|
| `AlashLineSensor(pins, count)` | The bar: pin array and sensor count (1..16) |
| `begin()` | Set up pins and memory |
| `calibrate(ms) / calibrateStart() + calibrateStep()` | Calibrate by sweeping the bar over the line and the background |
| `setCalibration(i, min, max)` | Ready calibration numbers |
| `position() / offset()` | Position 0..(n-1)*1000 / offset from the center |
| `read(out) / readRaw(out)` | Normalized 0..1000 / raw values |
| `lineDetected() / lineLost() / crossing() / allClear()` | Line states |
| `setSensorType(SENSOR_ANALOG / SENSOR_DIGITAL / SENSOR_RC) / setReader(fn)` | Sensor type: analog, digital, RC or your own read function |
| `setWhiteLine() / setInvert() / setAdcMax()` | White line, polarity, digital, ADC |
| `setSamples() / setThreshold() / setEmitterPin(s) / setReadMode()` | Averaging, threshold, emitter |
| `error() / errorText() / diagnose(Serial)` | Error, message, self-check in ru/kk/en |

### If something does not work
**The line position jumps or is always in the middle**

The calibration was not done (`calibrate()`) or the bar is too high (3-9 mm is needed). Calibrate on the surface the robot will drive on, sweeping the bar over the line and the background. Run the `Diagnose` example: it shows the raw numbers of every sensor.

**The robot drives away from the line**

Your module has reversed polarity (a smaller number on dark): call `setInvert(true)`. If the line is white on black, add `setWhiteLine(true)`. Also check the sign of the turn in `drive()`: the motors may be wired the other way.

**Calibration failed (ERR_FLAT)**

One or more sensors see no difference between the line and the background. During calibration sweep the bar exactly over the line and the background so every sensor sees both. Check the emitter power and the height.

**On ESP32 the readings are strange**

The ESP32 ADC is 12-bit: `setAdcMax(4095)`. The pins must be on ADC1 (GPIO 32-39): ADC2 does not work while Wi-Fi is on. Power the bar from 3.3 V.

**The robot does not notice a crossing**

The default threshold is 500 of 1000. If the line is faint lower it with `setThreshold(300)`. For a crossing the line must be under (almost) all the sensors: all three on a 3-sensor bar, all but one on 6 or more.

### Self-check
The library has `diagnose(Serial)`: it prints what the board sees and tells what to check. Tips language: `setLanguage(LANG_RU / LANG_KK / LANG_EN)`.

### Examples
`Basic`, `Calibrate`, `FollowLine`, `RC_Sensors`, `Multiplexer`, `Diagnose`

---

MIT License, Alash Electronics. https://github.com/Alash-electronics/AlashLineSensor
