//analog sensors
#define PRESSURE A5       //(922-7409), analog 0.2-4.7V, 0-1019.78mmH2O

#define LIMIT_SWITCH   13     //limit switch for homing

//control potentiometers, single turn 10k, analog 0-5V (789-9438)
#define VOL_POT A0            //sets volume setpoint in ml
#define RATE_POT A1           //sets breaths per minute 5-40 BPM
#define I2E_POT A2            //sets inspiration/expiration time ratio 1:1 - 1:4
#define PRS_POT A3            //sets max pressure

//switches and buttons
#define MODE_SWITCH A4         //selects volume or assist control
#define SILENCE_BUTTON 11         // momentary button to mute alarms
#define CONFIRM_BUTTON  8          // momentary buttton to confirm potentiometer changes

//OUTPUTS
#define BUZZER      9             //piezo buzzer (724-3178) PWM

//Display
#define LCD_RS      6
#define LCD_ENABLE  5
#define LCD_D4      4
#define LCD_D5      1
#define LCD_D6      7
#define LCD_D7      0
#define LCD_BACKLIGHT 14


//Encoder
#define ENC_A   2
#define ENC_B   10
#define ENC_I   18

//Leds
#define LED_POWER 15
#define LED_GREEN 16
#define LED_RED 17

#define RESET_PIN A11
