#include "CircuitController.h"

#include <algorithm>

void CircuitController::begin(RuntimeState* runtimeStateRef,
                              CircuitRepository* circuitRepositoryRef,
                              WallMapRepository* wallMapRepositoryRef,
                              LedRenderer* ledRendererRef)
{
    runtimeState = runtimeStateRef;
    circuitRepository = circuitRepositoryRef;
    wallMapRepository = wallMapRepositoryRef;
    ledRenderer = ledRendererRef;
}

void CircuitController::loop()
{
    if (!sequenceActive || activeCircuitId.isEmpty())
    {
        return;
    }

    updateStepSequence();
}

bool CircuitController::show(const String& circuitId)
{
    if (runtimeState == nullptr || ledRenderer == nullptr || circuitRepository == nullptr || wallMapRepository == nullptr)
    {
        return false;
    }

    const auto* circuit = circuitRepository->findById(circuitId);
    if (circuit == nullptr)
    {
        runtimeState->setLastError("circuitId not found");
        return false;
    }

    if (!circuit->steps.empty())
    {
        if (!startStepSequence(*circuit))
        {
            return false;
        }
    }
    else
    {
        resetSequenceState();

        std::vector<ResolvedLedCommand> ledCommands;
        if (!resolveCircuitLedCommands(*circuit, ledCommands))
        {
            return false;
        }

        if (!ledRenderer->showCircuit(*circuit, ledCommands))
        {
            runtimeState->setLastError("led render failed");
            return false;
        }
    }

    activeCircuitId = circuitId;
    runtimeState->setActiveCircuitId(activeCircuitId);
    runtimeState->setLastCommand(RuntimeLastCommand::ShowCircuit);
    runtimeState->setState(RuntimeAppState::CircuitActive);
    runtimeState->clearLastError();
    return true;
}

bool CircuitController::stop()
{
    if (runtimeState == nullptr || ledRenderer == nullptr)
    {
        return false;
    }

    resetSequenceState();
    ledRenderer->clear();
    activeCircuitId = "";
    runtimeState->setActiveCircuitId(activeCircuitId);
    runtimeState->setLastCommand(RuntimeLastCommand::StopCircuit);
    runtimeState->setState(RuntimeAppState::Idle);
    runtimeState->clearLastError();
    return true;
}

bool CircuitController::reset()
{
    if (runtimeState == nullptr || circuitRepository == nullptr)
    {
        return false;
    }

    runtimeState->setLastCommand(RuntimeLastCommand::ResetCircuit);
    if (activeCircuitId.isEmpty())
    {
        runtimeState->setLastError("no active circuit");
        return false;
    }

    const auto* circuit = circuitRepository->findById(activeCircuitId);
    if (circuit == nullptr)
    {
        runtimeState->setLastError("active circuit not found");
        return false;
    }

    if (!circuit->steps.empty())
    {
        if (!startStepSequence(*circuit))
        {
            return false;
        }
    }
    else
    {
        std::vector<ResolvedLedCommand> ledCommands;
        if (!resolveCircuitLedCommands(*circuit, ledCommands))
        {
            return false;
        }

        if (ledRenderer == nullptr || !ledRenderer->showCircuit(*circuit, ledCommands))
        {
            runtimeState->setLastError("led render failed");
            return false;
        }
    }

    runtimeState->clearLastError();
    return true;
}

bool CircuitController::clear()
{
    if (runtimeState == nullptr || ledRenderer == nullptr)
    {
        return false;
    }

    resetSequenceState();
    ledRenderer->clear();
    activeCircuitId = "";
    runtimeState->setActiveCircuitId(activeCircuitId);
    runtimeState->setLastCommand(RuntimeLastCommand::ClearCircuit);
    runtimeState->setState(RuntimeAppState::Idle);
    runtimeState->clearLastError();
    return true;
}

const String& CircuitController::getActiveCircuitId() const
{
    return activeCircuitId;
}

bool CircuitController::isSequenceActive() const
{
    return sequenceActive;
}

