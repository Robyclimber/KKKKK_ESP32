#include "HttpServer.h"

#include <ArduinoJson.h>

void HttpServer::begin(RuntimeState* runtimeStateRef,
                       SettingsStorage* settingsStorageRef,
                       NetworkManager* networkManagerRef,
                       WallMapRepository* wallMapRepositoryRef,
                       CircuitRepository* circuitRepositoryRef,
                       CircuitController* circuitControllerRef)
{
    runtimeState = runtimeStateRef;
    settingsStorage = settingsStorageRef;
    networkManager = networkManagerRef;
    wallMapRepository = wallMapRepositoryRef;
    circuitRepository = circuitRepositoryRef;
    circuitController = circuitControllerRef;

    configureRoutes();
    server.begin();
    Serial.println("HTTP server listening on port 80");
}

void HttpServer::loop()
{
    server.handleClient();
}

void HttpServer::configureRoutes()
{
    server.on("/api/health", HTTP_GET, [this]() { handleHealth(); });
    server.on("/api/status", HTTP_GET, [this]() { handleStatus(); });
    server.on("/api/config", HTTP_GET, [this]() { handleGetConfig(); });
    server.on("/api/config", HTTP_POST, [this]() { handlePostConfig(); });
    server.on("/api/circuits", HTTP_GET, [this]() { handleGetCircuits(); });
    server.on("/api/circuits", HTTP_POST, [this]() { handlePostCircuits(); });
    server.on("/api/wifi/config", HTTP_POST, [this]() { handlePostWifiConfig(); });
    server.on("/api/circuit/show", HTTP_POST, [this]() { handlePostCircuitShow(); });
    server.on("/api/circuit/stop", HTTP_POST, [this]() { handlePostCircuitStop(); });
    server.on("/api/circuit/reset", HTTP_POST, [this]() { handlePostCircuitReset(); });
    server.on("/api/circuit/clear", HTTP_POST, [this]() { handlePostCircuitClear(); });
}

void HttpServer::handleHealth()
{
    const String dataJson =
        String("{\"status\":\"online\",\"firmwareVersion\":\"") +
        runtimeState->getFirmwareVersion() + "\"}";

    server.send(200, "application/json", buildSuccessResponse("ok", dataJson));
}

void HttpServer::handleStatus()
{
    const String configuredWallId =
        runtimeState->getConfiguredWallId().isEmpty() ? "null" : "\"" + runtimeState->getConfiguredWallId() + "\"";
    const String activeCircuitId =
        runtimeState->getActiveCircuitId().isEmpty() ? "null" : "\"" + runtimeState->getActiveCircuitId() + "\"";
    const String selectedCircuitId =
        runtimeState->getSelectedCircuitId().isEmpty() ? "null" : "\"" + runtimeState->getSelectedCircuitId() + "\"";
    const String lastError =
        runtimeState->getLastError().isEmpty() ? "null" : "\"" + runtimeState->getLastError() + "\"";

    String dataJson = "{";
    dataJson += "\"deviceName\":\"" + runtimeState->getDeviceName() + "\",";
    dataJson += "\"firmwareVersion\":\"" + runtimeState->getFirmwareVersion() + "\",";
    dataJson += "\"apiVersion\":\"" + runtimeState->getApiVersion() + "\",";
    dataJson += "\"runtimeState\":\"" + String(runtimeState->toString()) + "\",";
    dataJson += "\"wifiStatus\":\"" + String(runtimeState->wifiStatusToString()) + "\",";
    dataJson += "\"ipAddress\":\"" + runtimeState->getIpAddress() + "\",";
    dataJson += "\"uptimeSeconds\":" + String(runtimeState->getUptimeSeconds()) + ",";
    dataJson += "\"configuredWallId\":" + configuredWallId + ",";
    dataJson += "\"configStatus\":\"" + String(runtimeState->configStatusToString()) + "\",";
    dataJson += "\"circuitsCount\":" + String(runtimeState->getCircuitsCount()) + ",";
    dataJson += "\"circuitsStatus\":\"" + String(runtimeState->circuitsStatusToString()) + "\",";
    dataJson += "\"selectedCircuitId\":" + selectedCircuitId + ",";
    dataJson += "\"activeCircuitId\":" + activeCircuitId + ",";
    dataJson += "\"lastInputSource\":\"" + String(runtimeState->lastInputSourceToString()) + "\",";
    dataJson += "\"sequenceActive\":" + String(runtimeState->isSequenceActive() ? "true" : "false") + ",";
    dataJson += "\"currentStepIndex\":" + String(runtimeState->getCurrentStepIndex()) + ",";
    dataJson += "\"sequencePhase\":\"" + String(runtimeState->sequencePhaseToString()) + "\",";
    dataJson += "\"currentPhaseRemainingMs\":" + String(runtimeState->getCurrentPhaseRemainingMs()) + ",";
    dataJson += "\"ledStatus\":\"" + runtimeState->getLedStatus() + "\",";
    dataJson += "\"lastCommand\":\"" + String(runtimeState->lastCommandToString()) + "\",";
    dataJson += "\"lastError\":" + lastError;
    dataJson += "}";

    server.send(200, "application/json", buildSuccessResponse("ok", dataJson));
}

