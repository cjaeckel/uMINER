#ifndef _BUTTON_CNTRL_H_
#define _BUTTON_CNTRL_H_

#include <functional>
// typedef void (*ButtonActivation)(void);
// typedef bool (*ButtonPresentActivation)(int sinceMs, int interval);

#define BTN_SINGLE_CLICK_DELAY 400
#define BTN_DOUBLE_CLICK_DELAY 800

class ButtonCntrl {
public:
  ButtonCntrl(int buttonPin, int intervalMs) {
    BTN = buttonPin;
    IVAL = intervalMs;
  }
  void Update();
  // ButtonActivation Click;
  std::function<void()> Click;
  // ButtonActivation DblClick;
  std::function<void()> DblClick;
  // ButtonPresentActivation LongPress;
  std::function<bool(int, int)> LongPress;

private:
  int BTN;
  int IVAL;
  unsigned long startPress;
  unsigned long prevPress;
};

#endif