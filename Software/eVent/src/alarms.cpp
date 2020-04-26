#include "pins.h"
#include <Arduino.h>
#include "globalFlags.h"
#include "modes.h"
#include "states.h"
#include "alarms.h"

const int pressurePlateauMax = 30;  // The maximum plateau pressure to start the alarm.
const int pressureMax = 40;         // [mBar] The maximum pressure for inhale to start the alarm.
const int pressureMin = 5;          // [mBar] The minimum pressure for inhale to start the alarm. (Leakage)
const int minVolumeperMinute = 3.0; // Liters

unsigned long timerBlink;

void handleAlarms(int mode, int pressurePlateau, int pressure, int position,
                  float volumePerMiunute) {   // volumePerMiunute in L/min
  // safety functions -----------------------------------------------------------
  // min volume in pressure mode
  if(volumePerMiunute < minVolumeperMinute && mode == MODE_PRESSURE){
    VOLUME_FLAG = true;
  }
  else{
    VOLUME_FLAG = false;
  }

  //pressure
  PRESSURE_FLAG = ((pressure > pressureMax) || (pressurePlateau > pressurePlateauMax)
                   || (pressurePlateau < pressureMin));

  // alarm handling--------------------------------------------------------------

  if(CHANGES_FLAG){
    if(millis() > timerBlink){
      digitalWrite(LCD_BACKLIGHT, !digitalRead(LCD_BACKLIGHT));
      timerBlink = millis() + 500;
    }
  }

  if((PRESSURE_FLAG || VOLUME_FLAG)){
    if(!SILENCE_FLAG){
      tone(BUZZER, 10000, 500);
    }
    digitalWrite(LED_RED, HIGH);
  }
  else{
    noTone(BUZZER);
    digitalWrite(LED_RED, LOW);
  }

  // avoid overflow of millis
  if(millis() > 4233600000){        // 49days
    digitalWrite(RESET_PIN, LOW);
  }
}
