
#include <Wire.h>

#include <Arduino.h>
#include <WiFi.h>
#include <WebServer.h>
#include <esp_task_wdt.h>
#include <OneButton.h>

#include "mbedtls/md.h"
#include "Configuration.h"
#include "mining.h"
#include "monitor.h"
#include "drivers/displays/display.h"
#include "drivers/storage/SDCard.h"
#include "timeconst.h"
// #include "drivers/displays/DisplayControl.h"
#include "drivers/ButtonCntrl.h"

#define MAIN_LOOP_INTERVAL 50

//3 seconds WDT
#define WDT_TIMEOUT 3
//15 minutes WDT for miner task
#define WDT_MINER_TIMEOUT 900

  /**********************⚡ GLOBAL Vars *******************************/
#ifdef PIN_BUTTON_1
  OneButton button1(PIN_BUTTON_1);
#endif

  ButtonCntrl dsplButton(PIN_BUTTON_SCREEN, MAIN_LOOP_INTERVAL);
  ButtonCntrl screenButton(PIN_BUTTON_2, MAIN_LOOP_INTERVAL);
#ifdef PIN_BUTTON_2
  // OneButton button2(PIN_BUTTON_2);
#endif

#ifdef TOUCH_ENABLE
extern TouchHandler touchHandler;
#endif

extern monitor_data mMonitor;

#ifdef SD_ID
  SDCard SDCrd = SDCard(SD_ID);
#else
  SDCard SDCrd = SDCard();
#endif
  TSettings Settings;

  unsigned long start = millis();
  const char *ntpServer = "pool.ntp.org";

  using namespace std::placeholders;  // for _1, _2, _3...

  /********* INIT *****/
  void setup() {
    // Init pin 15 to eneble 5V external power (LilyGo bug)
  #ifdef PIN_ENABLE5V
      pinMode(PIN_ENABLE5V, OUTPUT);
      digitalWrite(PIN_ENABLE5V, HIGH);
  #endif

#ifdef MONITOR_SPEED
    Serial.begin(MONITOR_SPEED);
#else
    Serial.begin(115200);
#endif //MONITOR_SPEED

  Serial.setTimeout(0);
  delay(SECOND_MS/10);

  esp_task_wdt_init(WDT_MINER_TIMEOUT, true);
  // Idle task that would reset WDT never runs, because core 0 gets fully utilized
  disableCore0WDT();
  //disableCore1WDT();

  // Setup the buttons
  dsplButton.btnClick = alternateScreenState;
  dsplButton.btnDblClick = alternateScreenRotation;
  dsplButton.btnLongPress = handleDisplayAdjust;

  /******** INIT μMINER ************/
  Serial.println("uMINER starting......");

  /******** INIT DISPLAY ************/
  initDisplay();

  /******** PRINT INIT SCREEN *****/
  drawLoadingScreen();
  delay(2*SECOND_MS);

  /******** SHOW LED INIT STATUS (devices without screen) *****/
  mMonitor.MinerStatus = PendingConfig;
  doLedStuff(0);

#ifdef SDMMC_1BIT_FIX
  SDCrd.initSDcard();
#endif

  /******** INIT Configuration (optional from WIFI AP portal...) ************/
  Configuration config(Settings);
  screenButton.btnClick = switchToNextScreen;
  auto fn= std::bind(&Configuration::Reset, &config, _1, _2);
  screenButton.btnLongPress = fn;
  if (!config.VerifyNetwork())
    config.Configure();

  /******** CREATE TASK TO PRINT SCREEN *****/
  //tft.pushImage(0, 0, MinerWidth, MinerHeight, MinerScreen);
  // Higher prio monitor task
  Serial.println("");
  Serial.println("Initiating tasks...");
  BaseType_t res1 = xTaskCreatePinnedToCore(runMonitor, "Monitor", 10000, nullptr, 4, NULL, 1);
  BaseType_t res2 = xTaskCreatePinnedToCore(runStratumWorker, "Stratum", 15000, nullptr, 3, NULL, 1);

  /******** CREATE MINER TASKS *****/
  //for (size_t i = 0; i < THREADS; i++) {
  //  char *name = (char*) malloc(32);
  //  sprintf(name, "(%d)", i);

  // Start mining tasks
  TaskHandle_t minerTask1, minerTask2 = NULL;
  xTaskCreate(runMiner, "Miner0", 6000, (void*)0, 1, &minerTask1);
  xTaskCreate(runMiner, "Miner1", 6000, (void*)1, 1, &minerTask2);

  esp_task_wdt_add(minerTask1);
  esp_task_wdt_add(minerTask2);

  /******** MONITOR SETUP *****/
  setup_monitor();
}

void app_error_fault_handler(void *arg) {
  // Get stack errors
  char *stack = (char *)arg;

  // Print the stack errors in the console
  esp_log_write(ESP_LOG_ERROR, "APP_ERROR", "Pila de errores:\n%s", stack);

  // restart ESP32
  esp_restart();
}


void loop() {
  // keep watching the push buttons:
  dsplButton.Update();
  screenButton.Update();

  vTaskDelay(MAIN_LOOP_INTERVAL / portTICK_PERIOD_MS);
}
