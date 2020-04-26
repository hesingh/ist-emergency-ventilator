#include "../lib/LiquidCrystal_ID887/src/LiquidCrystal.h"
#include "display.h"
#include "pins.h"
#include "modes.h"
#include "globalFlags.h"
#include <Arduino.h>


LiquidCrystal lcd(LCD_RS, LCD_ENABLE, LCD_D4, LCD_D5, LCD_D6, LCD_D7);

bool PRESS_ERR_SCREEN_FLAG;
bool VOLUME_ERR_SCREEN_FLAG;


void initLCD(){
  lcd.begin(20, 4);
}

void homeScreen() {
  // static chars for home screen
  lcd.leftToRight();

  lcd.setCursor(0, 0);
  lcd.print("VOL:");
  lcd.setCursor(8,0);
  lcd.print("mL");
  lcd.setCursor(11, 0);
  lcd.print("PP:");
  lcd.setCursor(0, 1);
  lcd.print("BPM:");
  lcd.setCursor(0, 2);
  lcd.print("I2E:");
  lcd.setCursor(5,2);
  lcd.print("1:");
  lcd.setCursor(0, 3);
  lcd.print("PTH:");
  lcd.setCursor(13,3);
  lcd.print("[mbar]");
}

void updateValues(int volumeTidalDisp, int breathsPerMinuteDisp, float I2E_ratioDisp,
                  int assistDetectionPressureDisp, int pressurePlateauDisp,
                  int realVolumeDisp, int mode){
  int I2E_int;
  // dynamic values for homescreen
  lcd.setCursor(5, 0);
  lcd.print("   ");

  lcd.setCursor(5, 0);
  lcd.print(volumeTidalDisp);

  lcd.setCursor(6, 1);
  lcd.print("  ");
  if(breathsPerMinuteDisp < 10){
    lcd.setCursor(7, 1);
  }
  else{
    lcd.setCursor(6, 1);
  }
  lcd.print(breathsPerMinuteDisp);
  if(I2E_ratioDisp == 0.2){
    I2E_int = 4;
  }
  else if(I2E_ratioDisp == 0.25){
    I2E_int = 3;
  }
  else if(I2E_ratioDisp == 0.34){
    I2E_int = 2;
  }
  else{
    I2E_int = 1;
  }
  lcd.setCursor(7, 2);
  lcd.print(I2E_int);

  lcd.setCursor(6, 3);
  lcd.print("      ");

  lcd.setCursor(6, 3);
  lcd.print(assistDetectionPressureDisp);

  lcd.setCursor(17, 0);
  lcd.print("   ");

  if(mode == MODE_VOLUME){
    lcd.setCursor(11, 0);
    lcd.print(" PP:");
    if(pressurePlateauDisp < 10){
      lcd.setCursor(16, 0);
    }
    else if(pressurePlateauDisp < 100){
      lcd.setCursor(15, 0);
    }
    else{
      lcd.setCursor(14, 0);
    }
    lcd.print(pressurePlateauDisp);
  }
  else if(mode == MODE_PRESSURE){
    lcd.setCursor(11, 0);
    lcd.print("VOL:");
    lcd.setCursor(15, 0);
    lcd.print("    ");
    if(realVolumeDisp < 1000){
      lcd.setCursor(16, 0);
    }
    else{
      lcd.setCursor(15, 0);
    }
    lcd.print(realVolumeDisp);
  }

}

void updateErrors(){

  if(PRESSURE_FLAG && !PRESS_ERR_SCREEN_FLAG){
    showPressureError();
    PRESS_ERR_SCREEN_FLAG = true;
  }
  else if(!PRESSURE_FLAG && PRESS_ERR_SCREEN_FLAG){
    PRESS_ERR_SCREEN_FLAG = false;
    clearPressureError();
  }

  if(VOLUME_FLAG && !VOLUME_ERR_SCREEN_FLAG){
    showVolumeError();
    VOLUME_ERR_SCREEN_FLAG = true;
  }
  else if(!VOLUME_FLAG && VOLUME_ERR_SCREEN_FLAG){
    VOLUME_ERR_SCREEN_FLAG = false;
    clearVolumeError();
  }

}

void showPressureError(){
  lcd.leftToRight();
  lcd.setCursor(9, 1);
  lcd.print("          ");
  lcd.setCursor(9, 1);
  lcd.print("PRES ERR!");

}

void showVolumeError(){
  lcd.leftToRight();

  lcd.setCursor(9,2);
  lcd.print("          ");
  lcd.setCursor(9,2);
  lcd.print("LOW VOLUME!");
}

void clearPressureError(){
  lcd.leftToRight();
  lcd.setCursor(9, 1);
  lcd.print("          ");
}

void clearVolumeError(){
  lcd.leftToRight();
  lcd.setCursor(9,2);
  lcd.print("           ");
}
