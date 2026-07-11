#pragma once

#include <Arduino.h>
#include <vector>

#include "DomainTypes.h"

struct ResolvedLedCommand
{
    int ledIndex = -1;
    String color;
    VisualEffect effect = VisualEffect::Steady;
    uint8_t brightness = 255;
};

class LedRenderer
{
public:
    void begin();
    void clear();
    bool showCircuit(const CircuitDefinitionDto& circuit, const std::vector<ResolvedLedCommand>& ledCommands);
    const char* getStatusLabel() const;
    const String& getLastRenderedCircuitId() const;
    int getLastRenderedLedCount() const;

private:
    uint32_t parseHtmlColor(const String& color) const;
    uint8_t clampBrightness(int brightness) const;

    bool initialized = false;
    bool circuitVisible = false;
    String lastRenderedCircuitId;
    int lastRenderedLedCount = 0;
};
