#include "displayDriver.h"

#ifdef T_DISPLAY

#include <TFT_eSPI.h>
#include "media/images_320_170.h"
#include "media/myFonts.h"
#include "media/Free_Fonts.h"
#include "version.h"
#include "monitor.h"
#include "SerialLog.h"
#include "OpenFontRender.h"
#include "rotation.h"
#include "drivers/storage/storage.h"

#define WIDTH 340
#define HEIGHT 170

extern TSettings Settings;            //from wManager.cpp

OpenFontRender render;
TFT_eSPI tft = TFT_eSPI();                  // Invoke library, pins defined in User_Setup.h
TFT_eSprite background = TFT_eSprite(&tft); // Invoke library sprite

void tDisplay_Init(void) {
      //Init pin 15 to eneble 5V external power (LilyGo bug)
#ifdef PIN_ENABLE5V
    pinMode(PIN_ENABLE5V, OUTPUT);
    digitalWrite(PIN_ENABLE5V, HIGH);
#endif

  tft.init();
  #ifdef LILYGO_S3_T_EMBED
  tft.setRotation(ROTATION_270);
  #else
  tft.setRotation(ROTATION_90);
  #endif
  tft.setSwapBytes(true);                 // Swap the colour byte order when rendering
  background.createSprite(WIDTH, HEIGHT); // Background Sprite
  background.setSwapBytes(true);
  render.setDrawer(background);  // Link drawing object to background instance (so font will be rendered on background)
  render.setLineSpaceRatio(0.9);

  // Load the font and check it can be read OK
  //if (render.loadFont(DigitalNumbers, sizeof(DigitalNumbers)))
  if (render.loadFont(Obitron, sizeof(Obitron))) {
    logERR("Error loading font!");
    return;
  }
}
void setBrightness(char value);

int tDisplay_AlternateScreenState(void) {
  int screen_state = digitalRead(TFT_BL);
  logDBG("Switching display state");
  screen_state = !screen_state;
  setBrightness(screen_state ? 16 : 0);
  return screen_state;
}

int tDisplay_AlternateRotation(void) {
  int flip = flipRotation(tft.getRotation());
  tft.setRotation(flip);
  return flip;
}

void tDisplay_MinerScreen(unsigned long mElapsed) {
  mining_data data = getMiningData(mElapsed);

  // Print background screen
  background.pushImage(0, 0, MinerWidth, MinerHeight, (const uint16_t*)MinerScreen);

  // Version
  render.setFontSize(11);
  render.drawString(CURRENT_VERSION, 10, 40, 0x630c);

  // Block templates
  render.setFontSize(18);
  render.drawString(data.templates.c_str(), 188, 23, 0xDEDB);
  // Best diff
  render.drawString(data.bestDiff.c_str(), 188, 49, 0xDEDB);
  // 32Bit shares
  render.setFontSize(18);
  render.drawString(data.completedShares.c_str(), 188, 76, 0xDEDB);
  // Uptime
  render.setFontSize(14);
  render.drawString(data.timeMining.c_str(), 188, 107, 0xDEDB);

  // Valid Blocks
  render.setFontSize(24);
  render.drawString(data.valids.c_str(), 279, 60, 0xDEDB);

  // // Time
  // render.setFontSize(9);
  // render.rdrawString(data.currentTime.c_str(), 122, 113, 0xDEDB);  //0x1BF3);

  // Total hashes
  render.setFontSize(14);
  render.drawString(data.totalMHashes.c_str(), 70, 107, 0xDEDB);

  // Hashrate
  render.setFontSize(22);
  render.rdrawString(data.currentHashRate.c_str(), 95, 145, 0xf700);  //TFT_BLACK);

  // BTC addr.
  render.setFontSize(12);
  render.drawString(getBTCaddress()->c_str(), 188, 149, 0x630c); //0x7b80);

  // Push prepared background to screen
  background.pushSprite(0, 0);
}

void tDisplay_ClockScreen(unsigned long mElapsed)
{
  clock_data data = getClockData(mElapsed);

  // Print background screen
  background.pushImage(0, 0, minerClockWidth, minerClockHeight, (const uint16_t*)minerClockScreen);

  // Version
  render.setFontSize(11);
  render.drawString(CURRENT_VERSION, 10, 40, 0x630c);

  // Time
  render.setFontSize(50);
  render.rdrawString(data.currentTime.c_str(), 250, 18, 0xDEDB);
  render.setFontSize(20);
  render.drawString((":" + data.currentSec).c_str(), 255, 48, 0xDEDB);
  // Date
  render.setFontSize(12);
  render.rdrawString(data.currentDate.c_str(), 293, 79, TFT_WHITE);

  // Hashrate
  render.setFontSize(22);
  render.rdrawString(data.currentHashRate.c_str(), 94, 111, 0xf700);  //TFT_BLACK);
  // Best diff.
  render.setFontSize(20);
  render.rdrawString(data.bestDiff.c_str(), 94, 142, 0xf700); //0xf700);

  // BTC addr.
  render.setFontSize(12);
  render.drawString(getBTCaddress()->c_str(), 188, 149, 0x630c); //0x7b80);

  // Push prepared background to screen
  background.pushSprite(0, 0);
}

