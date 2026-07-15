#pragma once

#include <Arduino.h>
#include <Preferences.h>
#include <vector>

#include "DomainTypes.h"

struct WifiSettings
{
    String ssid;
    String password;
};

class SettingsStorage
{
public:
    void begin();

    bool hasWifiCredentials() const;
    WifiSettings loadWifiSettings() const;
    bool saveWifiSettings(const WifiSettings& settings);
    bool clearWifiSettings();
    bool saveWallConfig(const WallConfigDto& config);
    bool loadWallConfig(WallConfigDto& config) const;
    bool clearWallConfig();
    bool saveCircuits(const String& wallId, const std::vector<CircuitDefinitionDto>& circuits);
    bool loadCircuits(String& wallId, std::vector<CircuitDefinitionDto>& circuits) const;
    bool saveEditorialCircuits(const String& wallId, const std::vector<CircuitEditorialDefinitionDto>& circuits);
    bool loadEditorialCircuits(String& wallId, std::vector<CircuitEditorialDefinitionDto>& circuits) const;
    bool clearCircuits();

private:
    void loadWifiSettingsFromPreferences();

    mutable Preferences preferences;
    WifiSettings wifiSettings;
    bool wifiConfigured = false;
};
