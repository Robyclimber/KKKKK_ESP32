#include "HttpServer.h"

#include <algorithm>

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
    server.on("/", HTTP_GET, [this]() { handleProvisioningPage(); });
    server.on("/api/health", HTTP_GET, [this]() { handleHealth(); });
    server.on("/api/status", HTTP_GET, [this]() { handleStatus(); });
    server.on("/api/config", HTTP_GET, [this]() { handleGetConfig(); });
    server.on("/api/config", HTTP_POST, [this]() { handlePostConfig(); });
    server.on("/api/circuits", HTTP_GET, [this]() { handleGetCircuits(); });
    server.on("/api/circuits", HTTP_POST, [this]() { handlePostCircuits(); });
    server.on("/api/circuits/editorial", HTTP_GET, [this]() { handleGetEditorialCircuits(); });
    server.on("/api/circuits/editorial", HTTP_POST, [this]() { handlePostEditorialCircuits(); });
    server.on("/api/wifi/config", HTTP_POST, [this]() { handlePostWifiConfig(); });
    server.on("/api/circuit/visualize", HTTP_POST, [this]() { handlePostCircuitVisualize(); });
    server.on("/api/circuit/start", HTTP_POST, [this]() { handlePostCircuitStart(); });
    server.on("/api/circuit/show", HTTP_POST, [this]() { handlePostCircuitShow(); });
    server.on("/api/circuit/stop", HTTP_POST, [this]() { handlePostCircuitStop(); });
    server.on("/api/circuit/reset", HTTP_POST, [this]() { handlePostCircuitReset(); });
    server.on("/api/circuit/clear", HTTP_POST, [this]() { handlePostCircuitClear(); });
    server.on("/api/test/random-sequence", HTTP_POST, [this]() { handlePostRandomSequenceTest(); });
}