void HttpServer::handleGetConfig()
{
    const String wallIdJson =
        runtimeState->getConfiguredWallId().isEmpty() ? "null" : "\"" + runtimeState->getConfiguredWallId() + "\"";

    String dataJson = "{";
    dataJson += "\"wallId\":" + wallIdJson + ",";
    dataJson += "\"ledCount\":" + String(wallMapRepository->getLedCount()) + ",";
    dataJson += "\"disabledPoints\":" + String(wallMapRepository->getDisabledPoints());
    dataJson += "}";

    server.send(200, "application/json", buildSuccessResponse("ok", dataJson));
}

void HttpServer::handleGetCircuits()
{
    String dataJson = "{";
    dataJson += "\"wallId\":";
    dataJson += circuitRepository->getWallId().isEmpty() ? "null" : "\"" + circuitRepository->getWallId() + "\"";
    dataJson += ",";
    dataJson += "\"circuits\":[";

    const auto& circuits = circuitRepository->getCircuits();
    for (size_t index = 0; index < circuits.size(); index++)
    {
        if (index > 0)
        {
            dataJson += ",";
        }

        dataJson += "{";
        dataJson += "\"circuitId\":\"" + circuits[index].circuitId + "\",";
        dataJson += "\"name\":\"" + circuits[index].name + "\"";
        dataJson += "}";
    }

    dataJson += "]}";

    server.send(200, "application/json", buildSuccessResponse("ok", dataJson));
}

void HttpServer::handlePostWifiConfig()
{
    const auto body = server.arg("plain");
    JsonDocument document;
    const auto error = deserializeJson(document, body);
    if (error)
    {
        server.send(400, "application/json", buildErrorResponse("WIFI_CONFIG_INVALID", "Invalid JSON body"));
        return;
    }

    const auto ssid = String(document["ssid"] | "");
    const auto password = String(document["password"] | "");

    if (ssid.isEmpty())
    {
        server.send(400, "application/json", buildErrorResponse("WIFI_CONFIG_INVALID", "Missing ssid"));
        return;
    }

    WifiSettings settings;
    settings.ssid = ssid;
    settings.password = password;

    const auto saveOk = settingsStorage->saveWifiSettings(settings);
    if (!saveOk)
    {
        server.send(400, "application/json", buildErrorResponse("WIFI_CONFIG_INVALID", "Invalid WiFi settings"));
        return;
    }

    networkManager->applyWifiSettings(settings);
    runtimeState->setLastInputSource(RuntimeInputSource::App);
    runtimeState->setLastCommand(RuntimeLastCommand::WifiConfig);
    runtimeState->setState(RuntimeAppState::ConnectingWifi);
    runtimeState->clearLastError();

    const String dataJson = "{\"restartRequired\":false}";
    server.send(200, "application/json", buildSuccessResponse("WiFi settings saved", dataJson));
}

