#include <Arduino.h>
#include <math.h>

#include "SerialLog.h"
#include "ButtonCntrl.h"
#include "timeconst.h"


void ButtonCntrl::Update() {
  if (0 == startPress) {
    if (digitalRead(BTN)) return; // no on going operation
    //press start:
    startPress = millis();
    return; //debounce
  }

  int elapsed = millis() - startPress;
  if (digitalRead(BTN)) { //button release
    if (   0 != startPress
        && elapsed < BTN_SINGLE_CLICK_DELAY) {    // short click
      if (   0 == prevPress
          || startPress - prevPress > BTN_DOUBLE_CLICK_DELAY) {
        prevPress = startPress;
        if (nullptr != Click) Click();   // single click
      }
      else {
        if (nullptr != DblClick) DblClick();   // double click
        prevPress = 0;
      }
    }
    startPress = 0;
    return;
  }

  // button still pressed
  if (0 == startPress) return;    //awaiting new press
  if (elapsed < BTN_SINGLE_CLICK_DELAY) return;    //wait for long press
  if (   nullptr != LongPress
      && LongPress(elapsed - BTN_SINGLE_CLICK_DELAY, IVAL)) return;   //keep button pressed

  logDBG("Long press abort");
  startPress = 0;
}

