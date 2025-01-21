#include <Arduino.h>
#include <ArduinoJson.h>
#include <WiFi.h>
#include <esp_task_wdt.h>
#include <nvs_flash.h>
#include <nvs.h>
#include "SerialLog.h"
#include "SHA256/SHA256plus.h"
#include "stratum.h"
#include "mining.h"
#include "utils.h"
#include "monitor.h"
#include "timeconst.h"
#include "drivers/displays/display.h"
#include "drivers/storage/storage.h"

nvs_handle_t stat_handle;

uint32_t templates = 0;
uint32_t hashes = 0;
uint32_t Mhashes = 0;
uint32_t totalKHashes = 0;
uint32_t elapsedKHs = 0;
uint64_t upTime = 0;

uint32_t shares; // increase if blockhash has 32 bits of zeroes
uint32_t valids; // increased if blockhash <= target

// Track best diff
double best_diff = 0.0;

// Variables to hold data from custom textboxes
//Track mining stats in non volatile memory
extern TSettings Settings;

IPAddress serverIP(1, 1, 1, 1); //Temporally save poolIPaddres

//Global work data
WiFiClient wifi;
miner_data mMiner; //Global miner data (Create a miner class TODO)
mining_subscribe mWorker;
mining_job mJob;
monitor_data mMonitor;
bool isMinerSuscribed = false;
unsigned long mLastTXtoPool = millis();

int saveIntervals[7] = {5 * 60, 15 * 60, 30 * 60, 1 * 3600, 3 * 3600, 6 * 3600, 12 * 3600};
int saveIntervalsSize = sizeof(saveIntervals)/sizeof(saveIntervals[0]);
int currentIntervalIndex = 0;

bool checkPoolConnection(void) {

  if (wifi.connected()) {
    return true;
  }

  isMinerSuscribed = false;
  logWRN("Connection lost, trying to reconnect...\n");

  //Resolve first time pool DNS and save IP
  if (serverIP == IPAddress(1,1,1,1)) {
    WiFi.hostByName(Settings.PoolAddress.c_str(), serverIP);
    logINF("Resolved DNS and save ip (first time) got: %s\n", serverIP.toString());
  }

  //Try connecting pool IP
  if (!wifi.connect(serverIP, Settings.PoolPort)) {
    logERR("Imposible to connect to : %s\n", Settings.PoolAddress.c_str());
    WiFi.hostByName(Settings.PoolAddress.c_str(), serverIP);
    logERR("Resolved DNS got: %s\n", serverIP.toString());
    vTaskDelay(1000 / portTICK_PERIOD_MS);
    return false;
  }

  return true;
}

//Implements a socketKeepAlive function and
//checks if pool is not sending any data to reconnect again.
//Even connection could be alive, pool could stop sending new job NOTIFY
unsigned long mStart0Hashrate = 0;
bool checkPoolInactivity(unsigned int keepAliveTime, unsigned long inactivityTime) {

  unsigned long currentKHashes = (Mhashes*1000) + hashes/1000;
  unsigned long elapsedKHs = currentKHashes - totalKHashes;

  // If no shares sent to pool
  // send something to pool to hold socket oppened
  if (millis() - mLastTXtoPool > keepAliveTime){
    mLastTXtoPool = millis();
    logINF("Sending: KeepAlive suggest_difficulty\n");
    //if (client.print("{}\n") == 0) {
      tx_suggest_difficulty(wifi, DEFAULT_DIFFICULTY);
    /*if (tx_suggest_difficulty(client, DEFAULT_DIFFICULTY)){
      logINF("Sending keepAlive to pool -> Detected client disconnected\n");
      return true;
    }*/
  }

  if (elapsedKHs == 0) {
    //Check if hashrate is 0 during inactivityTIme
    if (mStart0Hashrate == 0) mStart0Hashrate = millis();
    if ((millis()-mStart0Hashrate) <= inactivityTime) return false;
    mStart0Hashrate= 0;
    return true;
  }

  mStart0Hashrate = 0;
  return false;
}

