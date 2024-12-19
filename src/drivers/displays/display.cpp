#include <Arduino.h>
#include "display.h"
#include "mining.h"


#ifdef T_DISPLAY
DisplayDriver *currentDisplayDriver = &tDisplayDriver;
#endif

bool handleDisplayAdjust(int sinceMs, int interval) {
  static int level = 0;
  int lev = 16 - (sinceMs / (2 * interval));
  lev= max(1, lev);
  if (lev != level && sinceMs < 1600) {
    level = lev;
    setDisplayBrightness(level);
  }
  return true;
}

// Initialize the display
void initDisplay() {
  currentDisplayDriver->initDisplay();
}

// Set brightness 0 - 16
void setDisplayBrightness(char value) {
  currentDisplayDriver->setBrightness(value);
}

// Alternate screen state
void alternateScreenState() {
  currentDisplayDriver->screenOn= currentDisplayDriver->alternateScreenState();
  Serial.printf("New screen state: %d\n", (int)currentDisplayDriver->screenOn);
}

// Alternate screen rotation
void alternateScreenRotation() {
  alternateScreenState();
  currentDisplayDriver->alternateScreenRotation();
  Serial.println("Screen rotate.");
}

// Draw the loading screen
void drawLoadingScreen()
{
  currentDisplayDriver->loadingScreen();
}

// Draw the setup screen
void drawSetupScreen()
{
  currentDisplayDriver->setupScreen();
}

// Reset the current cyclic screen to the first one
void resetToFirstScreen() {
  currentDisplayDriver->current_cyclic_screen = 0;
}

extern unsigned long monitorFrame;
// Switches to the next cyclic screen without drawing it
void switchToNextScreen() {
  currentDisplayDriver->current_cyclic_screen = (currentDisplayDriver->current_cyclic_screen + 1) % currentDisplayDriver->num_cyclic_screens;
  monitorFrame += MONITOR_REDRAW_EVERY - (monitorFrame % MONITOR_REDRAW_EVERY) - 1;
}

// Draw the current cyclic screen
void drawCurrentScreen(unsigned long mElapsed) {
  if (!currentDisplayDriver->screenOn) return;  //only draw when screen is on
  currentDisplayDriver->cyclic_screens[currentDisplayDriver->current_cyclic_screen](mElapsed);
}

// Animate the current cyclic screen
void animateCurrentScreen(unsigned long frame) {
  currentDisplayDriver->animateCurrentScreen(frame);
}

// Do LED stuff
void doLedStuff(unsigned long frame) {
  currentDisplayDriver->doLedStuff(frame);
}
