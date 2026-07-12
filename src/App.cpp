#include "App.h"

#include "AppConstants.h"

void App::setup()
{
    Serial.println();
    Serial.println("Booting KKKKK ESP32 firmware scaffold...");

    bootstrapRuntimeState();

    settingsStorage.begin();
    restorePersistedConfiguration();

    WifiSettings wifiSettings;
    if (settingsStorage.hasWifiCredentials())
    {
        runtimeState.setState(RuntimeAppState::ConnectingWifi);
        wifiSettings = settingsStorage.loadWifiSettings();
    }
    else
    {
        runtimeState.setState(RuntimeAppState::Provisioning);
    }
    networkManager.begin(wifiSettings);

    ledRenderer.begin();
    diagnostics.begin();
    circuitController.begin(&runtimeState, &circuitRepository, &wallMapRepository, &ledRenderer);
    buttonController.begin(&runtimeState, &circuitRepository, &circuitController);
    httpServer.begin(&runtimeState,
                     &settingsStorage,
                     &networkManager,
                     &wallMapRepository,
                     &circuitRepository,
                     &circuitController);

    if (runtimeState.getState() == RuntimeAppState::ConnectingWifi && networkManager.isConnected())
    {
        runtimeState.setState(RuntimeAppState::Ready);
    }

    refreshDerivedState();
}

void App::loop()
{
    networkManager.loop();
    httpServer.loop();
    buttonController.loop();
    circuitController.loop();
    refreshDerivedState();
    diagnostics.loop();
    logPeriodicStatus();

    if (runtimeState.getState() == RuntimeAppState::ConnectingWifi && networkManager.isConnected())
    {
        runtimeState.setState(RuntimeAppState::Ready);
    }
}

void App::refreshDerivedState()
{
    runtimeState.setWifiStatus(networkManager.isConnected() ? RuntimeWifiStatus::Connected : RuntimeWifiStatus::Disconnected);
    runtimeState.setIpAddress(networkManager.getIpAddress());
    runtimeState.setConfiguredWallId(wallMapRepository.getWallId());
    runtimeState.setConfigLoaded(wallMapRepository.hasConfig());
    runtimeState.setCircuitsLoaded(circuitRepository.hasCircuits());
    runtimeState.setCircuitsCount(circuitRepository.getCircuitsCount());
    runtimeState.setLedStatus(ledRenderer.getStatusLabel());
    runtimeState.setActiveCircuitId(circuitController.getActiveCircuitId());

    if (!circuitController.getActiveCircuitId().isEmpty())
    {
        buttonController.syncSelectedCircuitId(circuitController.getActiveCircuitId());
    }

    runtimeState.setSelectedCircuitId(buttonController.getSelectedCircuitId());
    runtimeState.setSequenceActive(circuitController.isSequenceActive());
    runtimeState.setCurrentStepIndex(circuitController.getCurrentStepIndex());
    runtimeState.setCurrentPhaseRemainingMs(circuitController.getCurrentPhaseRemainingMs());
    const String phaseLabel = circuitController.getSequencePhaseLabel();
    if (phaseLabel == "Blinking")
    {
        runtimeState.setSequencePhase(RuntimeSequencePhase::Blinking);
    }
    else if (phaseLabel == "Holding")
    {
        runtimeState.setSequencePhase(RuntimeSequencePhase::Holding);
    }
    else
    {
        runtimeState.setSequencePhase(RuntimeSequencePhase::None);
    }
}

void App::restorePersistedConfiguration()
{
    WallConfigDto wallConfig;
    String validationError;
    if (settingsStorage.loadWallConfig(wallConfig))
    {
        if (!wallMapRepository.setConfig(wallConfig, validationError))
        {
            runtimeState.setLastError(validationError);
        }
    }

    String wallId;
    std::vector<CircuitDefinitionDto> circuits;
    if (settingsStorage.loadCircuits(wallId, circuits))
    {
        if (!circuitRepository.setCircuits(wallId, circuits, validationError))
        {
            runtimeState.setLastError(validationError);
        }
    }
}

void App::bootstrapRuntimeState()
{
    runtimeState.setState(RuntimeAppState::Booting);
    runtimeState.setLastCommand(RuntimeLastCommand::None);
    runtimeState.clearLastError();
    runtimeState.setDeviceName(AppConstants::DeviceName);
    runtimeState.setFirmwareVersion(AppConstants::FirmwareVersion);
    runtimeState.setApiVersion(AppConstants::ApiVersion);
}

void App::logPeriodicStatus()
{
    const auto now = millis();
    if (now - lastStatusLogAt < AppConstants::StatusLogIntervalMs)
    {
        return;
    }

    lastStatusLogAt = now;
    Serial.print("Runtime state: ");
    Serial.println(runtimeState.toString());
}
