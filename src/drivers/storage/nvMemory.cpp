#include "nvMemory.h"

#ifdef NVMEM_SPIFFS

#include <SPIFFS.h>
#include <FS.h>
#include <ArduinoJson.h>

#include "../devices/device.h"
#include "storage.h"

nvMemory::nvMemory() : Initialized_(false){};

nvMemory::~nvMemory()
{
    if (Initialized_)
        SPIFFS.end();
};

/// @brief Save settings to config file on SPIFFS
/// @param TSettings* Settings to be saved.
/// @return true on success
bool nvMemory::saveConfig(TSettings* Settings) {
    if (!init()) return false;

    // Save Config in JSON format
    Serial.println(F("SPIFS: Saving configuration."));

    // Create a JSON document
    StaticJsonDocument<512> json;
    json[JSON_SPIFFS_KEY_SSID]      = Settings->wifiSSID;
    json[JSON_SPIFFS_KEY_PWD]       = Settings->wifiPwd;
    auto altWifi = json.createNestedArray(JSON_SPIFFS_KEY_ALTWIFI);
    auto cred = altWifi.createNestedObject();
    cred["SSID"] = Settings->altWifi[0].SSID;
    cred["Pwd"] = Settings->altWifi[0].Pwd;
    cred = altWifi.createNestedObject();
    cred["SSID"] = Settings->altWifi[1].SSID;
    cred["Pwd"] = Settings->altWifi[1].Pwd;

    json[JSON_SPIFFS_KEY_POOLURL] = Settings->PoolAddress;
    json[JSON_SPIFFS_KEY_POOLPORT]  = Settings->PoolPort;
    json[JSON_SPIFFS_KEY_POOLPASS]  = Settings->PoolPassword;
    json[JSON_SPIFFS_KEY_WALLETID]  = Settings->BtcWallet;
    json[JSON_SPIFFS_KEY_TIMEZONE]  = Settings->Timezone;
    json[JSON_SPIFFS_KEY_STATS2NV]  = Settings->saveStats;
    json[JSON_SPIFFS_KEY_INVCOLOR]  = Settings->invertColors;

    // Open config file
    File configFile = SPIFFS.open(JSON_CONFIG_FILE, "w");
    if (!configFile) {
        // Error, file did not open
        Serial.println("SPIFS: Failed to open config file for writing");
        return false;
    }

    // Serialize JSON data to write to file
    serializeJsonPretty(json, Serial);
    Serial.print('\n');
    size_t sz = serializeJson(json, configFile);
    if (sz == 0) {
        // Error writing file
        Serial.println(F("SPIFS: Failed to write to file"));
    }
    // Close file
    configFile.close();
    return (sz > 0);
}

/// @brief Load settings from config file located in SPIFFS.
/// @param TSettings* Struct to update with new settings.
/// @return true on success
bool nvMemory::loadConfig(TSettings* Settings) {
    // Uncomment if we need to format filesystem
    // SPIFFS.format();

    // Load existing configuration file
    // Read configuration from FS json

    if (!init()) return false;
    if (!SPIFFS.exists(JSON_CONFIG_FILE)) {
        Serial.println("SPIFS: No config file available!");
        return false;
    }

    // The file exists, reading and loading
    File configFile = SPIFFS.open(JSON_CONFIG_FILE, "r");
    if (!configFile) {
        Serial.println("SPIFS: Error opening config file!");
        return false;
    }

    Serial.println("SPIFS: Loading config file");
    StaticJsonDocument<512> json;
    DeserializationError error = deserializeJson(json, configFile);
    configFile.close();
    serializeJsonPretty(json, Serial);
    Serial.print('\n');
    if (error) {
        // Error loading JSON data
        Serial.println("SPIFS: Error parsing config file!");
        return false;
    }

    Settings->wifiSSID = json[JSON_SPIFFS_KEY_SSID] | Settings->wifiSSID;
    Settings->wifiPwd = json[JSON_SPIFFS_KEY_PWD] | Settings->wifiPwd;
    auto altWifi = json[JSON_SPIFFS_KEY_ALTWIFI].as<JsonArray>();
    if (altWifi) {
      auto cred = altWifi[0].as<JsonObject>();
      Settings->altWifi[0].SSID = cred["SSID"] | Settings->altWifi[0].SSID;
      Settings->altWifi[0].Pwd = cred["Pwd"] | Settings->altWifi[0].Pwd;
      cred = altWifi[1].as<JsonObject>();
      Settings->altWifi[1].SSID = cred["SSID"] | Settings->altWifi[1].SSID;
      Settings->altWifi[1].Pwd = cred["Pwd"] | Settings->altWifi[1].Pwd;
    }
    Settings->PoolAddress = json[JSON_SPIFFS_KEY_POOLURL] | Settings->PoolAddress;
    Settings->PoolPassword= json[JSON_SPIFFS_KEY_POOLPASS] | Settings->PoolPassword;
    Settings->BtcWallet= json[JSON_SPIFFS_KEY_WALLETID] | Settings->BtcWallet;
    if (json.containsKey(JSON_SPIFFS_KEY_POOLPORT))
        Settings->PoolPort = json[JSON_SPIFFS_KEY_POOLPORT].as<int>();
    if (json.containsKey(JSON_SPIFFS_KEY_TIMEZONE))
        Settings->Timezone = json[JSON_SPIFFS_KEY_TIMEZONE].as<int>();
    if (json.containsKey(JSON_SPIFFS_KEY_STATS2NV))
        Settings->saveStats = json[JSON_SPIFFS_KEY_STATS2NV].as<bool>();
    if (json.containsKey(JSON_SPIFFS_KEY_INVCOLOR)) {
        Settings->invertColors = json[JSON_SPIFFS_KEY_INVCOLOR].as<bool>();
    } else {
        Settings->invertColors = false;
    }
    return true;
}

/// @brief Delete config file from SPIFFS
/// @return true on successs
bool nvMemory::deleteConfig()
{
    Serial.println("SPIFS: Erasing config file..");
    return SPIFFS.remove(JSON_CONFIG_FILE); //Borramos fichero
}

/// @brief Prepare and mount SPIFFS
/// @return true on success
bool nvMemory::init()
{
    if (!Initialized_)
    {
        Serial.println("SPIFS: Mounting File System...");
        // May need to make it begin(true) first time you are using SPIFFS
        Initialized_ = SPIFFS.begin(false) || SPIFFS.begin(true);
        Initialized_ ? Serial.println("SPIFS: Mounted") : Serial.println("SPIFS: Mounting failed.");
    }
    else
    {
        Serial.println("SPIFS: Already Mounted");
    }
    return Initialized_;
};

#else

nvMemory::nvMemory() {}
nvMemory::~nvMemory() {}
bool nvMemory::saveConfig(TSettings* Settings) { return false; }
bool nvMemory::loadConfig(TSettings* Settings) { return false; }
bool nvMemory::deleteConfig() { return false; }
bool nvMemory::init() { return false; }


#endif //NVMEM_TYPE