#include <Arduino.h>
#include "pins.h"
#include "states.h"
#include "display.h"
#include "motor.h"
#include "modes.h"
#include "globalFlags.h"
#include "../lib/Encoder_ID129/Encoder.h"
#include "alarms.h"

//for Arduino MEGA with compatibility to UNO for essential functions

bool PRESSURE_FLAG;
bool CHANGES_FLAG;
bool MOTOR_FLAG;
bool START_UP_FLAG;
bool SILENCE_FLAG;
bool MOTOR_ACTIVE_FLAG;
bool VOLUME_FLAG;


const int diffBagPosition = 60;     //ticks between homePosition and fingers touching the bag (HARDCODED)
const int timeHold = 200;           //The amount of time (in milliseconds) to hold the compression at the end of the inhale for plateau pressure
const int velocityHoming = 70;      //slow speed for homing
const int controlPeriod = 50;       //ms
const int closePosition = 190;      //Ticks between home and full close

//state machine variables
int lastMode;
int mode;                 //switch state variable for outer switch
int nextMode;             //mode to be executed after homing
int state;                //switch state variable for inner switch

//Encoder variables
int homePosition;         //Encoder value at the position of the limit switch
int currentPosition;
int bagPosition;          //absolute positions where fingers touching the bag
int inhaleEndPosition;    //calculated absolute position at the end of the inspiratory phase
int prePeriodPosition;    //ticks before the current velocity control period
int preInhalePosition;

//In/Exhale times
int targetTimeInhale;     //The length of time (in ms) of the inspiratory phase.
int targetTimeExhale;     //The length of time (in ms) of the expiratory phase.

//volume parameters
int volumeTidal;          //The total volume of air to be delivered to the patient
int volumeTicks;          //The number of Encoder ticks required for the given volumeTidal
int realVolume;
int realVolumeTicks;

//rate and ratio
int breathsPerMinute;     // Breaths per minute, also called respiratory rate (RR). Typically varies between 8-30 BPM.
float I2E_ratio;          // The ratio of the duration of the inhale to the duration of the exhale.

//velocity control
float targetInhaleVelocity; //target input of controller
float targetExhaleVelocity; //target input of controller
int currentVelocity;        //Measure Input of controller.
int dutyCycleOutput;       //Output of controller. The inhale velocity scaled from 0-255 for PWM Output
int integral;
float iTerm;   //integral term of PI controller
float pTerm;

//flow
int currentFlow;        //ml/s
int targetFlow;
int meanFlow;
int maxFlow;

//pressures
int pressurePlateau;      //Last measured plateauPressure in mbar
int limitPressure;        //maximum presssure for pressure limited mode

//variables for setpoint potentiometers
int volumeTidalSet;       //Current potentiometer value for volumeTidal, not confirmed
int breathsPerMinuteSet;  //Current potentiometer value for breathsPerMinute, not confirmed
float I2E_ratioSet;       //Current potentiometer value for I2E_ratio, not confirmed
int limitPressureSet;     //Current potentiometer value for limitPressure, not confirmed
int lastVolumeTidalSet;       //last potentiometer value for volumeTidal, not confirmed
int lastBreathsPerMinuteSet;  //last potentiometer value for breathsPerMinute, not confirmed
float lastI2E_ratioSet;       //last potentiometer value for I2E_ratio, not confirmed
int lastLimitPressureSet;//last potentiometer value for limitPressure, not confirmed



//timers in ms
unsigned long timerSetPoint;
unsigned long timerPotentiometer;
unsigned long timerDebounce;
unsigned long timerDisplay;
unsigned long timerControlPeriod;
unsigned long timerInhale;
unsigned long timerExhale;
unsigned long timerDelay;
unsigned long timerRealInhale;
unsigned long timerRealExhale;
unsigned long timerSilence;
unsigned long timerAlarmsInterrupt;

int currentMeanFlow;

int controlCounter;
long plottingCounter;

//encoder instance
Encoder encoder(ENC_A,ENC_B);

void calculateVolumeParameters(){
  int singleBreathTime =60000/breathsPerMinute - timeHold;
  targetTimeInhale = singleBreathTime/(1.0/I2E_ratio);
  targetTimeExhale = singleBreathTime -targetTimeInhale;
  volumeTicks = (volumeTidal+420)/12; //linear model y=12*x-420
  inhaleEndPosition = bagPosition - volumeTicks;
  meanFlow = (volumeTicks/(targetTimeInhale/1000.0))*12;
  targetExhaleVelocity = volumeTicks/(targetTimeExhale/1000.0);
}