int CircuitController::getCurrentStepIndex() const
{
    return currentStepIndex;
}

const char* CircuitController::getSequencePhaseLabel() const
{
    switch (currentPhase)
    {
        case SequencePhase::Blinking:
            return "Blinking";
        case SequencePhase::Holding:
            return "Holding";
        case SequencePhase::None:
        default:
            return "None";
    }
}

unsigned long CircuitController::getCurrentPhaseRemainingMs() const
{
    if (!sequenceActive || circuitRepository == nullptr || activeCircuitId.isEmpty())
    {
        return 0UL;
    }

    const auto* circuit = circuitRepository->findById(activeCircuitId);
    if (circuit == nullptr)
    {
        return 0UL;
    }

    const auto orderedSteps = getOrderedEnabledSteps(*circuit);
    if (orderedSteps.empty() || currentStepIndex < 0 || currentStepIndex >= static_cast<int>(orderedSteps.size()))
    {
        return 0UL;
    }

    const auto* currentStep = orderedSteps[currentStepIndex];
    const auto now = millis();

    if (currentPhase == SequencePhase::Blinking)
    {
        const int blinkPeriodMs = currentStep->blinkPeriodMs > 0 ? currentStep->blinkPeriodMs : 250;
        const int requiredToggles = currentStep->blinkCount > 0 ? currentStep->blinkCount * 2 : 0;
        if (requiredToggles <= 0)
        {
            return 0UL;
        }

        const int remainingToggles = requiredToggles - blinkToggleCount;
        const unsigned long elapsedInCurrentToggle = now - lastBlinkToggleAtMs;
        const unsigned long currentToggleRemaining =
            elapsedInCurrentToggle >= static_cast<unsigned long>(blinkPeriodMs)
                ? 0UL
                : static_cast<unsigned long>(blinkPeriodMs) - elapsedInCurrentToggle;

        return static_cast<unsigned long>((remainingToggles > 0 ? remainingToggles - 1 : 0) * blinkPeriodMs) + currentToggleRemaining;
    }

    if (currentPhase == SequencePhase::Holding)
    {
        const unsigned long holdDurationMs = currentStep->holdDurationMs > 0 ? static_cast<unsigned long>(currentStep->holdDurationMs) : 0UL;
        const unsigned long elapsed = now - phaseStartedAtMs;
        return elapsed >= holdDurationMs ? 0UL : holdDurationMs - elapsed;
    }

    return 0UL;
}

bool CircuitController::resolveCircuitLedCommands(const CircuitDefinitionDto& circuit, std::vector<ResolvedLedCommand>& ledCommands)
{
    ledCommands.clear();

    if (wallMapRepository == nullptr)
    {
        if (runtimeState != nullptr)
        {
            runtimeState->setLastError("wall map unavailable");
        }

        return false;
    }

    if (!circuit.steps.empty())
    {
        for (const auto& step : circuit.steps)
        {
            if (!step.enabled)
            {
                continue;
            }

            const auto* point = wallMapRepository->findPointById(step.pointId);
            if (point == nullptr)
            {
                runtimeState->setLastError("pointId not found in wall map");
                return false;
            }

            if (!point->enabled)
            {
                continue;
            }

            ResolvedLedCommand ledCommand;
            ledCommand.ledIndex = point->ledIndex;
            ledCommand.color = step.highlightColor.isEmpty() ? circuit.style.defaultColor : step.highlightColor;
            ledCommand.effect = VisualEffect::Blink;
            ledCommand.brightness = resolveBrightnessValue(step.highlightBrightness, circuit.style.brightness);
            ledCommands.push_back(ledCommand);
        }
    }
    else
    {
        for (const auto& item : circuit.items)
        {
            if (!item.enabled)
            {
                continue;
            }

            const auto* point = wallMapRepository->findPointById(item.pointId);
            if (point == nullptr)
            {
                runtimeState->setLastError("pointId not found in wall map");
                return false;
            }

            if (!point->enabled)
            {
                continue;
            }

            ResolvedLedCommand ledCommand;
            ledCommand.ledIndex = point->ledIndex;
            ledCommand.color = item.color.isEmpty() ? circuit.style.defaultColor : item.color;
            ledCommand.effect = item.effect == VisualEffect::Unknown ? circuit.style.effect : item.effect;
            ledCommand.brightness = resolveBrightnessValue(circuit.style.brightness, circuit.style.brightness);
            ledCommands.push_back(ledCommand);
        }
    }

    if (ledCommands.empty())
    {
        runtimeState->setLastError("circuit has no renderable leds");
        return false;
    }

    return true;
}

