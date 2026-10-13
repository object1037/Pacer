/*
 * Arduino-style tone() demo for a piezo buzzer on CH32V003.
 *
 * Connect the buzzer between PD4 and GND (active buzzers may need a
 * series resistor; passive piezo discs work directly).
 */

#include "ch32fun.h"
#include <stdio.h>

#define TONE_IMPLEMENTATION
#include "tone.h"

#define BUZZER_PIN PD4

int main() {
  SystemInit();
  funGpioInitAll();

  printf("tone() demo\n");

  while (1) {
    tone(BUZZER_PIN, 262, 200); // C4
    Delay_Ms(250);
    tone(BUZZER_PIN, 294, 200); // D4
    Delay_Ms(250);
    tone(BUZZER_PIN, 330, 200); // E4
    Delay_Ms(250);
    tone(BUZZER_PIN, 349, 200); // F4
    Delay_Ms(250);
    tone(BUZZER_PIN, 392, 200); // G4
    Delay_Ms(250);
    tone(BUZZER_PIN, 440, 200); // A4
    Delay_Ms(250);
    tone(BUZZER_PIN, 494, 200); // B4
    Delay_Ms(250);
    tone(BUZZER_PIN, 523, 400); // C5
    Delay_Ms(500);

    tone_continuous(BUZZER_PIN, 880);
    Delay_Ms(500);
    noTone(BUZZER_PIN);
    Delay_Ms(500);
  }
}
