#include "NetworkManager.h"

#include <WiFi.h>

#include "AppConstants.h"

void NetworkManager::begin(const WifiSettings& settings)
{
    currentSettings = settings;
    startProvisioning();

    if (!currentSettings.ssid.isEmpty())
    {
        connect();
    }
}

void NetworkManager::loop()
{
    connected = WiFi.status() == WL_CONNECTED;

    if (connected)
    {
        ipAddress = WiFi.localIP().toString();
        if (provisioning)
        {
            WiFi.softAPdisconnect(true);
            WiFi.mode(WIFI_STA);
            provisioning = false;
            Serial.print("WiFi connected, IP address: ");
            Serial.println(ipAddress);
        }
        return;
    }

    ipAddress = provisioning ? WiFi.softAPIP().toString() : "0.0.0.0";

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

bool NetworkManager::isProvisioning() const
{
    return provisioning;
}

String NetworkManager::getIpAddress() const
{
    return ipAddress;
}

const WifiSettings& NetworkManager::getCurrentSettings() const
{
    return currentSettings;
}

void NetworkManager::startProvisioning()
{
    const IPAddress localIp(192, 168, 4, 1);
    const IPAddress gateway(192, 168, 4, 1);
    const IPAddress subnet(255, 255, 255, 0);

    WiFi.mode(WIFI_AP_STA);
    WiFi.softAPConfig(localIp, gateway, subnet);
    provisioning = WiFi.softAP(AppConstants::ProvisioningSsid, AppConstants::ProvisioningPassword);
    ipAddress = provisioning ? WiFi.softAPIP().toString() : "0.0.0.0";

    if (provisioning)
    {
        Serial.print("Provisioning access point: ");
        Serial.println(AppConstants::ProvisioningSsid);
        Serial.print("Provisioning URL: http://");
        Serial.println(ipAddress);
    }
    else
    {
        Serial.println("Unable to start provisioning access point");
    }
}

void NetworkManager::connect()
{
    connected = false;
    ipAddress = "0.0.0.0";
    connectStartedAt = millis();

    if (currentSettings.ssid.isEmpty())
    {
        if (!provisioning)
        {
            startProvisioning();
        }
        return;
    }

    WiFi.mode(provisioning ? WIFI_AP_STA : WIFI_STA);
    WiFi.begin(currentSettings.ssid.c_str(), currentSettings.password.c_str());
}
