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
    bool showAllLeds(int ledCount, const String& color, int brightness);
    bool showSingleLed(int ledIndex, const String& color, int brightness);
    bool setLedRange(int startLedNumber, int endLedNumber, bool turnOn, const String& color, int brightness);
    bool runStartupAnimation(const WallConfigDto& config);
    bool setSignText(const String& text);
    bool showSignFor(const String& text, unsigned long durationMs);
    void clearSign();
    void loop();
    const char* getStatusLabel() const;
    const String& getLastRenderedCircuitId() const;
    int getLastRenderedLedCount() const;

private:
    uint32_t parseHtmlColor(const String& color) const;
    uint8_t clampBrightness(int brightness) const;
    void clearWall();
    void renderSignFrame();
    bool signGlyphPixel(int sourceColumn, int row) const;
    int signMatrixIndex(int row, int column) const;

    bool initialized = false;
    bool circuitVisible = false;
    String lastRenderedCircuitId;
    int lastRenderedLedCount = 0;
    String signText;
    bool signActive = false;
    int signScrollColumn = 0;
    unsigned long signLastFrameAtMs = 0UL;
    unsigned long signTurnOffAtMs = 0UL;
};
