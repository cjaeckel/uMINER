#ifndef _WIFI_CONFIG_H
#define _WIFI_CONFIG_H

#include <vector>
#include <WiFiManager.h>
#include "drivers/storage/storage.h"

class WifiConfig : public WiFiManager {
public:
  /* Try connect to WiFi specified in settings.
   * Return true on success
   */
  bool TryConnect(const TSettings &settings);

protected:
  /* Extract a std::vector<WifiCred> from settings.
   */
  std::vector<WifiCred> WifiCredentials(const TSettings &settings);
};

#endif  //_WIFI_CONFIG_H