int getPressure(){

  int readingA = analogRead(PRESSURE);
  delay(5);
  int readingB = analogRead(PRESSURE);
  delay(5);
  int readingC = analogRead(PRESSURE);
  return map((readingA+readingB+readingC)/3, 40, 963, 0, 100); // from 0.2V (41) to 3.7V (963) to a range of 0 to 100mbar
}


void controlInhaleVelocity(){

  //calculate ticks in last period
  int ticksInLastPeriod = prePeriodPosition-currentPosition;
  //calculate current velocity in Ticks/s
  currentVelocity =  ticksInLastPeriod * (1000/controlPeriod); //current velocity in Ticks/s

  //flow calculation
  //two seperate linear apprxomiations are used <60 or >60
  //calculate current flow in ml/s
  if((bagPosition-prePeriodPosition) < 60){
    currentFlow = currentVelocity*8;
  }
  else{
    currentFlow = currentVelocity*12;
  }
  currentMeanFlow = (currentMeanFlow*0.6+currentFlow*0.4);

  if(mode == MODE_VOLUME){
    //calculate target flow
    int inhaleControlCycles = targetTimeInhale/controlPeriod;
    if(controlCounter < inhaleControlCycles/2){
      targetFlow = meanFlow * ((1.1*controlCounter)/(inhaleControlCycles/2.0)+0.2);      //1.1 -> 110% difference between 20% at begin and 130% at the end
    }
    else{
      targetFlow = 1.3*meanFlow;            //130% flow in second half to reach mean flow
    }

  }

if((bagPosition-currentPosition) < 60){
  targetInhaleVelocity = targetFlow/8;
}
else{
  targetInhaleVelocity = targetFlow/12;
}
  //controller
  int controlDifference = targetInhaleVelocity - currentVelocity;
  //define P term of controller
  if(controlDifference < 0){
    pTerm = 0.001;
  }
  else{
    pTerm = 0.07;
  }
  iTerm = 0.001;
  integral += controlDifference;

  //new inhalleVelocity
  dutyCycleOutput = dutyCycleOutput + controlDifference*pTerm + integral*iTerm;
  //limit for inhale velocity
  if(dutyCycleOutput > 255){
    dutyCycleOutput=255;
  }
  //set parameters for next control cycle
  controlCounter++;
  prePeriodPosition = currentPosition;
}

void controlExhaleVelocity(){

  //calculate ticks in last period
  int ticksInLastPeriod = currentPosition - prePeriodPosition;
  //calculate current velocity in Ticks/s
  currentVelocity =  ticksInLastPeriod * (1000/controlPeriod); //current velocity in Ticks/s

  //controller
  int controlDifference = targetExhaleVelocity -currentVelocity;

  if(controlDifference < 0){
    pTerm = 0.001;
  }
  else{
    pTerm = 0.08;
  }
  iTerm = 0.0001;
  integral += controlDifference;

  dutyCycleOutput = dutyCycleOutput + controlDifference*pTerm + integral*iTerm;

  if(dutyCycleOutput < 35){
    dutyCycleOutput = 35;
  }
  //limit for inhale velocity
  if(dutyCycleOutput > 40){
    dutyCycleOutput=40;
  }
}

void controlPressure(){

  int currentPressure = getPressure();
  int controlDifference = limitPressure - currentPressure;
  integral += controlDifference;
  pTerm = 2;
  if(controlCounter < 4){
    integral = 0;
  }
  iTerm = 0.1;

  //new inhalleVelocity
  dutyCycleOutput = dutyCycleOutput + controlDifference*pTerm + integral*iTerm;
  //limit for inhale velocity
  if(dutyCycleOutput > 255){
    dutyCycleOutput=255;
  }
  if(currentPosition < (homePosition - closePosition)){
    timerInhale = millis()-1; //stops
  }

  //set parameters for next control cycle
  controlCounter++;
  prePeriodPosition = currentPosition;
}

void timer1_setup(long time_in_ms) {
   // initialize timer1
  noInterrupts();           // disable all interrupts
  TCCR1A = 0;
  TCCR1B = 0;
  TCNT1  = 0;
  unsigned long long match_register_value = 62500*(time_in_ms/1000.0); // compare match register 16MHz/256/xHz
  OCR1A = match_register_value;            // compare match register 16MHz/256/x Hz
  //OCR1A = 31250;
  TCCR1B |= (1 << WGM12);   // CTC mode
  TCCR1B |= (1 << CS12);    // 256 prescaler
  TIMSK1 |= (1 << OCIE1A);  // enable timer compare interrupt
  interrupts();             // enable all interrupts
}

