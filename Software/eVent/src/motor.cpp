#include <Arduino.h>
#include "motor.h"
#include "pins.h"
#include "states.h"
#include "globalFlags.h"

bool prevDirection;
int prevSpeed;

void moveMotor(bool direction, int speed, bool flag){
  //move motor without stop condition
  if(((speed != prevSpeed) || (direction != prevDirection)) && !flag){
    analogWrite(MOTOR_PWM, speed);
    digitalWrite(MOTOR_DIR, direction);
    prevSpeed = speed;
    prevDirection = direction;
    MOTOR_ACTIVE_FLAG = true;
  }
}

void stopMotor(){
  analogWrite(MOTOR_PWM, 0);
  digitalWrite(MOTOR_PWM, 0);
  prevSpeed = 0;
  MOTOR_ACTIVE_FLAG = false;
}
