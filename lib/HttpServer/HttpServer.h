#pragma once

#include <Arduino.h>
#include <WebServer.h>

#include "CircuitController.h"
#include "CircuitRepository.h"
#include "NetworkManager.h"
#include "RuntimeState.h"
#include "SettingsStorage.h"
#include "WallMapRepository.h"

class HttpServer
{
public:
    void begin(RuntimeState* runtimeState,
               SettingsStorage* settingsStorage,
               NetworkManager* networkManager,
               WallMapRepository* wallMapRepository,
               CircuitRepository* circuitRepository,
               CircuitController* circuitController);
    void loop();

private:
    void configureRoutes();
    void handleProvisioningPage();
    void handleHealth();
    void handleStatus();
    void handleGetConfig();
    void handleGetCircuits();
    void handlePostWifiConfig();
    void handlePostConfig();
    void handlePostCircuits();
    void handlePostCircuitShow();
    void handlePostCircuitStop();
    void handlePostCircuitReset();
    void handlePostCircuitClear();
    void handlePostRandomSequenceTest();
    String buildSuccessResponse(const String& message, const String& dataJson) const;
    String buildErrorResponse(const String& errorCode, const String& message) const;
    bool parseWallConfig(const String& body, WallConfigDto& config, String& validationError) const;
    bool parseCircuitsPayload(const String& body, String& wallId, std::vector<CircuitDefinitionDto>& circuits, String& validationError) const;

    RuntimeState* runtimeState = nullptr;
    SettingsStorage* settingsStorage = nullptr;
    NetworkManager* networkManager = nullptr;
    WallMapRepository* wallMapRepository = nullptr;
    CircuitRepository* circuitRepository = nullptr;
    CircuitController* circuitController = nullptr;
    WebServer server{80};
};