void tDisplay_GlobalHashScreen(unsigned long mElapsed) {
  network_data data = getNetworkData(mElapsed);

  // Print background screen
  background.pushImage(0, 0, globalHashWidth, globalHashHeight, (const uint16_t*)globalHashScreen);

  // Version
  render.setFontSize(11);
  render.drawString(CURRENT_VERSION, 10, 40, 0x630c);

  // Price
  render.setFontSize(18);
  render.drawString(data.btcPrice.c_str(), 188, 23, 0xDEDB);
  // reward
  render.drawString(data.blockReward.c_str(), 188, 49, 0xDEDB);

  // Difficulty
  render.setFontSize(18);
  render.rdrawString(data.netwrokDifficulty.c_str(), 231, 74, 0xDEDB);

  // BlockHeight
  render.setFontSize(23);
  render.drawString(data.blockHeight.c_str(), 5, 100, 0xf700); //0xDEDB);

  // Remaining BLocks
  render.setFontSize(14);
  render.drawString(data.remainingBlocks.c_str(), 5, 137, 0xDEDB);

  // Draw percentage rectangle
  int x2 = 5 + (115 * data.progressPercent / 100);
  background.fillRect(5, 160, x2, 5, TFT_GREEN);
  background.fillRect(5, 166, 115, 1, TFT_RED);

  // Global Hashrate
  render.setFontSize(24);
  render.rdrawString(data.globalHashRate.c_str(), 270, 143, 0xf700);

  // BTC addr.
  render.setFontSize(12);
  render.drawString(getBTCaddress()->c_str(), 188, 103, 0x630c); //0x7b80);

  // Push prepared background to screen
  background.pushSprite(0, 0);
}


void tDisplay_MinerStatus(unsigned long mElapsed) {
  status_data data = getMinerStatus(mElapsed);

  //if (data.currentDate.indexOf("12/2023")>) { tDisplay_ChristmasContent(data); return; }

  // Print background screen
  background.pushImage(0, 0, statusScreenWidth, statusScreenHeight, (const uint16_t*)statusScreen);

  // Version
  render.setFontSize(11);
  render.drawString(CURRENT_VERSION, 10, 40, 0x630c);

  // NTP
  render.setFontSize(13);
  render.drawString(data.NTPupdate.c_str(), 200, 21, 0x7b80);

  // Pool
  render.setFontSize(13);
  render.drawString(data.poolIP.c_str(), 200, 44, 0x7b80);
  render.drawString(data.poolUser.c_str(), 200, 62, 0x7b80);
  render.drawString(data.sessId.c_str(), 200, 82, 0x7b80);
  render.drawString(data.bestDiff.c_str(), 200, 106, 0x7b80);

  // Mem
  render.setFontSize(13);
  render.drawString(data.minFreeHeap.c_str(), 200, 132, 0x7b80);
  render.drawString(data.minStack.c_str(), 257, 132, 0x7b80);

  // Hashrate
  render.setFontSize(22);
  render.drawString(data.currentHashRate.c_str(), 27, 97, 0xf700);

  // CPU Temp
  render.setFontSize(16);
  render.rdrawString(data.cpuTemp.c_str(), 90, 124, 0xDEDB);  //0x1BF3);
  render.setFontSize(9);
  render.rdrawString("0", 100, 124, 0xDEDB);  //0x1BF3);

  // Voltage
  render.setFontSize(16);
  render.drawString(data.supplyVoltage.c_str(), 65, 147, 0xDEDB);  //0x1BF3);

  // BTC addr.
  render.setFontSize(12);
  render.drawString(getBTCaddress()->c_str(), 213, 150, 0x630c); //0x7b80);

  // Push prepared background to screen
  background.pushSprite(0, 0);
}

void tDisplay_LoadingScreen(void) {
  tft.setSwapBytes(true);
  tft.pushImage(0, 0, initWidth, initHeight, (const uint16_t*)initScreen);

  tft.setTextSize(1);
  tft.setTextColor(TFT_DARKGREY);
  tft.drawString(CURRENT_VERSION, 10, 40, FONT2);
}

void tDisplay_SetupScreen(void) {
  tft.setSwapBytes(true);
  tft.pushImage(0, 0, setupModeWidth, setupModeHeight, (const uint16_t*)setupModeScreen);
  tft.setTextSize(1);
  tft.setTextColor(TFT_DARKGREY);
  tft.drawString(CURRENT_VERSION, 10, 40, FONT2);
}

void tDisplay_AnimateCurrentScreen(unsigned long frame) { }

void tDisplay_DoLedStuff(unsigned long frame) { }

// LilyGo T-Display-S3 backlight control chip supports 16 brightness levels.
// For the range 0~16, 0 is the minimum brightness (off), 16 is maximum brightness
void setBrightness(char value) {
  static uint8_t level = 0;
  const uint8_t MAX_LEV = 16;
  if (value == 0) {
    digitalWrite(TFT_BL, 0);
    delay(3);
    level = 0;
    return;
  }
  if (level == 0) {
    digitalWrite(TFT_BL, 1);
    level = MAX_LEV;
    delayMicroseconds(30);
  }
  int from = MAX_LEV - level;
  int to = MAX_LEV - value;
  int num = (MAX_LEV + to - from) % MAX_LEV;
  noInterrupts();
  for (int i = 0; i < num; ++i) {
    digitalWrite(TFT_BL, 0);
    // delayMicroseconds(5);
    digitalWrite(TFT_BL, 1);
    // delayMicroseconds(5);
  }
  interrupts();
  level = value;
}
CyclicScreenFunction tDisplayCyclicScreens[] = {tDisplay_MinerScreen, tDisplay_ClockScreen, tDisplay_GlobalHashScreen, tDisplay_MinerStatus};

DisplayDriver tDisplayDriver = {
    tDisplay_Init,
    setBrightness,
    tDisplay_AlternateScreenState,
    tDisplay_AlternateRotation,
    tDisplay_LoadingScreen,
    tDisplay_SetupScreen,
    tDisplayCyclicScreens,
    tDisplay_AnimateCurrentScreen,
    tDisplay_DoLedStuff,
    1, //TFT_BACKLIGHT_ON
    SCREENS_ARRAY_SIZE(tDisplayCyclicScreens),
    0,
    WIDTH,
    HEIGHT};
#endif
