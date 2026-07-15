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
    void handleGetEditorialCircuits();
    void handlePostWifiConfig();
    void handlePostConfig();
    void handlePostCircuits();
    void handlePostEditorialCircuits();
    void handlePostCircuitVisualize();
    void handlePostCircuitStart();
    void handlePostCircuitShow();
    void handlePostCircuitStop();
    void handlePostCircuitReset();
    void handlePostCircuitClear();
    void handlePostRandomSequenceTest();
    String buildSuccessResponse(const String& message, const String& dataJson) const;
    String buildErrorResponse(const String& errorCode, const String& message) const;
    bool parseWallConfig(const String& body, WallConfigDto& config, String& validationError) const;
    bool parseCircuitsPayload(const String& body, String& wallId, std::vector<CircuitDefinitionDto>& circuits, String& validationError) const;
    bool parseEditorialCircuitsPayload(const String& body, String& wallId, std::vector<CircuitEditorialDefinitionDto>& circuits, String& validationError) const;
    bool tryConvertEditorialToRuntime(const std::vector<CircuitEditorialDefinitionDto>& editorialCircuits,
                                      std::vector<CircuitDefinitionDto>& runtimeCircuits,
                                      String& validationError) const;
    bool tryConvertRuntimeToEditorial(const std::vector<CircuitDefinitionDto>& runtimeCircuits,
                                      std::vector<CircuitEditorialDefinitionDto>& editorialCircuits,
                                      String& validationError) const;
    String buildEditorialCircuitsDataJson(const String& wallId, const std::vector<CircuitEditorialDefinitionDto>& circuits) const;

    RuntimeState* runtimeState = nullptr;
    SettingsStorage* settingsStorage = nullptr;
    NetworkManager* networkManager = nullptr;
    WallMapRepository* wallMapRepository = nullptr;
    CircuitRepository* circuitRepository = nullptr;
    CircuitController* circuitController = nullptr;
    WebServer server{80};
};
