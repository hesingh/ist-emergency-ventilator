#ifndef DISPLAY_H
#define DISPLAY_H

void initLCD();
void homeScreen();
void updateValues(int volumeTidalDisp, int breathsPerMinuteDisp, float I2E_ratioDisp, int assistDetectionPressureDisp, int pressurePlateauDisp, int realVolumeDisp, int mode);
void updateErrors();
void showPressureError();
void showVolumeError();
void clearPressureError();
void clearVolumeError();
void showMode(int state, int mode, int ticks, int nextMode);
void showParameters(int a, int b, int c, int d);

#endif