ISR(TIMER1_COMPA_vect)          // timer1 interrupt service routine
{
  handleAlarms(mode, pressurePlateau, getPressure(), encoder.read(), (realVolume/1000.0*breathsPerMinute));
}

void setup() {
  pinMode(RESET_PIN, OUTPUT);
  digitalWrite(RESET_PIN, HIGH);
  pinMode(PRESSURE, INPUT);
  pinMode(LIMIT_SWITCH, INPUT);
  pinMode(VOL_POT, INPUT);
  pinMode(RATE_POT, INPUT);
  pinMode(I2E_POT, INPUT);
  pinMode(PRS_POT, INPUT);
  pinMode(MODE_SWITCH, INPUT_PULLUP);
  pinMode(SILENCE_BUTTON, INPUT_PULLUP);
  pinMode(CONFIRM_BUTTON, INPUT_PULLUP);
  pinMode(BUZZER, OUTPUT);
  pinMode(MOTOR_DIR, OUTPUT);
  pinMode(MOTOR_PWM, OUTPUT);
  pinMode(MOTOR_BRAKE, OUTPUT);
  pinMode(MOTOR_SENSE, INPUT);
  pinMode(LCD_BACKLIGHT, OUTPUT);
  pinMode(ENC_A, INPUT_PULLUP);
  pinMode(ENC_B, INPUT_PULLUP);
  pinMode(ENC_I, INPUT_PULLUP);
  pinMode(LED_POWER, OUTPUT);
  pinMode(LED_GREEN, OUTPUT);
  pinMode(LED_RED, OUTPUT);

  START_UP_FLAG = true;

  //debugging serial
  Serial1.begin(9600);
  //lcd
  initLCD();
  homeScreen();
  digitalWrite(LCD_BACKLIGHT, HIGH);

  //test outputs
  digitalWrite(LED_POWER, HIGH);
  digitalWrite(LED_GREEN, HIGH);
  digitalWrite(LED_RED, HIGH);
  tone(BUZZER, 10000);
  delay(1500);
  digitalWrite(LED_POWER, LOW);
  digitalWrite(LED_GREEN, LOW);
  digitalWrite(LED_RED, LOW);
  noTone(BUZZER);

  //inital states
  lastMode = MODE_IDLE;
  mode = MODE_IDLE;
  nextMode = MODE_IDLE;
  state = 0;
  realVolume = 500;
  pressurePlateau = 5;

  timer1_setup(250); // perform safety and alarn function every 250ms
  digitalWrite(LED_POWER, HIGH);    //LED to show that arduino is running
}