void runStratumWorker(void *nil) {

// TEST: https://bitcoin.stackexchange.com/questions/22929/full-example-data-for-scrypt-stratum-client

  logALL("\n");
  logALL("Stratum worker started. (Running on core %d)\n", xPortGetCoreID());

  logDBG("[Total Heap / Free heap / Min free heap]: %d / %d / %d \n", ESP.getHeapSize(), ESP.getFreeHeap(), ESP.getMinFreeHeap());

  // connect to pool
  double currentPoolDifficulty = DEFAULT_DIFFICULTY;

  while(true) {

    if (WiFi.status() != WL_CONNECTED){
      // WiFi is disconnected, so reconnect now
      mMonitor.MinerStatus = Connecting;
      WiFi.reconnect();
      vTaskDelay(5000 / portTICK_PERIOD_MS);
      continue;
    }

    if (!checkPoolConnection()){
      //If server is not reachable add random delay for connection retries
      srand(millis());
      //Generate value between 1 and 120 secs
      vTaskDelay(((1 + rand() % 120) * 1000) / portTICK_PERIOD_MS);
    }

    if (!isMinerSuscribed){

      //Stop miner current jobs
      mMiner.inRun = false;
      mWorker = init_mining_subscribe();

      // STEP 1: Pool server connection (SUBSCRIBE)
      if (!tx_mining_subscribe(wifi, mWorker)) {
        wifi.stop();
        continue;
      }

      strcpy(mWorker.wName, Settings.BtcWallet.c_str());
      strcpy(mWorker.wPass, Settings.PoolPassword.c_str());
      // STEP 2: Pool authorize work (Block Info)
      tx_mining_auth(wifi, mWorker.wName, mWorker.wPass); //Don't verifies authoritzation, TODO
      //tx_mining_auth2(client, mWorker.wName, mWorker.wPass); //Don't verifies authoritzation, TODO

      // STEP 3: Suggest pool difficulty
      tx_suggest_difficulty(wifi, DEFAULT_DIFFICULTY);

      isMinerSuscribed=true;
      mLastTXtoPool = millis();
    }

    //Check if pool is down for almost 5minutes and then restart connection with pool (1min=600000ms)
    if (checkPoolInactivity(KEEPALIVE_TIME_ms, POOLINACTIVITY_TIME_ms)) {
      //Restart connection
      logWRN("Detected more than 2 min without data form stratum server. Closing socket and reopening...\n");
      wifi.stop();
      isMinerSuscribed=false;
      continue;
    }

    //Read pending messages from pool
    while(wifi.connected() && wifi.available()) {

      String line = wifi.readStringUntil('\n');
      stratum_method result = parse_mining_method(line);
      logINF("Received pool message of type: %u\n", result);
      switch (result) {
          case STRATUM_PARSE_ERROR:   logERR("Parsed JSON: error on JSON\n"); break;
          case MINING_NOTIFY:         if (parse_mining_notify(line, mJob)){
                                          //Increse templates readed
                                          templates++;
                                          //Stop miner current jobs
                                          mMiner.inRun = false;
                                          //Prepare data for new jobs
                                          mMiner=calculateMiningData(mWorker,mJob);
                                          mMiner.poolDifficulty = currentPoolDifficulty;
                                          mMiner.newJob = true;
                                          mMiner.newJob2 = true;
                                          //Give new job to miner

                                      }
                                      break;
          case MINING_SET_DIFFICULTY: parse_mining_set_difficulty(line, currentPoolDifficulty);
                                      mMiner.poolDifficulty = currentPoolDifficulty;
                                      break;
          case STRATUM_SUCCESS:       logINF("Parsed JSON: Success\n"); break;
          default:                    logWRN("Parsed JSON: unknown\n"); break;

      }
    }

    vTaskDelay(500 / portTICK_PERIOD_MS); //Small delay

  }

}


//////////////////THREAD CALLS///////////////////

//This works only with one thread, TODO -> Class or miner_data for each thread


