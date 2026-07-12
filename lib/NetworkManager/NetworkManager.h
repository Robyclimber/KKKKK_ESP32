#pragma once

#include <Arduino.h>

#include "SettingsStorage.h"

class NetworkManager
{
public:
    void begin(const WifiSettings& settings);
    void loop();
    bool applyWifiSettings(const WifiSettings& settings);

    bool isConnected() const;
    bool isProvisioning() const;
    String getIpAddress() const;
    const WifiSettings& getCurrentSettings() const;

private:
    void startProvisioning();
    void connect();

    WifiSettings currentSettings;
    bool connected = false;
    bool provisioning = false;
    String ipAddress = "0.0.0.0";
    unsigned long connectStartedAt = 0UL;
};
