#include <Arduino.h>
#include <WiFi.h>
#include "mbedtls/md.h"
#include "HTTPClient.h"
#include <NTPClient.h>
#include <WiFiUdp.h>
#include "mining.h"
#include "utils.h"
#include "monitor.h"
#include "drivers/storage/storage.h"
#include "drivers/devices/device.h"

extern uint32_t templates;
extern uint32_t hashes;
extern uint32_t Mhashes;
extern uint32_t totalKHashes;
extern uint32_t elapsedKHs;
extern uint64_t upTime;
extern WiFiClient wifi;

extern uint32_t shares; // increase if blockhash has 32 bits of zeroes
extern uint32_t valids; // increased if blockhash <= targethalfshares

extern double best_diff; // track best diff

extern monitor_data mMonitor;

//from saved config
extern TSettings Settings;
bool invertColors = false;

WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, "europe.pool.ntp.org", 3600, 60000);
pool_data pData;
String poolAPIUrl;
String *BTCaddr = NULL;

void setup_monitor(void){
    /******** TIME ZONE SETTING *****/

    timeClient.begin();

    // Adjust offset depending on your zone
    // GMT +2 in seconds (zona horaria de Europa Central)
    timeClient.setTimeOffset(3600 * Settings.Timezone);

    Serial.println("TimeClient setup done");
}

String *getBTCaddress() {
  if (NULL == BTCaddr) {
    BTCaddr = new String(Settings.BtcWallet.substring(0, 11) + "...");
  }
  return BTCaddr;
}

unsigned long lastNetUpdate= 0;

void updateNetworkData(network_data &netData) {

  if (   WiFi.status() != WL_CONNECTED
      || (lastNetUpdate != 0 && (millis() - lastNetUpdate < UPDATE_NETWORK_min * 60 * 1000) )) return;
  Serial.println("Starting updateNetworkData()...");

  HTTPClient http;
  http.setTimeout(1000);

  try {
    if (   !http.begin(GET_MINERSTAT_COINS)
        || http.GET() != HTTP_CODE_OK) {
      Serial.print("HTTP GET failed from: ");
      Serial.println(GET_MINERSTAT_COINS);
      return;
    }

    String payload = http.getString();
    DynamicJsonDocument json(1024);
    deserializeJson(json, payload);
    JsonObjectConst doc = json.as<JsonArrayConst>()[0];
    char fStr[16] = "";
    char pfx;

    netData.globalHashRate = numFormat("%.1f", doc["network_hashrate"].as<double>(), pfx);
    netData.netwrokDifficulty = numFormat("%.1f", doc["difficulty"].as<double>(), pfx) + pfx;  //fStr;

    sprintf(fStr, "%.4G", doc["reward_block"].as<double>());
    netData.blockReward = fStr;

    sprintf(fStr, "%.0f", doc["price"].as<double>());
    netData.btcPrice = fStr;
    unsigned int n = netData.btcPrice.length();
    if (n > 3)
      netData.btcPrice = netData.btcPrice.substring(0, n - 3) + " " + netData.btcPrice.substring(n - 3);  //thousends separator
    netData.btcPrice += "$";

    json.clear();
    lastNetUpdate = millis();
    http.end();
  }
  catch(...) {
    http.end();
  }
}

unsigned long lastBlockUpdate = 0;

String getBlockHeight(void){
  static String block = "";
  if (   WiFi.status() != WL_CONNECTED
      || (lastBlockUpdate != 0 && (millis() - lastBlockUpdate < UPDATE_Height_min * 60 * 1000))) return block;

  HTTPClient http;
  http.setTimeout(1000);
  try {
    if (   !http.begin(getHeightAPI)
        || http.GET() != HTTP_CODE_OK) {
      Serial.print("HTTP GET failed from: ");
      Serial.println(getHeightAPI);
      return block;
    }

    String payload = http.getString();
    payload.trim();
    block = payload;
    lastBlockUpdate = millis();
    http.end();
  } catch(...) {
    http.end();
  }
  return block;
}


unsigned long lastTimeUpdate = 0;
unsigned long lastNTPTime = 0;
unsigned long mPoolUpdate = 0;

void timeToHMS(unsigned long time, unsigned long &h, unsigned long &m, unsigned long &s) {
  // convert time into hours, minutes and seconds
  h = time % 86400 / 3600;
  m = time % 3600 / 60;
  s = time % 60;
}

void getTime(unsigned long tm, unsigned long* currentHours, unsigned long* currentMinutes, unsigned long* currentSeconds) {

  //Check if we need an NTP time update
  if ((lastTimeUpdate == 0) || (millis() - lastTimeUpdate > TIME_UPDATE_PERIOD_h * 60 * 60 * 1000)) { //60 sec. * 60 min * 1000ms
    if (WiFi.status() == WL_CONNECTED && timeClient.update()) { //NTP call to get current time succeeded:
      lastTimeUpdate = millis();
      lastNTPTime = timeClient.getEpochTime(); // Most current NTP unix time (sec. since 1970)
      Serial.print("TimeClient NTPupdateTime ");
    }
  }

  if (0 == tm) {
    unsigned long elapsedTime = (millis() - lastTimeUpdate) / 1000; // elapsed sec. since last time update via NTP
    tm = lastNTPTime + elapsedTime; // current time
  }

  timeToHMS(tm, *currentHours, *currentMinutes, *currentSeconds);
}

String getDate() {

  unsigned long elapsedTime = (millis() - lastTimeUpdate) / 1000; // elapsed time since boot in sec.
  unsigned long currentTime = lastNTPTime + elapsedTime; // current unix time

  // get local time struct
  struct tm *tm = localtime((time_t *)&currentTime);

  int year = tm->tm_year + 1900; // convert from unix epoch 1900
  int month = tm->tm_mon + 1;    // convert from tm_mon in 0..11
  int day = tm->tm_mday;

  char currentDate[12];
  sprintf(currentDate, "%04d-%02d-%02d", year, month, day);

  return String(currentDate);
}