bool CircuitController::startStepSequence(const CircuitDefinitionDto& circuit)
{
    const auto orderedSteps = getOrderedEnabledSteps(circuit);
    if (orderedSteps.empty())
    {
        runtimeState->setLastError("circuit has no renderable steps");
        return false;
    }

    sequenceActive = true;
    currentStepIndex = 0;
    currentPhase = SequencePhase::Blinking;
    phaseStartedAtMs = millis();
    lastBlinkToggleAtMs = phaseStartedAtMs;
    blinkToggleCount = 0;
    blinkLightOn = true;

    if (!renderStepSequenceState(circuit))
    {
        resetSequenceState();
        runtimeState->setLastError("led render failed");
        return false;
    }

    return true;
}

void CircuitController::resetSequenceState()
{
    sequenceActive = false;
    currentStepIndex = -1;
    currentPhase = SequencePhase::None;
    phaseStartedAtMs = 0UL;
    lastBlinkToggleAtMs = 0UL;
    blinkToggleCount = 0;
    blinkLightOn = false;
}

bool CircuitController::updateStepSequence()
{
    const auto* circuit = circuitRepository->findById(activeCircuitId);
    if (circuit == nullptr)
    {
        runtimeState->setLastError("active circuit not found");
        return false;
    }

    const auto orderedSteps = getOrderedEnabledSteps(*circuit);
    if (orderedSteps.empty() || currentStepIndex < 0 || currentStepIndex >= static_cast<int>(orderedSteps.size()))
    {
        resetSequenceState();
        return false;
    }

    const auto* currentStep = orderedSteps[currentStepIndex];
    const auto now = millis();

    if (currentPhase == SequencePhase::Blinking)
    {
        const int blinkPeriodMs = currentStep->blinkPeriodMs > 0 ? currentStep->blinkPeriodMs : 250;
        const int requiredToggles = currentStep->blinkCount > 0 ? currentStep->blinkCount * 2 : 0;

        if (requiredToggles == 0)
        {
            currentPhase = SequencePhase::Holding;
            phaseStartedAtMs = now;
            blinkLightOn = true;
            return renderStepSequenceState(*circuit);
        }

        if (now - lastBlinkToggleAtMs >= static_cast<unsigned long>(blinkPeriodMs))
        {
            lastBlinkToggleAtMs = now;
            blinkLightOn = !blinkLightOn;
            blinkToggleCount++;

            if (blinkToggleCount >= requiredToggles)
            {
                currentPhase = SequencePhase::Holding;
                phaseStartedAtMs = now;
                blinkLightOn = true;
            }

            return renderStepSequenceState(*circuit);
        }

        return true;
    }

    if (currentPhase == SequencePhase::Holding)
    {
        const unsigned long holdDurationMs = currentStep->holdDurationMs > 0 ? static_cast<unsigned long>(currentStep->holdDurationMs) : 0UL;
        if (now - phaseStartedAtMs >= holdDurationMs)
        {
            currentStepIndex++;
            if (currentStepIndex >= static_cast<int>(orderedSteps.size()))
            {
                currentStepIndex = static_cast<int>(orderedSteps.size());
                std::vector<ResolvedLedCommand> finalCommands;
                for (const auto* step : orderedSteps)
                {
                    if (!buildStaticCommandForStep(*circuit, *step, false, finalCommands))
                    {
                        return false;
                    }
                }

                ledRenderer->showCircuit(*circuit, finalCommands);
                resetSequenceState();
                runtimeState->setState(RuntimeAppState::Idle);
                activeCircuitId = "";
                runtimeState->setActiveCircuitId(activeCircuitId);
                return true;
            }

            currentPhase = SequencePhase::Blinking;
            phaseStartedAtMs = now;
            lastBlinkToggleAtMs = now;
            blinkToggleCount = 0;
            blinkLightOn = true;
            return renderStepSequenceState(*circuit);
        }
    }

    return true;
}