void HttpServer::handlePostConfig()
{
    WallConfigDto config;
    String validationError;
    const auto body = server.arg("plain");

    if (!parseWallConfig(body, config, validationError))
    {
        runtimeState->setLastError(validationError);
        server.send(400, "application/json", buildErrorResponse("CONFIG_INVALID", validationError));
        return;
    }

    if (!wallMapRepository->setConfig(config, validationError))
    {
        runtimeState->setLastError(validationError);
        server.send(400, "application/json", buildErrorResponse("CONFIG_INVALID", validationError));
        return;
    }

    circuitRepository->clear();
    settingsStorage->saveWallConfig(config);
    settingsStorage->clearCircuits();
    runtimeState->setLastInputSource(RuntimeInputSource::App);
    runtimeState->setLastCommand(RuntimeLastCommand::SaveConfig);
    runtimeState->clearLastError();
    if (runtimeState->getState() == RuntimeAppState::Ready || runtimeState->getState() == RuntimeAppState::Idle)
    {
        runtimeState->setState(RuntimeAppState::Idle);
    }

    String dataJson = "{";
    dataJson += "\"wallId\":\"" + config.wallId + "\",";
    dataJson += "\"pointsAccepted\":" + String(static_cast<int>(config.points.size())) + ",";
    dataJson += "\"pointsDisabled\":" + String(wallMapRepository->getDisabledPoints()) + ",";
    dataJson += "\"circuitsInvalidated\":true";
    dataJson += "}";

    server.send(200, "application/json", buildSuccessResponse("Configuration saved", dataJson));
}

void HttpServer::handlePostCircuits()
{
    const auto body = server.arg("plain");
    String wallId;
    String validationError;
    std::vector<CircuitDefinitionDto> circuits;

    if (!wallMapRepository->hasConfig())
    {
        server.send(409, "application/json", buildErrorResponse("CONFIG_NOT_LOADED", "Wall config not loaded"));
        return;
    }

    if (!parseCircuitsPayload(body, wallId, circuits, validationError))
    {
        runtimeState->setLastError(validationError);
        server.send(400, "application/json", buildErrorResponse("CIRCUIT_INVALID", validationError));
        return;
    }

    if (wallId != wallMapRepository->getWallId())
    {
        runtimeState->setLastError("wallId mismatch");
        server.send(400, "application/json", buildErrorResponse("WALL_ID_MISMATCH", "wallId mismatch"));
        return;
    }

    for (const auto& circuit : circuits)
    {
        for (const auto& item : circuit.items)
        {
            if (!wallMapRepository->hasPointId(item.pointId))
            {
                runtimeState->setLastError("pointId not found");
                server.send(400, "application/json", buildErrorResponse("POINT_NOT_FOUND", "Circuit references unknown pointId"));
                return;
            }
        }

        for (const auto& step : circuit.steps)
        {
            if (!wallMapRepository->hasPointId(step.pointId))
            {
                runtimeState->setLastError("pointId not found");
                server.send(400, "application/json", buildErrorResponse("POINT_NOT_FOUND", "Circuit step references unknown pointId"));
                return;
            }
        }
    }

    if (!circuitRepository->setCircuits(wallId, circuits, validationError))
    {
        runtimeState->setLastError(validationError);
        server.send(400, "application/json", buildErrorResponse("CIRCUIT_INVALID", validationError));
        return;
    }

    settingsStorage->saveCircuits(wallId, circuits);
    runtimeState->setLastInputSource(RuntimeInputSource::App);
    runtimeState->setLastCommand(RuntimeLastCommand::SyncCircuits);
    runtimeState->clearLastError();

    String dataJson = "{";
    dataJson += "\"circuitsAccepted\":" + String(circuitRepository->getCircuitsCount()) + ",";
    dataJson += "\"circuitsRejected\":0";
    dataJson += "}";

    server.send(200, "application/json", buildSuccessResponse("Circuits synchronized", dataJson));
}

void HttpServer::handlePostCircuitShow()
{
    if (!circuitRepository->hasCircuits())
    {
        server.send(409, "application/json", buildErrorResponse("CIRCUITS_NOT_LOADED", "Circuits not loaded"));
        return;
    }

    const auto body = server.arg("plain");
    JsonDocument document;
    const auto error = deserializeJson(document, body);
    if (error)
    {
        server.send(400, "application/json", buildErrorResponse("CIRCUIT_ID_MISSING", "Invalid JSON body"));
        return;
    }

    const auto circuitId = String(document["circuitId"] | "");
    if (circuitId.isEmpty())
    {
        server.send(400, "application/json", buildErrorResponse("CIRCUIT_ID_MISSING", "Missing circuitId"));
        return;
    }

    runtimeState->setLastInputSource(RuntimeInputSource::App);
    if (!circuitController->show(circuitId))
    {
        const auto* circuit = circuitRepository->findById(circuitId);
        if (circuit == nullptr)
        {
            runtimeState->setLastError("circuitId not found");
            server.send(404, "application/json", buildErrorResponse("CIRCUIT_NOT_FOUND", "Circuit not found"));
            return;
        }

        server.send(500, "application/json", buildErrorResponse("CIRCUIT_SHOW_FAILED", "Unable to show circuit"));
        return;
    }

    const auto* circuit = circuitRepository->findById(circuitId);
    String dataJson = "{";
    dataJson += "\"circuitId\":\"" + circuitId + "\",";
    dataJson += "\"name\":\"" + circuit->name + "\"";
    dataJson += "}";

    server.send(200, "application/json", buildSuccessResponse("Circuit activated", dataJson));
}