String getTime(unsigned long tm= 0) {
  unsigned long currentHours, currentMinutes, currentSeconds;
  getTime(tm, &currentHours, &currentMinutes, &currentSeconds);

  char localTm[6];
  sprintf(localTm, "%02d:%02d", currentHours, currentMinutes);

  return localTm;
}

String getSeconds(unsigned long tm= 0) {
  unsigned long currentHours, currentMinutes, currentSeconds;
  getTime(tm, &currentHours, &currentMinutes, &currentSeconds);

  char seconds[3];
  sprintf(seconds, "%02d", currentSeconds);

  return seconds;
}

String getCurrentHashRate(unsigned long mElapsed) {
  char pfx;
  return numFormat("%.2f", (1.0 * (elapsedKHs * 1000)) / mElapsed, pfx);
}

mining_data getMiningData(unsigned long mElapsed) {
  mining_data data;

  char best_diff_string[16] = {0};
  suffix_string(best_diff, best_diff_string, 16, 0);

  char timeMining[15] = {0};
  uint64_t secElapsed = upTime + (esp_timer_get_time() / 1000000);
  int days = secElapsed / 86400;
  int hours = (secElapsed - (days * 86400)) / 3600;               // Number of seconds in an hour
  int mins = (secElapsed - (days * 86400) - (hours * 3600)) / 60; // Remove the number of hours and calculate the minutes.
  int secs = secElapsed - (days * 86400) - (hours * 3600) - (mins * 60);
  sprintf(timeMining, "%01d  %02d:%02d:%02d", days, hours, mins, secs);

  data.completedShares = shares;
  data.totalMHashes = Mhashes;
  data.totalKHashes = totalKHashes;
  data.currentHashRate = getCurrentHashRate(mElapsed);
  data.templates = templates;
  data.bestDiff = best_diff_string;
  data.timeMining = timeMining;
  data.valids = valids;
  // data.temp = String(temperatureRead(), 0);
  // data.currentTime = getTime();

  //uint32_t volt = (analogRead(PIN_BAT_VOLT) * 2 * 3.3 * 1000) / 4096;
  return data;
}

clock_data getClockData(unsigned long mElapsed) {
  clock_data data;
  char best_diff_string[16] = {0};
  suffix_string(best_diff, best_diff_string, 16, 0);

  data.completedShares = shares;
  data.currentHashRate = getCurrentHashRate(mElapsed);
  data.bestDiff = best_diff_string;
  data.blockHeight = getBlockHeight();
  data.currentTime = getTime();
  data.currentSec = getSeconds();
  data.currentDate = getDate();

  return data;
}

network_data getNetworkData(unsigned long mElapsed) {
  static network_data data;

  updateNetworkData(data);
  data.blockHeight = getBlockHeight();

  unsigned long currentBlock = data.blockHeight.toInt();
  unsigned long remainingBlocks = (((currentBlock / HALVING_BLOCKS) + 1) * HALVING_BLOCKS) - currentBlock;
  data.progressPercent = (HALVING_BLOCKS - remainingBlocks) * 100 / HALVING_BLOCKS;
  data.remainingBlocks = String(remainingBlocks);

  return data;
}

void updatePoolData(status_data &poolData) {
  static unsigned long lastPoolUpdate= 0;

  if (   WiFi.status() != WL_CONNECTED
      || (lastPoolUpdate != 0 && (millis() - lastPoolUpdate < UPDATE_POOL_min * 60 * 1000) )) return;

  HTTPClient http;
  http.setTimeout(1000);
  try {
    String poolUrl = Settings.BtcWallet;
    int p = poolUrl.lastIndexOf('.');
    if (p > 0) poolUrl= poolUrl.substring(0, p);
    poolUrl = GET_PUBLIC_POOL + poolUrl;

    Serial.printf("+++ HTTP GET: %s\n", poolUrl.c_str());
    int ret;
    if (   !http.begin(poolUrl)
        || (ret= http.GET()) != HTTP_CODE_OK) {
      Serial.printf("+++ HTTP GET failed: %d\n", ret);
      return;
    }

    String payload = http.getString();
    Serial.println(payload);
    DynamicJsonDocument json(1024);
    deserializeJson(json, payload);
    char fStr[16] = "";
    char pfx;

    poolData.bestDiff = numFormat("%.2f", json["bestDifficulty"].as<double>(), pfx) + pfx;

    if (0 != json["workersCount"].as<int>()) {
      JsonObjectConst doc = json["workers"].as<JsonArrayConst>()[0];

      poolData.poolUser = doc["name"].as<const char*>();
      poolData.sessId = doc["sessionId"].as<const char*>();
      lastPoolUpdate = millis();
    }
    json.clear();
    http.end();
  }
  catch(...) {
    http.end();
    Serial.println("+++ POOL UPDATE FAILED.");
    lastPoolUpdate = 0;
  }
}

status_data getMinerStatus(unsigned long elapsed) {
  static status_data data;

  data.currentHashRate= getCurrentHashRate(elapsed);
  data.cpuTemp = String(temperatureRead(), 0);

  float mVolt = (analogRead(PIN_BAT_VOLT) * 2 * 3.3 * 1000) / 4096;
  char fStr[10] = {0};
  sprintf(fStr, "%.1fV", mVolt/1000.0);
  data.supplyVoltage = fStr;

  data.NTPupdate = getTime(lastNTPTime);
  data.poolIP = wifi.remoteIP().toString();

  updatePoolData(data);
  return data;
}
