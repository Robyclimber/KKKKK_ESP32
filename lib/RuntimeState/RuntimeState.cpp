#include "RuntimeState.h"

void RuntimeState::setState(RuntimeAppState nextState)
{
    state = nextState;
}

RuntimeAppState RuntimeState::getState() const
{
    return state;
}

void RuntimeState::setLastCommand(RuntimeLastCommand nextCommand)
{
    lastCommand = nextCommand;
}

RuntimeLastCommand RuntimeState::getLastCommand() const
{
    return lastCommand;
}

void RuntimeState::setLastError(const String& error)
{
    lastError = error;
}

const String& RuntimeState::getLastError() const
{
    return lastError;
}

void RuntimeState::setDeviceName(const String& value)
{
    deviceName = value;
}

const String& RuntimeState::getDeviceName() const
{
    return deviceName;
}

void RuntimeState::setFirmwareVersion(const String& value)
{
    firmwareVersion = value;
}

const String& RuntimeState::getFirmwareVersion() const
{
    return firmwareVersion;
}

void RuntimeState::setApiVersion(const String& value)
{
    apiVersion = value;
}

const String& RuntimeState::getApiVersion() const
{
    return apiVersion;
}

void RuntimeState::setWifiStatus(RuntimeWifiStatus value)
{
    wifiStatus = value;
}

RuntimeWifiStatus RuntimeState::getWifiStatus() const
{
    return wifiStatus;
}

void RuntimeState::setIpAddress(const String& value)
{
    ipAddress = value;
}

const String& RuntimeState::getIpAddress() const
{
    return ipAddress;
}

void RuntimeState::setConfiguredWallId(const String& value)
{
    configuredWallId = value;
}

const String& RuntimeState::getConfiguredWallId() const
{
    return configuredWallId;
}

void RuntimeState::setConfigLoaded(bool value)
{
    configLoaded = value;
}

bool RuntimeState::isConfigLoaded() const
{
    return configLoaded;
}

void RuntimeState::setCircuitsLoaded(bool value)
{
    circuitsLoaded = value;
}

bool RuntimeState::isCircuitsLoaded() const
{
    return circuitsLoaded;
}

void RuntimeState::setCircuitsCount(int value)
{
    circuitsCount = value;
}

int RuntimeState::getCircuitsCount() const
{
    return circuitsCount;
}

void RuntimeState::setActiveCircuitId(const String& value)
{
    activeCircuitId = value;
}

const String& RuntimeState::getActiveCircuitId() const
{
    return activeCircuitId;
}

void RuntimeState::setSelectedCircuitId(const String& value)
{
    selectedCircuitId = value;
}

const String& RuntimeState::getSelectedCircuitId() const
{
    return selectedCircuitId;
}

void RuntimeState::setLastInputSource(RuntimeInputSource value)
{
    lastInputSource = value;
}

RuntimeInputSource RuntimeState::getLastInputSource() const
{
    return lastInputSource;
}

void RuntimeState::setLedStatus(const String& value)
{
    ledStatus = value;
}

const String& RuntimeState::getLedStatus() const
{
    return ledStatus;
}

void RuntimeState::setSequenceActive(bool value)
{
    sequenceActive = value;
}

bool RuntimeState::isSequenceActive() const
{
    return sequenceActive;
}

void RuntimeState::setCurrentStepIndex(int value)
{
    currentStepIndex = value;
}

int RuntimeState::getCurrentStepIndex() const
{
    return currentStepIndex;
}

void RuntimeState::setSequencePhase(RuntimeSequencePhase value)
{
    sequencePhase = value;
}

RuntimeSequencePhase RuntimeState::getSequencePhase() const
{
    return sequencePhase;
}

void RuntimeState::setCurrentPhaseRemainingMs(unsigned long value)
{
    currentPhaseRemainingMs = value;
}

unsigned long RuntimeState::getCurrentPhaseRemainingMs() const
{
    return currentPhaseRemainingMs;
}

void RuntimeState::clearLastError()
{
    lastError = "";
}

const char* RuntimeState::toString() const
{
    switch (state)
    {
        case RuntimeAppState::Booting:
            return "Booting";
        case RuntimeAppState::Provisioning:
            return "Provisioning";
        case RuntimeAppState::ConnectingWifi:
            return "ConnectingWifi";
        case RuntimeAppState::Ready:
            return "Ready";
        case RuntimeAppState::Idle:
            return "Idle";
        case RuntimeAppState::CircuitActive:
            return "CircuitActive";
        case RuntimeAppState::Error:
            return "Error";
        default:
            return "Unknown";
    }
}

const char* RuntimeState::wifiStatusToString() const
{
    switch (wifiStatus)
    {
        case RuntimeWifiStatus::Connected:
            return "Connected";
        case RuntimeWifiStatus::Disconnected:
        default:
            return "Disconnected";
    }
}

const char* RuntimeState::lastCommandToString() const
{
    switch (lastCommand)
    {
        case RuntimeLastCommand::WifiConfig:
            return "WifiConfig";
        case RuntimeLastCommand::SaveConfig:
            return "SaveConfig";
        case RuntimeLastCommand::SyncCircuits:
            return "SyncCircuits";
        case RuntimeLastCommand::VisualizeCircuit:
            return "VisualizeCircuit";
        case RuntimeLastCommand::StartCircuit:
            return "StartCircuit";
        case RuntimeLastCommand::ShowCircuit:
            return "ShowCircuit";
        case RuntimeLastCommand::StopCircuit:
            return "StopCircuit";
        case RuntimeLastCommand::ResetCircuit:
            return "ResetCircuit";
        case RuntimeLastCommand::ClearCircuit:
            return "ClearCircuit";
        case RuntimeLastCommand::LedTest:
            return "LedTest";
        case RuntimeLastCommand::Restart:
            return "Restart";
        case RuntimeLastCommand::None:
        default:
            return "None";
    }
}

const char* RuntimeState::configStatusToString() const
{
    return configLoaded ? "Loaded" : "Missing";
}

const char* RuntimeState::circuitsStatusToString() const
{
    return circuitsLoaded ? "Loaded" : "Missing";
}

const char* RuntimeState::sequencePhaseToString() const
{
    switch (sequencePhase)
    {
        case RuntimeSequencePhase::Blinking:
            return "Blinking";
        case RuntimeSequencePhase::Holding:
            return "Holding";
        case RuntimeSequencePhase::None:
        default:
            return "None";
    }
}

const char* RuntimeState::lastInputSourceToString() const
{
    switch (lastInputSource)
    {
        case RuntimeInputSource::App:
            return "App";
        case RuntimeInputSource::Button:
            return "Button";
        case RuntimeInputSource::None:
        default:
            return "None";
    }
}

unsigned long RuntimeState::getUptimeSeconds() const
{
    return millis() / 1000UL;
}