void loop() {
  switch (mode) {
    case MODE_IDLE:{
      digitalWrite(LED_GREEN, LOW);
      break;
    }

    case MODE_HOMING:{
      digitalWrite(LED_GREEN, HIGH);
      switch (state) {
        //prehome
        case 0:{
          //move outward slowly
          moveMotor(DIRECTION_OPEN, velocityHoming, MOTOR_FLAG);

          if(digitalRead(LIMIT_SWITCH) == LIMIT_REACHED){
            stopMotor();
            timerDebounce = millis() + 100;
            state++;
          }
          break;
        }
        //homing
        case 1:{
          //move forward slowly, zero encoder
          moveMotor(DIRECTION_CLOSE, velocityHoming, MOTOR_FLAG);

          if((digitalRead(LIMIT_SWITCH)!=LIMIT_REACHED) && (millis()>timerDebounce)){
            stopMotor();
            tone(BUZZER, 20000, 500);
            delay(1000);
            homePosition = encoder.read();
            bagPosition = homePosition - diffBagPosition;
            state++;
          }
          break;
        }
        //posthome
        case 2:
          moveMotor(DIRECTION_CLOSE, velocityHoming, MOTOR_FLAG);
          currentPosition = encoder.read();
          if(currentPosition <= bagPosition){
            stopMotor();
            calculateVolumeParameters();
            state = 0;
            mode = nextMode;
          }
        break;
      }
      break;
    }

    case MODE_VOLUME:{
        digitalWrite(LED_GREEN, HIGH);
        switch (state) {
          //start homing
          case 0:{
            if(nextMode != MODE_VOLUME){
              nextMode = MODE_VOLUME;
              mode = MODE_HOMING;
            }
            else{
              state++;
              prePeriodPosition = encoder.read();
              timerRealInhale = millis();
              timerControlPeriod = millis() + controlPeriod;
              controlCounter = 0;
              integral=0;
              dutyCycleOutput = 60;
            }
            break;
          }
          //inhale
          case 1:{
            moveMotor(DIRECTION_CLOSE, dutyCycleOutput, MOTOR_FLAG);
            currentPosition = encoder.read();
            if(millis() > timerControlPeriod){
              controlInhaleVelocity();
              timerControlPeriod = millis() + controlPeriod;
            }
            if(currentPosition <= inhaleEndPosition){
              stopMotor();
              timerRealInhale = millis()-timerRealInhale;
              timerSetPoint = millis() + timeHold;
              delay(10);
              pressurePlateau = getPressure();
              state++;
            }
            break;
          }
          //pause
          case 2:{
            if(millis() >= timerSetPoint){
              state++;
              prePeriodPosition = encoder.read();
              timerRealExhale = millis();
              timerControlPeriod = millis() + controlPeriod;
              controlCounter = 0;
              integral = 0;
              dutyCycleOutput = 18;
            }
            break;
          }
          //exhale
          case 3:{
            moveMotor(DIRECTION_OPEN, dutyCycleOutput,MOTOR_FLAG);
            currentPosition = encoder.read();
            if(millis() > timerControlPeriod){
              controlExhaleVelocity();
              prePeriodPosition = currentPosition;
              timerControlPeriod = millis() + controlPeriod;
            }
            if(currentPosition >= bagPosition){
              stopMotor();
              timerRealExhale = millis()-timerRealExhale;
              int totalTime = timerRealInhale + timerRealExhale + timeHold;
              if(totalTime < 60000/breathsPerMinute){
                int delay =  60000/breathsPerMinute - totalTime;
                timerDelay = millis() + delay;
              }
              state++;
            }
          break;
          }
          case 4: {
            if(millis() > timerDelay){
              nextMode = MODE_VOLUME;
              state = 0;
            }
            break;
          }
        }
      break;
    }

    case MODE_PRESSURE:{
      digitalWrite(LED_GREEN, HIGH);
      switch (state) {
        //homing
        case 0: {
          if(nextMode != MODE_PRESSURE){
            nextMode = MODE_PRESSURE;
            mode = MODE_HOMING;
          }
          else{
            prePeriodPosition = encoder.read();
            timerInhale = millis() + targetTimeInhale;
            timerControlPeriod = millis() + controlPeriod;
            timerRealInhale = millis();
            controlCounter = 0;
            integral=0;
            dutyCycleOutput = 80;
            state++;
          }
          break;
        }
        //inhale
        case 1: {
          moveMotor(DIRECTION_CLOSE, dutyCycleOutput, MOTOR_FLAG);
          currentPosition = encoder.read();
          if(millis() > timerControlPeriod){
            controlPressure();
            timerControlPeriod = millis() + controlPeriod;
          }
          if(millis() > timerInhale){
            stopMotor();
            timerRealInhale = millis()-timerRealInhale;
            timerSetPoint = millis() + timeHold;
            //calculate applied volume
            realVolumeTicks = bagPosition - currentPosition;

            if(realVolumeTicks < 60){

              realVolume = realVolumeTicks*8 -190;
            }
            else{
              realVolume = realVolumeTicks*12 -420;
            }
            state++;
          }
          break;
        }
        //hold
        case 2: {
          if(millis() >= timerSetPoint){
            state++;
            prePeriodPosition = encoder.read();
            timerRealExhale = millis();
            timerControlPeriod = millis() + controlPeriod;
            controlCounter = 0;
            integral = 0;
            dutyCycleOutput = 18;
          }
          break;
        }
        //exhale
        case 3: {
          moveMotor(DIRECTION_OPEN, dutyCycleOutput,MOTOR_FLAG);
          currentPosition = encoder.read();
          if(millis() > timerControlPeriod){
            controlExhaleVelocity();
            prePeriodPosition = currentPosition;
            timerControlPeriod = millis() + controlPeriod;
          }
          if(currentPosition >= bagPosition){
            stopMotor();
            timerRealExhale = millis()-timerRealExhale;
            int totalTime = timerRealInhale + timerRealExhale + timeHold;
            if(totalTime < 60000/breathsPerMinute){
              int delay =  60000/breathsPerMinute - totalTime;
              timerDelay = millis() + delay;
            }
            state++;
          }
          break;
        }
        //delay
        case 4: {
          if(millis() > timerDelay){
            nextMode = MODE_PRESSURE;
            state = 0;
          }
          break;
        }
      }
      break;
    }

  }

  //input handling --------------------------------------------------------------
  if(millis() >= timerPotentiometer){
  //potentiometer reads
    volumeTidalSet = map(analogRead(VOL_POT),0,1020,900,300);
    breathsPerMinuteSet = map(analogRead(RATE_POT),0,1020,40,8);
    limitPressureSet = map(analogRead(PRS_POT),0,1020,30,10);
    int I2E_ratioMeasure = analogRead(I2E_POT);

    if(I2E_ratioMeasure < 256){
      I2E_ratioSet = 0.2;
    }
    else if(I2E_ratioMeasure < 512){
      I2E_ratioSet = 0.25;
    }
    else if(I2E_ratioMeasure < 768){
      I2E_ratioSet = 0.34;
    }
    else{
      I2E_ratioSet = 0.5;
    }

//have parameters been changed?
    if( (volumeTidalSet < (lastVolumeTidalSet-5)||
        volumeTidalSet > (lastVolumeTidalSet+5)||
        breathsPerMinuteSet < (lastBreathsPerMinuteSet-1) ||
        breathsPerMinuteSet > (lastBreathsPerMinuteSet+1) ||
        limitPressureSet < (lastLimitPressureSet-2) ||
        limitPressureSet > (lastLimitPressureSet+2) ||
        I2E_ratioSet != lastI2E_ratioSet ) && !START_UP_FLAG) {
          CHANGES_FLAG = true;
            digitalWrite(LCD_BACKLIGHT, HIGH);
        }

        int modeMeasure = analogRead(MODE_SWITCH);
        if(modeMeasure < 250 && mode != MODE_HOMING){
          mode = MODE_VOLUME;
        }
        else if(modeMeasure < 750 && mode != MODE_HOMING){
          mode = MODE_PRESSURE;
        }
        else if(modeMeasure > 800){
          stopMotor();
          mode = MODE_IDLE;
          nextMode = MODE_IDLE;
        }
        if(mode != lastMode){
          lastMode = mode;
          state = 0;
        }

        if((volumeTidalSet <= (lastVolumeTidalSet-5)) ||
          (volumeTidalSet >= (lastVolumeTidalSet+5))){
            lastVolumeTidalSet = volumeTidalSet;
          }
          else{
            volumeTidalSet = lastVolumeTidalSet;
          }

    lastBreathsPerMinuteSet = breathsPerMinuteSet;
    lastLimitPressureSet = limitPressureSet;
    lastI2E_ratioSet = I2E_ratioSet;

    timerPotentiometer = millis() + 100;
  }

  if(digitalRead(CONFIRM_BUTTON) == BUTTON_PRESSED && millis()>timerDebounce){
      timerDebounce = millis() + 500;

      //confirm changes and start mode if not started
      volumeTidal = volumeTidalSet;
      breathsPerMinute = breathsPerMinuteSet;
      limitPressure = limitPressureSet;
      I2E_ratio = I2E_ratioSet;


      CHANGES_FLAG = false;
      digitalWrite(LCD_BACKLIGHT, HIGH);
      calculateVolumeParameters();

      //Confirm error
      MOTOR_FLAG = PRESSURE_FLAG = false;
      state=0;
    }

  if(digitalRead(SILENCE_BUTTON) == BUTTON_PRESSED && millis()>timerDebounce){
    SILENCE_FLAG = true;
    timerSilence = millis() + 10000;
  }
  else if( SILENCE_FLAG && millis() > timerSilence){
    SILENCE_FLAG = false;
  }

  //LCD outputs ----------------------------------------------------------------
  if(millis() > timerDisplay){
    updateValues(volumeTidalSet, breathsPerMinuteSet, I2E_ratioSet, limitPressureSet, pressurePlateau, realVolume, mode);
    updateErrors();
    timerDisplay = millis()+ 100;
  }

  if(!MOTOR_ACTIVE_FLAG){
    stopMotor(); //to avoid random movement of motor due to noise
  }
  if(START_UP_FLAG){
    //confirm changes and start mode if not started
    volumeTidal = volumeTidalSet;
    breathsPerMinute = breathsPerMinuteSet;
    limitPressure = limitPressureSet;
    I2E_ratio = I2E_ratioSet;
    START_UP_FLAG = false;
  }

}
