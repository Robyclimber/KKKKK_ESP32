#include "NetworkManager.h"

#include <WiFi.h>

#include "AppConstants.h"

void NetworkManager::begin(const WifiSettings& settings)
{
    currentSettings = settings;
    connect();
}

void NetworkManager::loop()
{
    connected = WiFi.status() == WL_CONNECTED;
    ipAddress = connected ? WiFi.localIP().toString() : "0.0.0.0";

    if (!connected &&
        !currentSettings.ssid.isEmpty() &&
        millis() - connectStartedAt > AppConstants::WifiConnectTimeoutMs)
    {
        connect();
    }
}

bool NetworkManager::applyWifiSettings(const WifiSettings& settings)
{
    currentSettings = settings;
    connect();
    return !currentSettings.ssid.isEmpty();
}

bool NetworkManager::isConnected() const
{
    return connected;
}

String NetworkManager::getIpAddress() const
{
    return ipAddress;
}

const WifiSettings& NetworkManager::getCurrentSettings() const
{
    return currentSettings;
}

void NetworkManager::connect()
{
    connected = false;
    ipAddress = "0.0.0.0";
    connectStartedAt = millis();

    if (currentSettings.ssid.isEmpty())
    {
        WiFi.disconnect(true, true);
        WiFi.mode(WIFI_OFF);
        return;
    }

    WiFi.mode(WIFI_STA);
    WiFi.begin(currentSettings.ssid.c_str(), currentSettings.password.c_str());
}
