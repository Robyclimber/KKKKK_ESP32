#pragma once

#include <Arduino.h>
#include <vector>

#include "CircuitRepository.h"
#include "LedRenderer.h"
#include "RuntimeState.h"
#include "WallMapRepository.h"

class CircuitController
{
public:
    void begin(RuntimeState* runtimeState, CircuitRepository* circuitRepository, WallMapRepository* wallMapRepository, LedRenderer* ledRenderer);
    void loop();

    bool show(const String& circuitId);
    bool showPreview(const CircuitDefinitionDto& circuit);
    bool stop();
    bool reset();
    bool clear();
    const String& getActiveCircuitId() const;
    bool isSequenceActive() const;
    int getCurrentStepIndex() const;
    const char* getSequencePhaseLabel() const;
    unsigned long getCurrentPhaseRemainingMs() const;

private:
    const CircuitDefinitionDto* getActiveCircuitDefinition() const;
    bool resolveCircuitLedCommands(const CircuitDefinitionDto& circuit, std::vector<ResolvedLedCommand>& ledCommands);
    bool startStepSequence(const CircuitDefinitionDto& circuit);
    void resetSequenceState();
    bool updateStepSequence();
    bool renderStepSequenceState(const CircuitDefinitionDto& circuit);
    bool buildStaticCommandForStep(const CircuitDefinitionDto& circuit,
                                   const CircuitStepDto& step,
                                   bool highlighted,
                                   std::vector<ResolvedLedCommand>& ledCommands) const;
    std::vector<const CircuitStepDto*> getOrderedEnabledSteps(const CircuitDefinitionDto& circuit) const;
    uint8_t resolveBrightnessValue(int value, int fallbackValue) const;

    enum class SequencePhase
    {
        None,
        Blinking,
        Holding
    };

    RuntimeState* runtimeState = nullptr;
    CircuitRepository* circuitRepository = nullptr;
    WallMapRepository* wallMapRepository = nullptr;
    LedRenderer* ledRenderer = nullptr;
    String activeCircuitId;
    bool previewCircuitActive = false;
    CircuitDefinitionDto previewCircuit;
    bool sequenceActive = false;
    int currentStepIndex = -1;
    SequencePhase currentPhase = SequencePhase::None;
    unsigned long phaseStartedAtMs = 0UL;
    unsigned long lastBlinkToggleAtMs = 0UL;
    int blinkToggleCount = 0;
    bool blinkLightOn = false;
};