void runMiner(void * task_id) {

  unsigned int miner_id = (uint32_t)task_id;

  logALL("Started runMiner Task #%d!\n", miner_id);

  while(1) {

    //Wait new job
    while(1) {
      if (mMiner.newJob==true || mMiner.newJob2==true) break;
      vTaskDelay(100 / portTICK_PERIOD_MS); //Small delay
    }
    vTaskDelay(10 / portTICK_PERIOD_MS); //Small delay to join both mining threads

    if (mMiner.newJob)
      mMiner.newJob = false; //Clear newJob flag
    else if (mMiner.newJob2)
      mMiner.newJob2 = false; //Clear newJob flag
    mMiner.inRun = true; //Set inRun flag

    mMonitor.MinerStatus = Hashing;

    //Prepare Premining data
    SHA256mining sha256Ctx;
    uint8_t hash[32];


    //Calcular midstate
    sha256Ctx.InitBlockHeader(mMiner.bytearray_blockheader);

    // search a valid nonce
    unsigned long nonce = TARGET_NONCE - MAX_NONCE;
    // split up odd/even nonces between miner tasks
    nonce += miner_id;
    uint32_t startT = micros();
    unsigned char *header64;
    // each miner thread needs to track its own blockheader template
    uint8_t temp;

    memcpy(mMiner.bytearray_blockheader2, &mMiner.bytearray_blockheader, 80);
    if (miner_id == 0)
      header64 = mMiner.bytearray_blockheader + 64;
    else
      header64 = mMiner.bytearray_blockheader2 + 64;

    bool is16BitShare=true;
    logINF(">>> STARTING TO HASH NONCES\n");
    while(true) {
      if (miner_id == 0)
        memcpy(mMiner.bytearray_blockheader + 76, &nonce, 4);
      else
        memcpy(mMiner.bytearray_blockheader2 + 76, &nonce, 4);


      is16BitShare= sha256Ctx.SHA256d(header64, hash);

      hashes++;
      if (nonce > TARGET_NONCE) break; //exit
      if (!mMiner.inRun) { logINF("MINER WORK ABORTED >> waiting new job\n"); break;}

      // check if 16bit share
      if (hash[31] !=0 || hash[30] !=0) {
      //if (!is16BitShare){
        // increment nonce
        nonce += 2;
        continue;
      }

      //Check target to submit
      //Difficulty of 1 > 0x00000000FFFF0000000000000000000000000000000000000000000000000000
      //NM2 pool diff 1e-9 > Target = diff_1 / diff_pool > 0x00003B9ACA00....00
      //Swapping diff bytes little endian >>>>>>>>>>>>>>>> 0x0000DC59D300....00
      //if ((hash[29] <= 0xDC) && (hash[28] <= 0x59))     //0x00003B9ACA00  > diff value for 1e-9
      double diff_hash = diff_from_target(hash);

      // update best diff
      if (diff_hash > best_diff)
        best_diff = diff_hash;

      if (diff_hash > mMiner.poolDifficulty) { //(hash[29] <= 0x3B)//(diff_hash > 1e-9)
        tx_mining_submit(wifi, mWorker, mJob, nonce);
        logINF("   - Current diff share: %g\n", diff_hash);
        logINF("   - Current pool diff : %g\n", mMiner.poolDifficulty);
        logINF("   - TX SHARE: ");
        for (size_t i = 0; i < 32; i++) logINF0("%02x", hash[i]);
        logINF0("\n\n");
        logINF("   - Current nonce: %d\n", nonce);
        logDBG("   - Current block header: ");
        for (size_t i = 0; i < 80; i++) logDBG0("%02x", mMiner.bytearray_blockheader[i]);
        logDBG0("\n");
        mLastTXtoPool = millis();
      }

      // check if 32bit share
      if (hash[29] !=0 || hash[28] !=0) {
        // increment nonce
        nonce += 2;
        continue;
      }
      shares++;

      // check if valid header
      if (checkValid(hash, mMiner.bytearray_target)) {
        logALL("Worker #%d: CONGRATULATIONS! Valid block found with nonce: %d | 0x%x\n", miner_id, nonce, nonce);
        valids++;
        logALL("Worker #%d:  Submitted work valid!\n", miner_id);
        break;
      }
      // increment nonce
      nonce += 2;
    } // exit if found a valid result or nonce > MAX_NONCE

    mMiner.inRun = false;
    logINF(">>> Finished job waiting new data from pool\n");

    if (hashes>=MAX_NONCE_STEP) {
      Mhashes=Mhashes+MAX_NONCE_STEP/1000000;
      hashes=hashes-MAX_NONCE_STEP;
    }

    uint32_t duration = micros() - startT;
    if (esp_task_wdt_reset() == ESP_OK)
      logINF(">>> Resetting watchdog timer\n");
  }
}

