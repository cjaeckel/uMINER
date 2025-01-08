#ifndef _CONFIGURATION_H
#define _CONFIGURATION_H

#include "drivers/storage/nvMemory.h"
#include "utils/WifiConfig.h"

struct Configuration {
private:
  TSettings &settings;
  WifiConfig wifiCfg;
  nvMemory nvMem;

  WiFiManagerParameter ssid2_fld {"SSID2", "#2 SSID"};
  WiFiManagerParameter pwd2_fld {"Pwd2", "#2 Pass."};
  WiFiManagerParameter ssid3_fld {"SSID3", "#3 SSID"};
  WiFiManagerParameter pwd3_fld {"Pwd3", "#3 Pass."};
  WiFiManagerParameter pool_fld {"Poolurl", "Pool url"};
  WiFiManagerParameter port_fld {"Poolport", "Pool port"};
  WiFiManagerParameter btcAddr_fld {"btcAddress", "Your BTC address"};
  WiFiManagerParameter tmzone_fld {"TimeZone", "TimeZone fromUTC (-12/+12)"};
  WiFiManagerParameter poolPwd_fld {"Poolpassword - Optional", "Pool password"};
  void handleConfigStart();

public:
  void handleConfigInput();
  Configuration(TSettings &settings);
  bool VerifyNetwork();
  void Configure();
  bool Reset(int sinceMs= 999999999, int interval= 0);
};

#endif // _CONFIGURATION_H
