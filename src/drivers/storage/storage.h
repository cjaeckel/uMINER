#ifndef _STORAGE_H_
#define _STORAGE_H_

#include <Arduino.h>

// config files

// default settings
#define DEFAULT_SSID		"uMINER"
#define DEFAULT_WIFIPW		"GoMinerGo"
#define DEFAULT_POOLURL		"public-pool.io"
#define DEFAULT_POOLPASS	"x"
#define DEFAULT_WALLETID	"your-Btc-Address"
#define DEFAULT_POOLPORT	21496
#define DEFAULT_TIMEZONE	2
#define DEFAULT_SAVESTATS	false
#define DEFAULT_INVERTCOLORS	false

// JSON config files
#define JSON_CONFIG_FILE	"/config.json"

// JSON config file SD card (for user interaction, readme.md)
#define JSON_KEY_SSID		"SSID"
#define JSON_KEY_PASW		"WifiPW"
#define JSON_KEY_POOLURL	"PoolUrl"
#define JSON_KEY_POOLPASS	"PoolPassword"
#define JSON_KEY_WALLETID	"BtcWallet"
#define JSON_KEY_POOLPORT	"PoolPort"
#define JSON_KEY_TIMEZONE	"Timezone"
#define JSON_KEY_STATS2NV	"SaveStats"
#define JSON_KEY_INVCOLOR	"invertColors"

// JSON config file SPIFFS (different for backward compatibility with existing devices)
#define JSON_SPIFFS_KEY_SSID		  "SSID"
#define JSON_SPIFFS_KEY_PWD		    "wifiPwd"
#define JSON_SPIFFS_KEY_ALTWIFI   "altWifi"
#define JSON_SPIFFS_KEY_POOLURL		"poolString"
#define JSON_SPIFFS_KEY_POOLPORT	"portNumber"
#define JSON_SPIFFS_KEY_POOLPASS	"poolPassword"
#define JSON_SPIFFS_KEY_WALLETID	"btcString"
#define JSON_SPIFFS_KEY_TIMEZONE	"gmtZone"
#define JSON_SPIFFS_KEY_STATS2NV	"saveStatsToNVS"
#define JSON_SPIFFS_KEY_INVCOLOR	"invertColors"

struct WifiCred {
  WifiCred() {};
  WifiCred(const String &s, const String &p) {
    SSID = s;
    Pwd = p;
  };
  String SSID = "";
  String Pwd = "";
};

// settings
struct TSettings {
  //backward compatibility
	String wifiSSID= DEFAULT_SSID;
	String wifiPwd= DEFAULT_WIFIPW;
  WifiCred altWifi[2];
  String PoolAddress= DEFAULT_POOLURL;
	String BtcWallet= DEFAULT_WALLETID;
	String PoolPassword= DEFAULT_POOLPASS;
	int PoolPort{ DEFAULT_POOLPORT };
	int Timezone{ DEFAULT_TIMEZONE };
	bool saveStats{ DEFAULT_SAVESTATS };
	bool invertColors{ DEFAULT_INVERTCOLORS };
};


#endif // _STORAGE_H_