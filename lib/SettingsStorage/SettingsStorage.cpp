#include "SettingsStorage.h"

#include <ArduinoJson.h>

void SettingsStorage::begin()
{
    preferences.begin("kkkkk-fw", false);
    loadWifiSettingsFromPreferences();
}

bool SettingsStorage::hasWifiCredentials() const
{
    return wifiConfigured && !wifiSettings.ssid.isEmpty();
}

WifiSettings SettingsStorage::loadWifiSettings() const
{
    return wifiSettings;
}

bool SettingsStorage::saveWifiSettings(const WifiSettings& settings)
{
    if (settings.ssid.isEmpty())
    {
        return false;
    }

    wifiSettings = settings;
    wifiConfigured = !settings.ssid.isEmpty();
    preferences.putString("wifi_ssid", wifiSettings.ssid);
    preferences.putString("wifi_pwd", wifiSettings.password);
    preferences.putBool("wifi_cfg", wifiConfigured);
    return wifiConfigured;
}

bool SettingsStorage::clearWifiSettings()
{
    wifiSettings = {};
    wifiConfigured = false;
    preferences.remove("wifi_ssid");
    preferences.remove("wifi_pwd");
    preferences.putBool("wifi_cfg", false);
    return true;
}

void SettingsStorage::loadWifiSettingsFromPreferences()
{
    wifiConfigured = preferences.getBool("wifi_cfg", false);
    wifiSettings.ssid = preferences.getString("wifi_ssid", "");
    wifiSettings.password = preferences.getString("wifi_pwd", "");

    if (wifiSettings.ssid.isEmpty())
    {
        wifiConfigured = false;
    }
}

bool SettingsStorage::saveWallConfig(const WallConfigDto& config)
{
    JsonDocument document;
    document["wallId"] = config.wallId;
    document["wallName"] = config.wallName;
    document["roomId"] = config.roomId;
    document["roomName"] = config.roomName;
    document["controllerId"] = config.controllerId;
    document["ledCount"] = config.ledCount;
    document["brightnessLimit"] = config.brightnessLimit;

    JsonArray points = document["points"].to<JsonArray>();
    for (const auto& point : config.points)
    {
        JsonObject pointJson = points.add<JsonObject>();
        pointJson["pointId"] = point.pointId;
        pointJson["holeNumber"] = point.holeNumber;
        pointJson["panelName"] = point.panelName;
        pointJson["ledIndex"] = point.ledIndex;
        pointJson["x"] = point.x;
        pointJson["y"] = point.y;
        pointJson["enabled"] = point.enabled;
        pointJson["kind"] = pointKindToString(point.kind);
    }

    String json;
    serializeJson(document, json);
    preferences.putString("wall_cfg", json);
    preferences.putBool("wall_cfg_ok", true);
    return true;
}

bool SettingsStorage::loadWallConfig(WallConfigDto& config) const
{
    config = {};
    if (!preferences.getBool("wall_cfg_ok", false))
    {
        return false;
    }

    const String json = preferences.getString("wall_cfg", "");
    if (json.isEmpty())
    {
        return false;
    }

    JsonDocument document;
    const auto error = deserializeJson(document, json);
    if (error)
    {
        return false;
    }

    config.wallId = String(document["wallId"] | "");
    config.wallName = String(document["wallName"] | "");
    config.roomId = String(document["roomId"] | "");
    config.roomName = String(document["roomName"] | "");
    config.controllerId = String(document["controllerId"] | "");
    config.ledCount = document["ledCount"] | 0;
    config.brightnessLimit = document["brightnessLimit"] | 0;

    JsonArrayConst points = document["points"].as<JsonArrayConst>();
    for (JsonObjectConst pointJson : points)
    {
        LedPointDto point;
        point.pointId = String(pointJson["pointId"] | "");
        point.holeNumber = pointJson["holeNumber"] | -1;
        point.panelName = String(pointJson["panelName"] | "");
        point.ledIndex = pointJson["ledIndex"] | -1;
        point.x = pointJson["x"] | 0.0f;
        point.y = pointJson["y"] | 0.0f;
        point.enabled = pointJson["enabled"] | true;
        point.kind = pointKindFromString(String(pointJson["kind"] | ""));
        config.points.push_back(point);
    }

    return !config.wallId.isEmpty();
}

bool SettingsStorage::clearWallConfig()
{
    preferences.remove("wall_cfg");
    preferences.putBool("wall_cfg_ok", false);
    return true;
}