void HttpServer::handlePostCircuitStop()
{
    runtimeState->setLastInputSource(RuntimeInputSource::App);
    if (!circuitController->stop())
    {
        server.send(500, "application/json", buildErrorResponse("CIRCUIT_STOP_FAILED", "Unable to stop circuit"));
        return;
    }

    const String dataJson = "{\"activeCircuitId\":null}";
    server.send(200, "application/json", buildSuccessResponse("Circuit stopped", dataJson));
}

void HttpServer::handlePostCircuitReset()
{
    runtimeState->setLastInputSource(RuntimeInputSource::App);
    if (!circuitController->reset())
    {
        const String lastError = runtimeState->getLastError();
        if (lastError == "no active circuit")
        {
            server.send(409, "application/json", buildErrorResponse("NO_ACTIVE_CIRCUIT", "No active circuit"));
            return;
        }

        server.send(500, "application/json", buildErrorResponse("CIRCUIT_RESET_FAILED", "Unable to reset circuit"));
        return;
    }

    const String activeCircuitId =
        circuitController->getActiveCircuitId().isEmpty() ? "null" : "\"" + circuitController->getActiveCircuitId() + "\"";
    const String dataJson = "{\"activeCircuitId\":" + activeCircuitId + "}";
    server.send(200, "application/json", buildSuccessResponse("Circuit reset", dataJson));
}

void HttpServer::handlePostCircuitClear()
{
    runtimeState->setLastInputSource(RuntimeInputSource::App);
    if (!circuitController->clear())
    {
        server.send(500, "application/json", buildErrorResponse("CIRCUIT_CLEAR_FAILED", "Unable to clear circuit"));
        return;
    }

    const String dataJson = "{\"activeCircuitId\":null}";
    server.send(200, "application/json", buildSuccessResponse("Circuit cleared", dataJson));
}

String HttpServer::buildSuccessResponse(const String& message, const String& dataJson) const
{
    String payload = "{";
    payload += "\"success\":true,";
    payload += "\"message\":\"" + message + "\",";
    payload += "\"data\":" + dataJson + ",";
    payload += "\"apiVersion\":\"" + runtimeState->getApiVersion() + "\"";
    payload += "}";
    return payload;
}

String HttpServer::buildErrorResponse(const String& errorCode, const String& message) const
{
    String payload = "{";
    payload += "\"success\":false,";
    payload += "\"errorCode\":\"" + errorCode + "\",";
    payload += "\"message\":\"" + message + "\",";
    payload += "\"apiVersion\":\"" + runtimeState->getApiVersion() + "\"";
    payload += "}";
    return payload;
}

bool HttpServer::parseWallConfig(const String& body, WallConfigDto& config, String& validationError) const
{
    config = {};
    JsonDocument document;
    const auto error = deserializeJson(document, body);
    if (error)
    {
        validationError = "invalid JSON body";
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
    if (points.isNull())
    {
        validationError = "points array missing";
        return false;
    }

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

    if (config.points.empty())
    {
        validationError = "points array empty";
        return false;
    }

    validationError = "";
    return true;
}

bool HttpServer::parseCircuitsPayload(const String& body,
                                      String& wallId,
                                      std::vector<CircuitDefinitionDto>& circuits,
                                      String& validationError) const
{
    circuits.clear();
    JsonDocument document;
    const auto error = deserializeJson(document, body);
    if (error)
    {
        validationError = "invalid JSON body";
        return false;
    }

    wallId = String(document["wallId"] | "");

    JsonArrayConst circuitsJson = document["circuits"].as<JsonArrayConst>();
    if (circuitsJson.isNull())
    {
        validationError = "circuits array missing";
        return false;
    }

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
        if (!itemsJson.isNull())
        {
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
        }

        JsonArrayConst stepsJson = circuitJson["steps"].as<JsonArrayConst>();
        if (!stepsJson.isNull())
        {
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
        }

        circuits.push_back(circuit);
    }

    validationError = "";
    return !circuits.empty();
}