void restoreStat() {
  if (true || !Settings.saveStats) return;   //DISABLED
  esp_err_t ret = nvs_flash_init();
  if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
    logWRN("NVS partition is full or has invalid version, erasing...\n");
    nvs_flash_init();
  }

  ret = nvs_open("state", NVS_READWRITE, &stat_handle);

  size_t required_size = sizeof(double);
  nvs_get_blob(stat_handle, "best_diff", &best_diff, &required_size);
  nvs_get_u32(stat_handle, "Mhashes", &Mhashes);
  nvs_get_u32(stat_handle, "shares", &shares);
  nvs_get_u32(stat_handle, "valids", &valids);
  nvs_get_u32(stat_handle, "templates", &templates);
  nvs_get_u64(stat_handle, "upTime", &upTime);
}

void saveStat() {
  if (true || !Settings.saveStats) return;    //DISABLED
  logINF("Saving stats\n");
  nvs_set_blob(stat_handle, "best_diff", &best_diff, sizeof(double));
  nvs_set_u32(stat_handle, "Mhashes", Mhashes);
  nvs_set_u32(stat_handle, "shares", shares);
  nvs_set_u32(stat_handle, "valids", valids);
  nvs_set_u32(stat_handle, "templates", templates);
  nvs_set_u64(stat_handle, "upTime", upTime + (esp_timer_get_time()/1000000));
}

void resetStat() {
    logINF("[MONITOR] Resetting NVS stats\n");
    templates = hashes = Mhashes = totalKHashes = elapsedKHs = upTime = shares = valids = 0;
    best_diff = 0.0;
    saveStat();
}

unsigned long monitorFrame = 0;

void runMonitor(void *nil) {

  logALL("%s started\n", __FUNCTION__);
  restoreStat();

  unsigned long mLastCheck = 0;

  resetToFirstScreen();


  uint32_t seconds_elapsed = 0;

  totalKHashes = (Mhashes * 1000) + hashes / 1000;;

  while (true) {
    if (0 == (monitorFrame % MONITOR_REDRAW_EVERY)) {
      unsigned long mElapsed = millis() - mLastCheck;
      mLastCheck = millis();
      unsigned long currentKHashes = (Mhashes * 1000) + hashes / 1000;
      elapsedKHs = currentKHashes - totalKHashes;
      totalKHashes = currentKHashes;

      drawCurrentScreen(mElapsed);

      // Monitor state when hashrate is 0.0
      if (elapsedKHs == 0) {
        logINF(">>> [i] Miner: newJob>%s / inRun>%s) - Client: connected>%s / subscribed>%s / wificonnected>%s\n",
               mMiner.newJob ? "true" : "false", mMiner.inRun ? "true" : "false",
               wifi.connected() ? "true" : "false", isMinerSuscribed ? "true" : "false", WiFi.status() == WL_CONNECTED ? "true" : "false");
      }
      else if ((monitorFrame % (5*MONITOR_REDRAW_EVERY)) == 0){
        char pfx;
        logINF(">>> %s %cH  %d kH/s  %d shares\n",
               numFormat("%.1f", 1e6*Mhashes, pfx), pfx, (elapsedKHs * 1000) / mElapsed, shares);

      }
#ifdef DEBUG_MEMORY
      logINF("### [Total Heap / Free heap / Min free heap]: %d / %d / %d \n", ESP.getHeapSize(), ESP.getFreeHeap(), ESP.getMinFreeHeap());
      logINF("### Min stack avail: %d\n", uxTaskGetStackHighWaterMark(NULL));
#endif

      if ((seconds_elapsed++) % (saveIntervals[currentIntervalIndex]) == 0) {
        saveStat();
        seconds_elapsed = 0;
        if (currentIntervalIndex < saveIntervalsSize - 1)
          currentIntervalIndex++;
      }
    }

    animateCurrentScreen(monitorFrame);
    doLedStuff(monitorFrame);

    vTaskDelay(MONITOR_DELAY / portTICK_PERIOD_MS);
    monitorFrame++;
  } //while (true)
}
