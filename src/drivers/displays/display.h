#ifndef DISPLAY_H
#define DISPLAY_H

#include "displayDriver.h"

extern DisplayDriver *currentDisplayDriver;

bool handleDisplayAdjust(int sinceMs, int interval);

void initDisplay();
void setDisplayBrightness(char value);
void alternateScreenState();
void alternateScreenRotation();
void switchToNextScreen();
void resetToFirstScreen();
void drawLoadingScreen();
void drawSetupScreen();
void drawCurrentScreen(unsigned long mElapsed);
void animateCurrentScreen(unsigned long frame);
void doLedStuff(unsigned long frame);

#endif // DISPLAY_H
