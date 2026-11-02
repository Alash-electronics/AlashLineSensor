// [ru] AlashLineSensor: робот едет по чёрной линии. Отклонение линии от центра переводится в поворот (пропорциональный регулятор),
//      при потере линии робот крутится в ту сторону, где линию видели последний раз. Моторы через драйвер L298N / L9110 (две пары пинов ШИМ).
//      Для плавной езды вместо одного коэффициента возьмите AlashPID (регулятор с интегралом и производной).
//      Подключение моторов: левый на пины 5 и 6, правый на 9 и 10 (ШИМ). Линейка как в примере Basic. Поднимите робота колёсами вверх при первой проверке!
// [kk] AlashLineSensor: робот қара сызықпен жүреді. Сызықтың центрден ауытқуы бұрылысқа айналады (пропорционалды реттегіш),
//      сызық жоғалса робот сызық соңғы рет көрінген жаққа бұрылады. Моторлар L298N / L9110 драйвері арқылы (екі ШИМ пин жұбы).
//      Біркелкі жүру үшін бір коэффициенттің орнына AlashPID алыңыз (интегралы және туындысы бар реттегіш).
//      Моторларды қосу: сол жағы 5 және 6 пиндерге, оң жағы 9 және 10-ға (ШИМ). Линейка Basic мысалындағыдай. Алғаш тексергенде роботты дөңгелектерін жоғары қаратып ұстаңыз!
// [en] AlashLineSensor: a robot follows a black line. The line offset from the center becomes a turn (a proportional controller),
//      when the line is lost the robot turns toward the side where it was last seen. Motors through an L298N / L9110 driver (two PWM pin pairs).
//      For smoother driving use AlashPID instead of one gain (a controller with an integral and a derivative).
//      Motor wiring: left on pins 5 and 6, right on 9 and 10 (PWM). The bar as in the Basic example. Hold the robot with its wheels up for the first test!
#include <AlashLineSensor.h>

const uint8_t PINS[5] = {A0, A1, A2, A3, A4};
AlashLineSensor line(PINS, 5);
const uint8_t L1 = 5, L2 = 6, R1 = 9, R2 = 10;       // пины моторов (на ESP32 укажите свои пины и подключите analogWrite)
const int BASE = 150;                                // скорость 0..255
const float KP = 0.08f;                              // сколько единиц поворота на единицу отклонения

void drive(int left, int right) {
  left = constrain(left, -255, 255); right = constrain(right, -255, 255);
  analogWrite(L1, left > 0 ? left : 0);  analogWrite(L2, left < 0 ? -left : 0);
  analogWrite(R1, right > 0 ? right : 0); analogWrite(R2, right < 0 ? -right : 0);
}

void setup() {
  Serial.begin(115200);
  line.begin();
  Serial.println(F("Calibrating: sweep the bar over the line..."));
  line.calibrate(3000);
  delay(1000);
}

void loop() {
  int off = line.offset();                           // -2000 .. +2000, 0 на линии
  if (line.lineLost()) { drive(off > 0 ? BASE : -BASE, off > 0 ? -BASE : BASE); return; }   // искать там, где видели
  int turn = (int)(KP * off);
  drive(BASE + turn, BASE - turn);
}
