
#include "drivers/displays/display.h"
#include "WifiConfig.h"
#include "SerialLog.h"

bool WifiConfig::TryConnect(const TSettings &settings) {
  logINF("TryConnect...");

  if(_hostname != "") {
    setupHostname(true);

    // disable wifi if already on
    if(WiFi.getMode() & WIFI_STA){
      WiFi.mode(WIFI_OFF);
      int timeout = millis()+1200;
      // async loop for mode change
      while(WiFi.getMode()!= WIFI_OFF && millis()<timeout){
        delay(1);
      }
    }
  }

  _startconn = millis();
  _begin();

  if(!WiFi.enableSTA(true)) {
    DEBUG_WM(WM_DEBUG_ERROR,F("[FATAL] Unable to enable wifi!"));
    return false;
  }

  WiFiSetCountry();

  #ifdef ESP32
  if(esp32persistent) WiFi.persistent(false); // disable persistent for esp32 after esp_wifi_start or else saves wont work
  #endif

  _usermode = WIFI_STA;

  // no getter for autoreconnectpolicy before this
  // https://github.com/esp8266/Arduino/pull/4359
  // so we must force it on else, if not connectimeout then waitforconnectionresult gets stuck endless loop
  WiFi_autoReconnect(); //enable auto reconnect handler

  if (WiFi.status() == WL_CONNECTED)
    return true;  //already connected

  std::vector<WifiCred> wifiCreds = WifiCredentials(settings);
  for (WifiCred c : wifiCreds) {
    logINF("TryConnect: %s...\n", c.SSID.c_str());
    if (connectWifi(c.SSID, c.Pwd) == WL_CONNECTED) {
      //connected
      _lastconxresult = WL_CONNECTED;
      logINF("Connected in %d ms\n", millis()-_startconn);
      logINF("STA IP: %s\n", WiFi.localIP().toString().c_str());
      if (_hostname != "") logINF("Hostname: STA: %s\n", getWiFiHostname().c_str());
      return true; // connected success
    }
  }

  logERR("WiFi connect FAILED after %d ms\n", millis()-_startconn);
  return false;
}

std::vector<WifiCred> WifiConfig::WifiCredentials(const TSettings &settings) {
  std::vector<WifiCred> creds;
  if (settings.wifiSSID != "") creds.push_back(WifiCred {settings.wifiSSID, settings.wifiPwd});
  if (settings.altWifi[0].SSID != "") creds.push_back(settings.altWifi[0]);
  if (settings.altWifi[1].SSID != "") creds.push_back(settings.altWifi[1]);
  return creds;
}

// Called when config mode launched
static void configModeCallback(WiFiManager* myWiFiManager) {
  logINF("Entered Configuration Mode\n");
  drawSetupScreen();
  logINF("Config SSID: %s\n", myWiFiManager->getConfigPortalSSID());
  logINF("Config IP Address: %s\n", WiFi.softAPIP());
}
