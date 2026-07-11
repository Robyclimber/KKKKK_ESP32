#pragma once

#include <Arduino.h>

enum class RuntimeAppState
{
    Booting,
    Provisioning,
    ConnectingWifi,
    Ready,
    Idle,
    CircuitActive,
    Error
};

enum class RuntimeLastCommand
{
    None,
    WifiConfig,
    SaveConfig,
    SyncCircuits,
    ShowCircuit,
    StopCircuit,
    ResetCircuit,
    ClearCircuit,
    LedTest,
    Restart
};

enum class RuntimeWifiStatus
{
    Disconnected,
    Connected
};

enum class RuntimeSequencePhase
{
    None,
    Blinking,
    Holding
};

enum class RuntimeInputSource
{
    None,
    App,
    Button
};

class RuntimeState
{
public:
    void setState(RuntimeAppState nextState);
    RuntimeAppState getState() const;

    void setLastCommand(RuntimeLastCommand nextCommand);
    RuntimeLastCommand getLastCommand() const;

    void setLastError(const String& error);
    const String& getLastError() const;
    void clearLastError();

    void setDeviceName(const String& value);
    const String& getDeviceName() const;

    void setFirmwareVersion(const String& value);
    const String& getFirmwareVersion() const;

    void setApiVersion(const String& value);
    const String& getApiVersion() const;

    void setWifiStatus(RuntimeWifiStatus value);
    RuntimeWifiStatus getWifiStatus() const;

    void setIpAddress(const String& value);
    const String& getIpAddress() const;

    void setConfiguredWallId(const String& value);
    const String& getConfiguredWallId() const;

    void setConfigLoaded(bool value);
    bool isConfigLoaded() const;

    void setCircuitsLoaded(bool value);
    bool isCircuitsLoaded() const;

    void setCircuitsCount(int value);
    int getCircuitsCount() const;

    void setActiveCircuitId(const String& value);
    const String& getActiveCircuitId() const;

    void setSelectedCircuitId(const String& value);
    const String& getSelectedCircuitId() const;

    void setLastInputSource(RuntimeInputSource value);
    RuntimeInputSource getLastInputSource() const;

    void setLedStatus(const String& value);
    const String& getLedStatus() const;

    void setSequenceActive(bool value);
    bool isSequenceActive() const;

    void setCurrentStepIndex(int value);
    int getCurrentStepIndex() const;

    void setSequencePhase(RuntimeSequencePhase value);
    RuntimeSequencePhase getSequencePhase() const;

    void setCurrentPhaseRemainingMs(unsigned long value);
    unsigned long getCurrentPhaseRemainingMs() const;

    const char* toString() const;
    const char* lastCommandToString() const;
    const char* wifiStatusToString() const;
    const char* configStatusToString() const;
    const char* circuitsStatusToString() const;
    const char* sequencePhaseToString() const;
    const char* lastInputSourceToString() const;
    unsigned long getUptimeSeconds() const;

private:
    RuntimeAppState state = RuntimeAppState::Booting;
    RuntimeLastCommand lastCommand = RuntimeLastCommand::None;
    String lastError;
    String deviceName;
    String firmwareVersion;
    String apiVersion;
    RuntimeWifiStatus wifiStatus = RuntimeWifiStatus::Disconnected;
    String ipAddress = "0.0.0.0";
    String configuredWallId;
    bool configLoaded = false;
    bool circuitsLoaded = false;
    int circuitsCount = 0;
    String activeCircuitId;
    String selectedCircuitId;
    RuntimeInputSource lastInputSource = RuntimeInputSource::None;
    String ledStatus = "Unknown";
    bool sequenceActive = false;
    int currentStepIndex = -1;
    RuntimeSequencePhase sequencePhase = RuntimeSequencePhase::None;
    unsigned long currentPhaseRemainingMs = 0UL;
};