bool SettingsStorage::saveCircuits(const String& wallId, const std::vector<CircuitDefinitionDto>& circuits)
{
    JsonDocument document;
    document["wallId"] = wallId;

    JsonArray circuitsJson = document["circuits"].to<JsonArray>();
    for (const auto& circuit : circuits)
    {
        JsonObject circuitJson = circuitsJson.add<JsonObject>();
        circuitJson["circuitId"] = circuit.circuitId;
        circuitJson["name"] = circuit.name;
        circuitJson["defaultColor"] = circuit.style.defaultColor;
        circuitJson["brightness"] = circuit.style.brightness;
        circuitJson["effect"] = visualEffectToString(circuit.style.effect);
        circuitJson["fadeInMs"] = circuit.style.fadeInMs;
        circuitJson["fadeOutMs"] = circuit.style.fadeOutMs;
        circuitJson["blinkPeriodMs"] = circuit.style.blinkPeriodMs;

        JsonArray itemsJson = circuitJson["items"].to<JsonArray>();
        for (const auto& item : circuit.items)
        {
            JsonObject itemJson = itemsJson.add<JsonObject>();
            itemJson["pointId"] = item.pointId;
            itemJson["role"] = circuitRoleToString(item.role);
            itemJson["color"] = item.color;
            itemJson["effect"] = visualEffectToString(item.effect);
            itemJson["enabled"] = item.enabled;
        }

        JsonArray stepsJson = circuitJson["steps"].to<JsonArray>();
        for (const auto& step : circuit.steps)
        {
            JsonObject stepJson = stepsJson.add<JsonObject>();
            stepJson["pointId"] = step.pointId;
            stepJson["orderIndex"] = step.orderIndex;
            stepJson["blinkCount"] = step.blinkCount;
            stepJson["blinkPeriodMs"] = step.blinkPeriodMs;
            stepJson["highlightBrightness"] = step.highlightBrightness;
            stepJson["holdDurationMs"] = step.holdDurationMs;
            stepJson["dimmedBrightness"] = step.dimmedBrightness;
            stepJson["highlightColor"] = step.highlightColor;
            stepJson["dimmedColor"] = step.dimmedColor;
            stepJson["autoAdvance"] = step.autoAdvance;
            stepJson["enabled"] = step.enabled;
        }
    }

    String json;
    serializeJson(document, json);
    preferences.putString("circuits_cfg", json);
    preferences.putBool("circuits_ok", true);
    return true;
}

bool SettingsStorage::loadCircuits(String& wallId, std::vector<CircuitDefinitionDto>& circuits) const
{
    wallId = "";
    circuits.clear();

    if (!preferences.getBool("circuits_ok", false))
    {
        return false;
    }

    const String json = preferences.getString("circuits_cfg", "");
    if (json.isEmpty())
    {
        return false;
    }

    JsonDocument document;
    const auto error = deserializeJson(document, json);
    if (error)
    {
        return false;
    }

    wallId = String(document["wallId"] | "");
    JsonArrayConst circuitsJson = document["circuits"].as<JsonArrayConst>();
    for (JsonObjectConst circuitJson : circuitsJson)
    {
        CircuitDefinitionDto circuit;
        circuit.circuitId = String(circuitJson["circuitId"] | "");
        circuit.name = String(circuitJson["name"] | "");
        circuit.wallId = wallId;
        circuit.style.defaultColor = String(circuitJson["defaultColor"] | "");
        circuit.style.brightness = circuitJson["brightness"] | -1;
        circuit.style.effect = visualEffectFromString(String(circuitJson["effect"] | ""));
        circuit.style.fadeInMs = circuitJson["fadeInMs"] | 0;
        circuit.style.fadeOutMs = circuitJson["fadeOutMs"] | 0;
        circuit.style.blinkPeriodMs = circuitJson["blinkPeriodMs"] | 0;

        JsonArrayConst itemsJson = circuitJson["items"].as<JsonArrayConst>();
        for (JsonObjectConst itemJson : itemsJson)
        {
            CircuitItemDto item;
            item.pointId = String(itemJson["pointId"] | "");
            item.role = circuitRoleFromString(String(itemJson["role"] | ""));
            item.color = String(itemJson["color"] | "");
            item.effect = visualEffectFromString(String(itemJson["effect"] | ""));
            item.enabled = itemJson["enabled"] | true;
            circuit.items.push_back(item);
        }

        JsonArrayConst stepsJson = circuitJson["steps"].as<JsonArrayConst>();
        for (JsonObjectConst stepJson : stepsJson)
        {
            CircuitStepDto step;
            step.pointId = String(stepJson["pointId"] | "");
            step.orderIndex = stepJson["orderIndex"] | -1;
            step.blinkCount = stepJson["blinkCount"] | 0;
            step.blinkPeriodMs = stepJson["blinkPeriodMs"] | 0;
            step.highlightBrightness = stepJson["highlightBrightness"] | -1;
            step.holdDurationMs = stepJson["holdDurationMs"] | 0;
            step.dimmedBrightness = stepJson["dimmedBrightness"] | -1;
            step.highlightColor = String(stepJson["highlightColor"] | "");
            step.dimmedColor = String(stepJson["dimmedColor"] | "");
            step.autoAdvance = stepJson["autoAdvance"] | true;
            step.enabled = stepJson["enabled"] | true;
            circuit.steps.push_back(step);
        }

        circuits.push_back(circuit);
    }

    return !wallId.isEmpty() && !circuits.empty();
}

bool SettingsStorage::clearCircuits()
{
    preferences.remove("circuits_cfg");
    preferences.putBool("circuits_ok", false);
    return true;
}