bool CircuitController::renderStepSequenceState(const CircuitDefinitionDto& circuit)
{
    std::vector<ResolvedLedCommand> ledCommands;
    const auto orderedSteps = getOrderedEnabledSteps(circuit);
    if (orderedSteps.empty() || currentStepIndex < 0 || currentStepIndex >= static_cast<int>(orderedSteps.size()))
    {
        runtimeState->setLastError("invalid step sequence state");
        return false;
    }

    for (int index = 0; index < currentStepIndex; index++)
    {
        if (!buildStaticCommandForStep(circuit, *orderedSteps[index], false, ledCommands))
        {
            return false;
        }
    }

    if (currentPhase == SequencePhase::Blinking)
    {
        if (blinkLightOn)
        {
            if (!buildStaticCommandForStep(circuit, *orderedSteps[currentStepIndex], true, ledCommands))
            {
                return false;
            }
        }
    }
    else if (currentPhase == SequencePhase::Holding)
    {
        if (!buildStaticCommandForStep(circuit, *orderedSteps[currentStepIndex], true, ledCommands))
        {
            return false;
        }
    }

    return ledRenderer->showCircuit(circuit, ledCommands);
}

bool CircuitController::buildStaticCommandForStep(const CircuitDefinitionDto& circuit,
                                                  const CircuitStepDto& step,
                                                  bool highlighted,
                                                  std::vector<ResolvedLedCommand>& ledCommands) const
{
    const auto* point = wallMapRepository->findPointById(step.pointId);
    if (point == nullptr)
    {
        runtimeState->setLastError("pointId not found in wall map");
        return false;
    }

    if (!point->enabled)
    {
        return true;
    }

    ResolvedLedCommand ledCommand;
    ledCommand.ledIndex = point->ledIndex;
    ledCommand.color = highlighted
                           ? (step.highlightColor.isEmpty() ? circuit.style.defaultColor : step.highlightColor)
                           : (step.dimmedColor.isEmpty() ? circuit.style.defaultColor : step.dimmedColor);
    ledCommand.effect = highlighted ? VisualEffect::Steady : VisualEffect::Steady;
    ledCommand.brightness = highlighted
                                ? resolveBrightnessValue(step.highlightBrightness, circuit.style.brightness)
                                : resolveBrightnessValue(step.dimmedBrightness, circuit.style.brightness / 3);
    ledCommands.push_back(ledCommand);
    return true;
}

std::vector<const CircuitStepDto*> CircuitController::getOrderedEnabledSteps(const CircuitDefinitionDto& circuit) const
{
    std::vector<const CircuitStepDto*> orderedSteps;
    orderedSteps.reserve(circuit.steps.size());

    for (const auto& step : circuit.steps)
    {
        if (step.enabled)
        {
            orderedSteps.push_back(&step);
        }
    }

    std::sort(orderedSteps.begin(), orderedSteps.end(), [](const CircuitStepDto* left, const CircuitStepDto* right) {
        return left->orderIndex < right->orderIndex;
    });

    return orderedSteps;
}

uint8_t CircuitController::resolveBrightnessValue(int value, int fallbackValue) const
{
    int normalizedValue = value >= 0 ? value : fallbackValue;
    if (normalizedValue < 0)
    {
        normalizedValue = 255;
    }

    if (normalizedValue > 255)
    {
        normalizedValue = 255;
    }

    return static_cast<uint8_t>(normalizedValue);
}
