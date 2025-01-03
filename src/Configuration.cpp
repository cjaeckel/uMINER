#define ESP_DRD_USE_SPIFFS true

#include <functional>
#include <WiFi.h>
#include <WiFiManager.h>

#include "Configuration.h"
#include "monitor.h"
#include "drivers/storage/SDCard.h"
#include "drivers/storage/nvMemory.h"
#include "drivers/storage/storage.h"
#include "drivers/displays/display.h"
#include "mining.h"
#include "timeconst.h"

extern SDCard SDCrd;
extern monitor_data mMonitor;

Configuration::Configuration(TSettings &set) : settings(set) {
  if (!nvMem.loadConfig(&settings)) {
    //No config file on internal flash.
    if (SDCrd.loadConfigFile(&settings)) {
      //No config file on SD card.
      SDCrd.SD2nvMemory(&nvMem, &settings); // reboot on success.
    }
  }
  SDCrd.terminate();  // Free the memory from SDCard class

  pool_fld.setValue(settings.PoolAddress.c_str(), 80);
  poolPwd_fld.setValue(settings.PoolPassword.c_str(), 80);
  port_fld.setValue(String(settings.PoolPort).c_str(), 7);
  btcAddr_fld.setValue(settings.BtcWallet.c_str(), 80);
  tmzone_fld.setValue(String(settings.Timezone).c_str(), 3);
}

bool Configuration::Reset(int sinceMs, int interval) {
  if (sinceMs < 7000) return true;

  Serial.println("Erasing Config, restarting");
  nvMem.deleteConfig();
  resetStat();
  wifiCfg.resetSettings();
  ESP.restart();
  return false;
}


// Handle AP start
void Configuration::handleConfigStart() {
  Serial.println("Config portal started");
  drawSetupScreen();
}

// Callback notifying us of the need to save configuration
void Configuration::handleConfigInput() {
  Serial.println("Applying config input");
  settings.PoolAddress = pool_fld.getValue();
  settings.PoolPort = atoi(port_fld.getValue());
  settings.PoolPassword= poolPwd_fld.getValue();
  settings.BtcWallet = btcAddr_fld.getValue();
  settings.Timezone = atoi(tmzone_fld.getValue());
  settings.saveStats = false; //(strncmp(save_stats_to_nvs.getValue(), "T", 1) == 0);
  nvMem.saveConfig(&settings);
}



bool Configuration::VerifyNetwork() {
#if defined(PIN_BUTTON_2)
  // Check if button2 is pressed to enter configMode with actual configuration
  if (!digitalRead(PIN_BUTTON_2)) {
    Serial.println(F("Button pressed to force start config mode"));
    wifiCfg.setBreakAfterConfig(true); //Set to detect config edition and save
    return false;   //Indicate that a setup is required
  }
#endif

  wifiCfg.setConnectTimeout(5); // how long to try to connect for before continuing

  return wifiCfg.TryConnect(settings);
}

void Configuration::Configure() {

  //Set Title and dark theme
  wifiCfg.setTitle("CRYPTO uMINER");
  wifiCfg.setClass("invert"); // dark theme

  // Set config save notify callback
  wifiCfg.setSaveParamsCallback(std::bind(&Configuration::handleConfigInput, this));

  // Set callback that gets when enters Access Point mode
  wifiCfg.setAPCallback(std::bind(&Configuration::handleConfigInput, this));

  // Add all defined parameters
  wifiCfg.addParameter(&pool_fld);
  wifiCfg.addParameter(&port_fld);
  wifiCfg.addParameter(&poolPwd_fld);
  wifiCfg.addParameter(&btcAddr_fld);
  wifiCfg.addParameter(&tmzone_fld);
  // wm.addParameter(&features_html);
  // wm.addParameter(&save_stats_to_nvs);

  wifiCfg.setCaptivePortalEnable(true);
  wifiCfg.setConfigPortalBlocking(true);
  wifiCfg.setEnableConfigPortal(true);
  wifiCfg.setConfigPortalTimeout(5*50); // auto close configportal after n seconds
  mMonitor.MinerStatus = Connecting;

  if (!wifiCfg.startConfigPortal(DEFAULT_SSID, DEFAULT_WIFIPW)) {
    delay(3*SECOND_MS);
    ESP.restart();  //reset and try again
    return;
  }

}
