#pragma once

#include <Arduino.h>

#include "ButtonController.h"
#include "CircuitController.h"
#include "CircuitRepository.h"
#include "Diagnostics.h"
#include "HttpServer.h"
#include "LedRenderer.h"
#include "NetworkManager.h"
#include "RuntimeState.h"
#include "SettingsStorage.h"
#include "WallMapRepository.h"

class App
{
public:
    void setup();
    void loop();

private:
    void bootstrapRuntimeState();
    void logPeriodicStatus();
    void refreshDerivedState();
    void restorePersistedConfiguration();

    RuntimeState runtimeState;
    SettingsStorage settingsStorage;
    NetworkManager networkManager;
    WallMapRepository wallMapRepository;
    CircuitRepository circuitRepository;
    LedRenderer ledRenderer;
    Diagnostics diagnostics;
    ButtonController buttonController;
    CircuitController circuitController;
    HttpServer httpServer;

    unsigned long lastStatusLogAt = 0UL;
};
