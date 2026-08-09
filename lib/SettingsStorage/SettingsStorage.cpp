#include "SettingsStorage.h"

#include <ArduinoJson.h>
#include <LittleFS.h>

namespace
{
constexpr const char* WallConfigPath = "/wall_config.json";
constexpr const char* CircuitsPath = "/circuits.json";
constexpr const char* EditorialCircuitsPath = "/circuits_editorial.json";
}

void SettingsStorage::begin()
{
    preferences.begin("kkkkk-fw", false);
    LittleFS.begin(true);
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
    File file = LittleFS.open(WallConfigPath, "w");
    if (!file)
    {
        return false;
    }

    // Serializing hundreds of points in one JsonDocument causes a large peak
    // allocation. Write the document incrementally so config sync fits in RAM.
    auto writeJsonString = [&file](const String& value) {
        JsonDocument document;
        document.set(value);
        return serializeJson(document, file) > 0;
    };

    bool saved = file.print("{\"wallId\":") > 0;
    saved = saved && writeJsonString(config.wallId);
    saved = saved && file.print(",\"wallName\":") > 0;
    saved = saved && writeJsonString(config.wallName);
    saved = saved && file.print(",\"roomId\":") > 0;
    saved = saved && writeJsonString(config.roomId);
    saved = saved && file.print(",\"roomName\":") > 0;
    saved = saved && writeJsonString(config.roomName);
    saved = saved && file.print(",\"controllerId\":") > 0;
    saved = saved && writeJsonString(config.controllerId);
    saved = saved && file.printf(",\"ledCount\":%d,\"brightnessLimit\":%d,\"points\":[", config.ledCount, config.brightnessLimit) > 0;

    for (size_t index = 0; saved && index < config.points.size(); index++)
    {
        if (index > 0) saved = file.print(',') > 0;
        const auto& point = config.points[index];
        JsonDocument pointDocument;
        pointDocument["pointId"] = point.pointId;
        pointDocument["holeNumber"] = point.holeNumber;
        pointDocument["panelName"] = point.panelName;
        pointDocument["ledIndex"] = point.ledIndex;
        pointDocument["x"] = point.x;
        pointDocument["y"] = point.y;
        pointDocument["enabled"] = point.enabled;
        pointDocument["kind"] = pointKindToString(point.kind);
        saved = serializeJson(pointDocument, file) > 0;
    }

    saved = saved && file.print("]}") > 0;
    file.close();
    preferences.putBool("wall_cfg_ok", saved);
    if (saved)
    {
        preferences.remove("wall_cfg");
    }
    return saved;
}

bool SettingsStorage::loadWallConfig(WallConfigDto& config) const
{
    config = {};
    if (!preferences.getBool("wall_cfg_ok", false))
    {
        return false;
    }

    JsonDocument document;
    DeserializationError error;
    File file = LittleFS.open(WallConfigPath, "r");
    if (file)
    {
        error = deserializeJson(document, file);
        file.close();
    }
    else
    {
        const String legacyJson = preferences.getString("wall_cfg", "");
        if (legacyJson.isEmpty())
        {
            return false;
        }
        error = deserializeJson(document, legacyJson);
    }
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
    LittleFS.remove(WallConfigPath);
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

    File file = LittleFS.open(CircuitsPath, "w");
    if (!file)
    {
        return false;
    }

    const auto bytesWritten = serializeJson(document, file);
    file.close();
    const bool saved = bytesWritten > 0;
    preferences.putBool("circuits_ok", saved);
    if (saved)
    {
        preferences.remove("circuits_cfg");
    }
    return saved;
}

