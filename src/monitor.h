#ifndef MONITOR_API_H
#define MONITOR_API_H

#include <Arduino.h>

// Monitor states
#define SCREEN_MINING   0
#define SCREEN_CLOCK    1
#define SCREEN_GLOBAL   2
#define NO_SCREEN       3   //Used when board has no TFT

//Time update period
#define TIME_UPDATE_PERIOD_h   5

//API BTC price (Update to USDT cus it's more liquidity and flow price updade)
#define getBTCAPI "https://api.blockchain.com/v3/exchange/tickers/BTC-USDT"
#define UPDATE_BTC_min   1

//API Block height
#define getHeightAPI "https://mempool.space/api/blocks/tip/height"
#define UPDATE_Height_min 2

//APIs Global Stats
#define getGlobalHash "https://mempool.space/api/v1/mining/hashrate/3d"
#define getDifficulty "https://mempool.space/api/v1/difficulty-adjustment"
#define getFees "https://mempool.space/api/v1/fees/recommended"
#define UPDATE_NETWORK_min 3

//API MinerStat
#define GET_MINERSTAT_COINS "https://api.minerstat.com/v2/coins?list=BTC" //https://api.minerstat.com/docs-coins/documentation

//API public-pool.io
#define GET_PUBLIC_POOL "https://public-pool.io:40557/api/client/" // +btcString
#define UPDATE_POOL_min 1

#define NEXT_HALVING_EVENT 1050000 //840000
#define HALVING_BLOCKS 210000

enum MiningState {
  PendingConfig,
  Connecting,
  Hashing
};

typedef struct{
  uint8_t screen;
  bool rotation;
  MiningState MinerStatus;
} monitor_data;

typedef struct {
  String completedShares;
  String totalMHashes;
  String totalKHashes;
  String currentHashRate;
  String templates;
  String bestDiff;
  String timeMining;
  String valids;
} mining_data;

typedef struct {
  String completedShares;
  String currentHashRate;
  String bestDiff;
  String blockHeight;
  String currentTime;
  String currentSec;
  String currentDate;
} clock_data;

typedef struct {
  String btcPrice;
  String netwrokDifficulty;
  String globalHashRate;
  String blockHeight;
  String blockReward;
  float progressPercent;
  String remainingBlocks;
} network_data;

typedef struct {
  String currentHashRate;
  String cpuTemp;
  String supplyVoltage;
  String NTPupdate;
  String poolIP;
  String poolUser;
  String sessId;
  String bestDiff;
} status_data;

typedef struct{
  int workersCount;       // Workers count of how many uMINERs using your address
  String workersHash;     // Workers Total Hash Rate
  String bestDifficulty;  // Your miners best difficulty
} pool_data;

void setup_monitor(void);
String *getBTCaddress();
mining_data getMiningData(unsigned long mElapsed);
clock_data getClockData(unsigned long mElapsed);
network_data getNetworkData(unsigned long mElapsed);
status_data getMinerStatus(unsigned long elapsed);
pool_data getPoolData(void);

String getPoolAPIUrl(void);

#endif //MONITOR_API_H
