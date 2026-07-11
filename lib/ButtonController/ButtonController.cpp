#include "ButtonController.h"

#include "AppConstants.h"

void ButtonController::begin(RuntimeState* runtimeStateRef,
                             CircuitRepository* circuitRepositoryRef,
                             CircuitController* circuitControllerRef)
{
    runtimeState = runtimeStateRef;
    circuitRepository = circuitRepositoryRef;
    circuitController = circuitControllerRef;

    nextCircuitButton.pin = AppConstants::ButtonNextCircuitPin;
    startStopButton.pin = AppConstants::ButtonStartStopPin;
    resetButton.pin = AppConstants::ButtonResetPin;

    pinMode(nextCircuitButton.pin, INPUT_PULLUP);
    pinMode(startStopButton.pin, INPUT_PULLUP);
    pinMode(resetButton.pin, INPUT_PULLUP);

    initializeSelection();
}

void ButtonController::loop()
{
    if (!canHandleButtons())
    {
        return;
    }

    const auto now = millis();
    if (now - lastPollAtMs < AppConstants::ButtonPollIntervalMs)
    {
        return;
    }

    lastPollAtMs = now;
    initializeSelection();
    processButton(nextCircuitButton, &ButtonController::handleNextCircuitPressed);
    processButton(startStopButton, &ButtonController::handleStartStopPressed);
    processButton(resetButton, &ButtonController::handleResetPressed, &ButtonController::handleResetLongPressed);
}

const String& ButtonController::getSelectedCircuitId() const
{
    return selectedCircuitId;
}

void ButtonController::syncSelectedCircuitId(const String& circuitId)
{
    if (runtimeState == nullptr || circuitRepository == nullptr)
    {
        return;
    }

    if (circuitId.isEmpty())
    {
        runtimeState->setSelectedCircuitId(selectedCircuitId);
        return;
    }

    if (circuitRepository->findById(circuitId) == nullptr)
    {
        return;
    }

    selectedCircuitId = circuitId;
    runtimeState->setSelectedCircuitId(selectedCircuitId);
}

void ButtonController::initializeSelection()
{
    if (circuitRepository == nullptr || runtimeState == nullptr)
    {
        return;
    }

    if (!selectedCircuitId.isEmpty() && circuitRepository->findById(selectedCircuitId) != nullptr)
    {
        runtimeState->setSelectedCircuitId(selectedCircuitId);
        return;
    }

    const auto& circuits = circuitRepository->getCircuits();
    if (!circuits.empty())
    {
        selectedCircuitId = circuits.front().circuitId;
    }
    else
    {
        selectedCircuitId = "";
    }

    runtimeState->setSelectedCircuitId(selectedCircuitId);
}

void ButtonController::processButton(ButtonState& buttonState,
                                     void (ButtonController::*onShortPress)(),
                                     void (ButtonController::*onLongPress)())
{
    const bool rawPressed = digitalRead(buttonState.pin) == LOW;
    const auto now = millis();

    if (rawPressed != buttonState.lastRawPressed)
    {
        buttonState.lastRawPressed = rawPressed;
        buttonState.lastRawChangeAtMs = now;
    }

    if (rawPressed != buttonState.stablePressed &&
        now - buttonState.lastRawChangeAtMs >= AppConstants::ButtonDebounceMs)
    {
        buttonState.stablePressed = rawPressed;
        buttonState.stableChangedAtMs = now;

        if (buttonState.stablePressed)
        {
            buttonState.longPressHandled = false;
        }
        else if (!buttonState.longPressHandled && onShortPress != nullptr)
        {
            (this->*onShortPress)();
        }
    }

    if (buttonState.stablePressed &&
        !buttonState.longPressHandled &&
        onLongPress != nullptr &&
        now - buttonState.stableChangedAtMs >= AppConstants::ButtonLongPressMs)
    {
        buttonState.longPressHandled = true;
        (this->*onLongPress)();
    }
}

void ButtonController::handleNextCircuitPressed()
{
    if (circuitRepository == nullptr || runtimeState == nullptr)
    {
        return;
    }

    if (!runtimeState->getActiveCircuitId().isEmpty())
    {
        return;
    }

    const auto& circuits = circuitRepository->getCircuits();
    if (circuits.empty())
    {
        return;
    }

    int currentIndex = -1;
    for (size_t index = 0; index < circuits.size(); index++)
    {
        if (circuits[index].circuitId == selectedCircuitId)
        {
            currentIndex = static_cast<int>(index);
            break;
        }
    }

    const int nextIndex = circuits.empty() ? 0 : (currentIndex + 1) % static_cast<int>(circuits.size());
    selectedCircuitId = circuits[nextIndex].circuitId;
    runtimeState->setSelectedCircuitId(selectedCircuitId);
    runtimeState->setLastInputSource(RuntimeInputSource::Button);
}

void ButtonController::handleStartStopPressed()
{
    if (circuitController == nullptr || runtimeState == nullptr)
    {
        return;
    }

    if (!runtimeState->getActiveCircuitId().isEmpty())
    {
        runtimeState->setLastInputSource(RuntimeInputSource::Button);
        circuitController->stop();
        return;
    }

    if (selectedCircuitId.isEmpty())
    {
        return;
    }

    runtimeState->setLastInputSource(RuntimeInputSource::Button);
    circuitController->show(selectedCircuitId);
}

void ButtonController::handleResetPressed()
{
    if (circuitController == nullptr)
    {
        return;
    }

    if (runtimeState != nullptr)
    {
        runtimeState->setLastInputSource(RuntimeInputSource::Button);
    }

    circuitController->reset();
}

void ButtonController::handleResetLongPressed()
{
    if (circuitController == nullptr)
    {
        return;
    }

    if (runtimeState != nullptr)
    {
        runtimeState->setLastInputSource(RuntimeInputSource::Button);
    }

    circuitController->clear();
}

bool ButtonController::canHandleButtons() const
{
    if (runtimeState == nullptr || circuitRepository == nullptr || circuitController == nullptr)
    {
        return false;
    }

    const auto state = runtimeState->getState();
    if (state == RuntimeAppState::Booting ||
        state == RuntimeAppState::Provisioning ||
        state == RuntimeAppState::ConnectingWifi)
    {
        return false;
    }

    return circuitRepository->hasCircuits();
}
