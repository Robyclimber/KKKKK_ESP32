#pragma once

#include <Arduino.h>

#include "CircuitController.h"
#include "CircuitRepository.h"
#include "RuntimeState.h"

class ButtonController
{
public:
    void begin(RuntimeState* runtimeState, CircuitRepository* circuitRepository, CircuitController* circuitController);
    void loop();
    const String& getSelectedCircuitId() const;
    void syncSelectedCircuitId(const String& circuitId);

private:
    struct ButtonState
    {
        int pin = -1;
        bool stablePressed = false;
        bool lastRawPressed = false;
        unsigned long lastRawChangeAtMs = 0UL;
        unsigned long stableChangedAtMs = 0UL;
        bool longPressHandled = false;
    };

    void initializeSelection();
    void processButton(ButtonState& buttonState,
                       void (ButtonController::*onShortPress)(),
                       void (ButtonController::*onLongPress)() = nullptr);
    void handleNextCircuitPressed();
    void handleStartStopPressed();
    void handleResetPressed();
    void handleResetLongPressed();
    bool canHandleButtons() const;

    RuntimeState* runtimeState = nullptr;
    CircuitRepository* circuitRepository = nullptr;
    CircuitController* circuitController = nullptr;
    String selectedCircuitId;
    unsigned long lastPollAtMs = 0UL;
    ButtonState nextCircuitButton;
    ButtonState startStopButton;
    ButtonState resetButton;
};