void HttpServer::handleProvisioningPage()
{
    static const char page[] = R"HTML(
<!doctype html>
<html lang="it">
<head>
  <meta charset="utf-8">
  <meta name="viewport" content="width=device-width,initial-scale=1">
  <title>KKKKK ESP32 - Configurazione Wi-Fi</title>
  <style>
    body{font-family:system-ui,sans-serif;background:#111827;color:#f9fafb;margin:0;padding:24px}
    main{max-width:420px;margin:8vh auto;background:#1f2937;padding:28px;border-radius:16px}
    h1{font-size:1.4rem;margin-top:0}label{display:block;margin-top:16px}
    input,button{box-sizing:border-box;width:100%;padding:12px;margin-top:6px;border-radius:8px;border:1px solid #4b5563}
    input{background:#111827;color:#fff}button{background:#22c55e;color:#052e16;font-weight:700;cursor:pointer}
    #message{min-height:24px;margin-top:18px;color:#86efac}.hint{color:#9ca3af;font-size:.9rem}
  </style>
</head>
<body><main>
  <h1>Configurazione Wi-Fi</h1>
  <p class="hint">Inserisci la rete alla quale deve collegarsi KKKKK-ESP32.</p>
  <form id="wifiForm">
    <label>Nome rete (SSID)<input id="ssid" autocomplete="off" required></label>
    <label>Password<input id="password" type="password"></label>
    <button type="submit">Salva e collega</button>
  </form>
  <div id="message"></div>
</main>
<script>
const form=document.getElementById('wifiForm');
const message=document.getElementById('message');
form.addEventListener('submit',async event=>{
  event.preventDefault();
  message.textContent='Salvataggio in corso...';
  try{
    const response=await fetch('/api/wifi/config',{method:'POST',headers:{'Content-Type':'application/json'},body:JSON.stringify({ssid:document.getElementById('ssid').value,password:document.getElementById('password').value})});
    const result=await response.json();
    if(!response.ok) throw new Error(result.message||'Configurazione non riuscita');
    message.textContent='Dati salvati. La ESP32 si sta collegando alla rete.';
  }catch(error){message.textContent='Errore: '+error.message;}
});
</script></body></html>
)HTML";

    server.send(200, "text/html; charset=utf-8", page);
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

void HttpServer::handleGetEditorialCircuits()
{
    String wallId;
    std::vector<CircuitEditorialDefinitionDto> circuits;

    if (!settingsStorage->loadEditorialCircuits(wallId, circuits))
    {
        server.send(409, "application/json", buildErrorResponse("EDITORIAL_CIRCUITS_NOT_LOADED", "Editorial circuits not loaded"));
        return;
    }

    server.send(200, "application/json", buildSuccessResponse("ok", buildEditorialCircuitsDataJson(wallId, circuits)));
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
    std::vector<CircuitEditorialDefinitionDto> editorialCircuits;
    if (tryConvertRuntimeToEditorial(circuits, editorialCircuits, validationError))
    {
        settingsStorage->saveEditorialCircuits(wallId, editorialCircuits);
    }
    runtimeState->setLastInputSource(RuntimeInputSource::App);
    runtimeState->setLastCommand(RuntimeLastCommand::SyncCircuits);
    runtimeState->clearLastError();

    String dataJson = "{";
    dataJson += "\"circuitsAccepted\":" + String(circuitRepository->getCircuitsCount()) + ",";
    dataJson += "\"circuitsRejected\":0";
    dataJson += "}";

    server.send(200, "application/json", buildSuccessResponse("Circuits synchronized", dataJson));
}

void HttpServer::handlePostEditorialCircuits()
{
    const auto body = server.arg("plain");
    String wallId;
    String validationError;
    std::vector<CircuitEditorialDefinitionDto> editorialCircuits;

    if (!wallMapRepository->hasConfig())
    {
        server.send(409, "application/json", buildErrorResponse("CONFIG_NOT_LOADED", "Wall config not loaded"));
        return;
    }

    if (!parseEditorialCircuitsPayload(body, wallId, editorialCircuits, validationError))
    {
        runtimeState->setLastError(validationError);
        server.send(400, "application/json", buildErrorResponse("EDITORIAL_CIRCUIT_INVALID", validationError));
        return;
    }

    if (wallId != wallMapRepository->getWallId())
    {
        runtimeState->setLastError("wallId mismatch");
        server.send(400, "application/json", buildErrorResponse("WALL_ID_MISMATCH", "wallId mismatch"));
        return;
    }

    std::vector<CircuitDefinitionDto> runtimeCircuits;
    if (!tryConvertEditorialToRuntime(editorialCircuits, runtimeCircuits, validationError))
    {
        runtimeState->setLastError(validationError);
        server.send(400, "application/json", buildErrorResponse("EDITORIAL_CIRCUIT_INVALID", validationError));
        return;
    }

    if (!circuitRepository->setCircuits(wallId, runtimeCircuits, validationError))
    {
        runtimeState->setLastError(validationError);
        server.send(400, "application/json", buildErrorResponse("CIRCUIT_INVALID", validationError));
        return;
    }

    settingsStorage->saveEditorialCircuits(wallId, editorialCircuits);
    settingsStorage->saveCircuits(wallId, runtimeCircuits);
    runtimeState->setLastInputSource(RuntimeInputSource::App);
    runtimeState->setLastCommand(RuntimeLastCommand::SyncCircuits);
    runtimeState->clearLastError();

    String dataJson = "{";
    dataJson += "\"wallId\":\"" + wallId + "\",";
    dataJson += "\"circuitsAccepted\":" + String(static_cast<int>(editorialCircuits.size())) + ",";
    dataJson += "\"circuitsRejected\":0";
    dataJson += "}";
    server.send(200, "application/json", buildSuccessResponse("Editorial circuits synchronized", dataJson));
}

void HttpServer::handlePostCircuitShow()
{
    handlePostCircuitStart();
}

void HttpServer::handlePostCircuitVisualize()
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
    if (!circuitController->visualize(circuitId))
    {
        const auto* circuit = circuitRepository->findById(circuitId);
        if (circuit == nullptr)
        {
            runtimeState->setLastError("circuitId not found");
            server.send(404, "application/json", buildErrorResponse("CIRCUIT_NOT_FOUND", "Circuit not found"));
            return;
        }

        server.send(500, "application/json", buildErrorResponse("CIRCUIT_VISUALIZE_FAILED", "Unable to visualize circuit"));
        return;
    }

    const auto* circuit = circuitRepository->findById(circuitId);
    String dataJson = "{";
    dataJson += "\"circuitId\":\"" + circuitId + "\",";
    dataJson += "\"name\":\"" + circuit->name + "\"";
    dataJson += "}";

    server.send(200, "application/json", buildSuccessResponse("Circuit visualized", dataJson));
}

void HttpServer::handlePostCircuitStart()
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
    if (!circuitController->start(circuitId))
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

void HttpServer::handlePostRandomSequenceTest()
{
    if (!wallMapRepository->hasConfig())
    {
        server.send(409, "application/json", buildErrorResponse("CONFIG_NOT_LOADED", "Wall config not loaded"));
        return;
    }

    const auto& config = wallMapRepository->getConfig();
    std::vector<const LedPointDto*> enabledPoints;
    enabledPoints.reserve(config.points.size());

    for (const auto& point : config.points)
    {
        if (point.enabled)
        {
            enabledPoints.push_back(&point);
        }
    }

    if (enabledPoints.empty())
    {
        server.send(409, "application/json", buildErrorResponse("POINTS_NOT_AVAILABLE", "No enabled LED points available"));
        return;
    }

    std::sort(enabledPoints.begin(), enabledPoints.end(), [](const LedPointDto* left, const LedPointDto* right) {
        return left->ledIndex < right->ledIndex;
    });

    randomSeed(micros());
    static const char* palette[] = {
        "#FF3B30",
        "#FF9500",
        "#FFCC00",
        "#34C759",
        "#00C7BE",
        "#007AFF",
        "#5856D6",
        "#FF2D55",
        "#FFFFFF"
    };

    CircuitDefinitionDto previewCircuit;
    previewCircuit.circuitId = "__preview_random_sequence__";
    previewCircuit.name = "RandomSequencePreview";
    previewCircuit.wallId = config.wallId;
    previewCircuit.style.defaultColor = "#FFFFFF";
    previewCircuit.style.brightness = config.brightnessLimit > 0 ? config.brightnessLimit : 180;
    previewCircuit.style.effect = VisualEffect::Steady;

    for (size_t index = 0; index < enabledPoints.size(); index++)
    {
        const auto* point = enabledPoints[index];
        CircuitStepDto step;
        step.pointId = point->pointId;
        step.orderIndex = static_cast<int>(index);
        step.blinkCount = 1 + random(0, 2);
        step.blinkPeriodMs = 90 + random(0, 110);
        step.holdDurationMs = 110 + random(0, 170);
        step.highlightBrightness = 64 + random(0, std::max(1, previewCircuit.style.brightness - 63));
        step.dimmedBrightness = 0;
        step.highlightColor = palette[random(0, static_cast<long>(sizeof(palette) / sizeof(palette[0])))];
        step.dimmedColor = "#000000";
        step.autoAdvance = true;
        step.enabled = true;
        previewCircuit.steps.push_back(step);
    }

    runtimeState->setLastInputSource(RuntimeInputSource::App);
    runtimeState->setLastCommand(RuntimeLastCommand::LedTest);
    if (!circuitController->showPreview(previewCircuit))
    {
        server.send(500, "application/json", buildErrorResponse("LED_TEST_FAILED", "Unable to start random LED sequence"));
        return;
    }

    String dataJson = "{";
    dataJson += "\"circuitId\":\"" + previewCircuit.circuitId + "\",";
    dataJson += "\"steps\":" + String(static_cast<int>(previewCircuit.steps.size())) + ",";
    dataJson += "\"wallId\":\"" + config.wallId + "\"";
    dataJson += "}";
    server.send(200, "application/json", buildSuccessResponse("Random LED sequence started", dataJson));
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

bool HttpServer::parseEditorialCircuitsPayload(const String& body,
                                               String& wallId,
                                               std::vector<CircuitEditorialDefinitionDto>& circuits,
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
        if (movementsJson.isNull())
        {
            validationError = "movements array missing";
            return false;
        }

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

    validationError = "";
    return !wallId.isEmpty() && !circuits.empty();
}

bool HttpServer::tryConvertEditorialToRuntime(const std::vector<CircuitEditorialDefinitionDto>& editorialCircuits,
                                              std::vector<CircuitDefinitionDto>& runtimeCircuits,
                                              String& validationError) const
{
    runtimeCircuits.clear();
    if (wallMapRepository == nullptr)
    {
        validationError = "wall map unavailable";
        return false;
    }

    for (const auto& editorialCircuit : editorialCircuits)
    {
        if (editorialCircuit.circuitId.isEmpty() || editorialCircuit.name.isEmpty() || editorialCircuit.wallId.isEmpty())
        {
            validationError = "circuitId, name and wallId are required";
            return false;
        }

        if (editorialCircuit.movements.empty())
        {
            validationError = "movements must not be empty";
            return false;
        }

        CircuitDefinitionDto runtimeCircuit;
        runtimeCircuit.circuitId = editorialCircuit.circuitId;
        runtimeCircuit.name = editorialCircuit.name;
        runtimeCircuit.wallId = editorialCircuit.wallId;
        runtimeCircuit.style.defaultColor = editorialCircuit.globals.rightHandColor;
        runtimeCircuit.style.brightness = editorialCircuit.globals.defaultBrightness;
        runtimeCircuit.style.effect = editorialCircuit.globals.effect;

        for (const auto& movement : editorialCircuit.movements)
        {
            if (movement.pointRef <= 0 || movement.sequence <= 0 || (movement.hand != 0 && movement.hand != 1) || movement.role < 0 || movement.role > 2)
            {
                validationError = "invalid editorial movement";
                return false;
            }

            const auto* point = wallMapRepository->findPointByHoleNumber(movement.pointRef);
            if (point == nullptr || point->pointId.isEmpty())
            {
                validationError = "holeNumber not found in wall map";
                return false;
            }

            CircuitItemDto item;
            item.pointId = point->pointId;
            item.role = movement.role == 1
                ? CircuitRole::Start
                : movement.role == 2
                    ? CircuitRole::Top
                    : movement.hand == 1 ? CircuitRole::RightHand : CircuitRole::LeftHand;
            item.color = movement.role == 1
                ? editorialCircuit.globals.startColor
                : movement.role == 2
                    ? editorialCircuit.globals.topColor
                    : movement.hand == 1 ? editorialCircuit.globals.rightHandColor : editorialCircuit.globals.leftHandColor;
            item.effect = movement.role == 2 ? VisualEffect::Pulse : editorialCircuit.globals.effect;
            item.enabled = true;
            runtimeCircuit.items.push_back(item);

            CircuitStepDto step;
            step.pointId = point->pointId;
            step.orderIndex = movement.sequence - 1;
            step.blinkCount = movement.role == 1 ? editorialCircuit.globals.blinkCount : 1;
            step.blinkPeriodMs = editorialCircuit.globals.blinkPeriodMs;
            step.highlightBrightness = editorialCircuit.globals.defaultBrightness;
            step.holdDurationMs = movement.role == 2
                ? std::max(editorialCircuit.globals.holdDurationMs, 3500)
                : editorialCircuit.globals.holdDurationMs;
            step.dimmedBrightness = editorialCircuit.globals.dimmedBrightness;
            step.highlightColor = item.color;
            step.dimmedColor = movement.role == 1
                ? "#404000"
                : movement.role == 2
                    ? "#400000"
                    : movement.hand == 1 ? "#40201C" : "#1D3440";
            step.autoAdvance = true;
            step.enabled = true;
            runtimeCircuit.steps.push_back(step);
        }

        runtimeCircuits.push_back(runtimeCircuit);
    }

    validationError = "";
    return !runtimeCircuits.empty();
}

bool HttpServer::tryConvertRuntimeToEditorial(const std::vector<CircuitDefinitionDto>& runtimeCircuits,
                                              std::vector<CircuitEditorialDefinitionDto>& editorialCircuits,
                                              String& validationError) const
{
    editorialCircuits.clear();
    if (wallMapRepository == nullptr)
    {
        validationError = "wall map unavailable";
        return false;
    }

    for (const auto& runtimeCircuit : runtimeCircuits)
    {
        CircuitEditorialDefinitionDto editorialCircuit;
        editorialCircuit.circuitId = runtimeCircuit.circuitId;
        editorialCircuit.name = runtimeCircuit.name;
        editorialCircuit.wallId = runtimeCircuit.wallId;
        editorialCircuit.globals.effect = runtimeCircuit.style.effect == VisualEffect::Unknown
            ? VisualEffect::Steady
            : runtimeCircuit.style.effect;
        editorialCircuit.globals.defaultBrightness = runtimeCircuit.style.brightness > 0 ? runtimeCircuit.style.brightness : 96;
        editorialCircuit.globals.dimmedBrightness = 96;

        const auto orderedSteps = runtimeCircuit.steps;
        for (const auto& step : orderedSteps)
        {
            const auto* point = wallMapRepository->findPointById(step.pointId);
            if (point == nullptr || point->holeNumber <= 0)
            {
                validationError = "pointId not found in wall map";
                return false;
            }

            CircuitMovementEditorialDto movement;
            movement.pointRef = point->holeNumber;
            movement.sequence = step.orderIndex + 1;

            const auto itemIt = std::find_if(runtimeCircuit.items.begin(), runtimeCircuit.items.end(), [&](const CircuitItemDto& item) {
                return item.pointId == step.pointId;
            });

            const auto role = itemIt == runtimeCircuit.items.end() ? CircuitRole::Unknown : itemIt->role;
            movement.hand = role == CircuitRole::LeftHand ? 0 : 1;
            movement.role = role == CircuitRole::Start ? 1 : role == CircuitRole::Top ? 2 : 0;
            editorialCircuit.movements.push_back(movement);

            if (itemIt != runtimeCircuit.items.end())
            {
                if (role == CircuitRole::Start)
                {
                    editorialCircuit.globals.startColor = itemIt->color;
                }
                else if (role == CircuitRole::Top)
                {
                    editorialCircuit.globals.topColor = itemIt->color;
                }
                else if (role == CircuitRole::LeftHand)
                {
                    editorialCircuit.globals.leftHandColor = itemIt->color;
                }
                else
                {
                    editorialCircuit.globals.rightHandColor = itemIt->color;
                }
            }

            if (step.blinkCount > 1)
            {
                editorialCircuit.globals.blinkCount = step.blinkCount;
            }

            if (step.blinkPeriodMs > 0)
            {
                editorialCircuit.globals.blinkPeriodMs = step.blinkPeriodMs;
            }

            if (step.holdDurationMs > 0 && step.holdDurationMs != 3500)
            {
                editorialCircuit.globals.holdDurationMs = step.holdDurationMs;
            }

            if (step.dimmedBrightness >= 0)
            {
                editorialCircuit.globals.dimmedBrightness = step.dimmedBrightness;
            }
        }

        std::sort(editorialCircuit.movements.begin(), editorialCircuit.movements.end(), [](const CircuitMovementEditorialDto& left, const CircuitMovementEditorialDto& right) {
            return left.sequence < right.sequence;
        });

        editorialCircuits.push_back(editorialCircuit);
    }

    validationError = "";
    return !editorialCircuits.empty();
}

String HttpServer::buildEditorialCircuitsDataJson(const String& wallId, const std::vector<CircuitEditorialDefinitionDto>& circuits) const
{
    String dataJson = "{";
    dataJson += "\"wallId\":\"" + wallId + "\",";
    dataJson += "\"circuits\":[";

    for (size_t circuitIndex = 0; circuitIndex < circuits.size(); circuitIndex++)
    {
        if (circuitIndex > 0)
        {
            dataJson += ",";
        }

        const auto& circuit = circuits[circuitIndex];
        dataJson += "{";
        dataJson += "\"circuitId\":\"" + circuit.circuitId + "\",";
        dataJson += "\"name\":\"" + circuit.name + "\",";
        dataJson += "\"wallId\":\"" + circuit.wallId + "\",";
        dataJson += "\"difficulty\":\"" + circuit.difficulty + "\",";
        dataJson += "\"inclination\":\"" + circuit.inclination + "\",";
        dataJson += "\"globals\":{";
        dataJson += "\"presetName\":\"" + circuit.globals.presetName + "\",";
        dataJson += "\"effect\":\"" + String(visualEffectToString(circuit.globals.effect)) + "\",";
        dataJson += "\"defaultBrightness\":" + String(circuit.globals.defaultBrightness) + ",";
        dataJson += "\"dimmedBrightness\":" + String(circuit.globals.dimmedBrightness) + ",";
        dataJson += "\"rightHandColor\":\"" + circuit.globals.rightHandColor + "\",";
        dataJson += "\"leftHandColor\":\"" + circuit.globals.leftHandColor + "\",";
        dataJson += "\"startColor\":\"" + circuit.globals.startColor + "\",";
        dataJson += "\"topColor\":\"" + circuit.globals.topColor + "\",";
        dataJson += "\"blinkCount\":" + String(circuit.globals.blinkCount) + ",";
        dataJson += "\"blinkPeriodMs\":" + String(circuit.globals.blinkPeriodMs) + ",";
        dataJson += "\"holdDurationMs\":" + String(circuit.globals.holdDurationMs);
        dataJson += "},";
        dataJson += "\"movements\":[";

        for (size_t movementIndex = 0; movementIndex < circuit.movements.size(); movementIndex++)
        {
            if (movementIndex > 0)
            {
                dataJson += ",";
            }

            const auto& movement = circuit.movements[movementIndex];
            dataJson += "{";
            dataJson += "\"p\":" + String(movement.pointRef) + ",";
            dataJson += "\"h\":" + String(movement.hand) + ",";
            dataJson += "\"r\":" + String(movement.role) + ",";
            dataJson += "\"s\":" + String(movement.sequence);
            dataJson += "}";
        }

        dataJson += "]}";
    }

    dataJson += "]}";
    return dataJson;
}