bool SettingsStorage::loadCircuits(String& wallId, std::vector<CircuitDefinitionDto>& circuits) const
{
    wallId = "";
    circuits.clear();

    if (!preferences.getBool("circuits_ok", false))
    {
        return false;
    }

    JsonDocument document;
    DeserializationError error;
    File file = LittleFS.open(CircuitsPath, "r");
    if (file)
    {
        error = deserializeJson(document, file);
        file.close();
    }
    else
    {
        const String legacyJson = preferences.getString("circuits_cfg", "");
        if (legacyJson.isEmpty())
        {
            return false;
        }
        error = deserializeJson(document, legacyJson);
    }
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

bool SettingsStorage::saveEditorialCircuits(const String& wallId, const std::vector<CircuitEditorialDefinitionDto>& circuits)
{
    JsonDocument document;
    document["wallId"] = wallId;

    JsonArray circuitsJson = document["circuits"].to<JsonArray>();
    for (const auto& circuit : circuits)
    {
        JsonObject circuitJson = circuitsJson.add<JsonObject>();
        circuitJson["circuitId"] = circuit.circuitId;
        circuitJson["name"] = circuit.name;
        circuitJson["wallId"] = circuit.wallId;
        circuitJson["difficulty"] = circuit.difficulty;
        circuitJson["inclination"] = circuit.inclination;

        JsonObject globalsJson = circuitJson["globals"].to<JsonObject>();
        globalsJson["presetName"] = circuit.globals.presetName;
        globalsJson["effect"] = visualEffectToString(circuit.globals.effect);
        globalsJson["defaultBrightness"] = circuit.globals.defaultBrightness;
        globalsJson["dimmedBrightness"] = circuit.globals.dimmedBrightness;
        globalsJson["rightHandColor"] = circuit.globals.rightHandColor;
        globalsJson["leftHandColor"] = circuit.globals.leftHandColor;
        globalsJson["startColor"] = circuit.globals.startColor;
        globalsJson["topColor"] = circuit.globals.topColor;
        globalsJson["blinkCount"] = circuit.globals.blinkCount;
        globalsJson["blinkPeriodMs"] = circuit.globals.blinkPeriodMs;
        globalsJson["holdDurationMs"] = circuit.globals.holdDurationMs;

        JsonArray movementsJson = circuitJson["movements"].to<JsonArray>();
        for (const auto& movement : circuit.movements)
        {
            JsonObject movementJson = movementsJson.add<JsonObject>();
            movementJson["p"] = movement.pointRef;
            movementJson["h"] = movement.hand;
            movementJson["r"] = movement.role;
            movementJson["s"] = movement.sequence;
        }
    }

    File file = LittleFS.open(EditorialCircuitsPath, "w");
    if (!file)
    {
        return false;
    }

    const auto bytesWritten = serializeJson(document, file);
    file.close();
    const bool saved = bytesWritten > 0;
    preferences.putBool("circuits_editorial_ok", saved);
    if (saved)
    {
        preferences.remove("circuits_editorial_cfg");
    }
    return saved;
}

bool SettingsStorage::loadEditorialCircuits(String& wallId, std::vector<CircuitEditorialDefinitionDto>& circuits) const
{
    wallId = "";
    circuits.clear();

    if (!preferences.getBool("circuits_editorial_ok", false))
    {
        return false;
    }

    JsonDocument document;
    DeserializationError error;
    File file = LittleFS.open(EditorialCircuitsPath, "r");
    if (file)
    {
        error = deserializeJson(document, file);
        file.close();
    }
    else
    {
        const String legacyJson = preferences.getString("circuits_editorial_cfg", "");
        if (legacyJson.isEmpty())
        {
            return false;
        }
        error = deserializeJson(document, legacyJson);
    }
    if (error)
    {
        return false;
    }

    wallId = String(document["wallId"] | "");
    JsonArrayConst circuitsJson = document["circuits"].as<JsonArrayConst>();
    for (JsonObjectConst circuitJson : circuitsJson)
    {
        CircuitEditorialDefinitionDto circuit;
        circuit.circuitId = String(circuitJson["circuitId"] | "");
        circuit.name = String(circuitJson["name"] | "");
        circuit.wallId = String(circuitJson["wallId"] | wallId);
        circuit.difficulty = String(circuitJson["difficulty"] | "");
        circuit.inclination = String(circuitJson["inclination"] | "");

        JsonObjectConst globalsJson = circuitJson["globals"].as<JsonObjectConst>();
        if (!globalsJson.isNull())
        {
            circuit.globals.presetName = String(globalsJson["presetName"] | "");
            circuit.globals.effect = visualEffectFromString(String(globalsJson["effect"] | "steady"));
            circuit.globals.defaultBrightness = globalsJson["defaultBrightness"] | 96;
            circuit.globals.dimmedBrightness = globalsJson["dimmedBrightness"] | 48;
            circuit.globals.rightHandColor = String(globalsJson["rightHandColor"] | "#C44536");
            circuit.globals.leftHandColor = String(globalsJson["leftHandColor"] | "#247BA0");
            circuit.globals.startColor = String(globalsJson["startColor"] | "#FFFF00");
            circuit.globals.topColor = String(globalsJson["topColor"] | "#FF0000");
            circuit.globals.blinkCount = globalsJson["blinkCount"] | 3;
            circuit.globals.blinkPeriodMs = globalsJson["blinkPeriodMs"] | 250;
            circuit.globals.holdDurationMs = globalsJson["holdDurationMs"] | 2500;
        }

        JsonArrayConst movementsJson = circuitJson["movements"].as<JsonArrayConst>();
        for (JsonObjectConst movementJson : movementsJson)
        {
            CircuitMovementEditorialDto movement;
            movement.pointRef = movementJson["p"] | -1;
            movement.hand = movementJson["h"] | -1;
            movement.role = movementJson["r"] | -1;
            movement.sequence = movementJson["s"] | -1;
            circuit.movements.push_back(movement);
        }

        circuits.push_back(circuit);
    }

    return !wallId.isEmpty() && !circuits.empty();
}

bool SettingsStorage::clearCircuits()
{
    preferences.remove("circuits_cfg");
    LittleFS.remove(CircuitsPath);
    preferences.putBool("circuits_ok", false);
    preferences.remove("circuits_editorial_cfg");
    LittleFS.remove(EditorialCircuitsPath);
    preferences.putBool("circuits_editorial_ok", false);
    return true;
}
