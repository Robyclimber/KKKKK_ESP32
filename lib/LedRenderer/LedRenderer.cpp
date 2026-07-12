#include "LedRenderer.h"

#include <FastLED.h>

#include "AppConstants.h"

namespace
{
CRGB leds[AppConstants::MaxLedCount];
}

void LedRenderer::begin()
{
    FastLED.addLeds<WS2811, AppConstants::LedDataPin, GRB>(leds, AppConstants::MaxLedCount);
    FastLED.setBrightness(AppConstants::DefaultLedBrightness);
    FastLED.clear(true);
    initialized = true;
}

void LedRenderer::clear()
{
    if (initialized)
    {
        FastLED.clear(true);
    }

    circuitVisible = false;
    lastRenderedCircuitId = "";
    lastRenderedLedCount = 0;
}

bool LedRenderer::showCircuit(const CircuitDefinitionDto& circuit, const std::vector<ResolvedLedCommand>& ledCommands)
{
    if (!initialized)
    {
        return false;
    }

    FastLED.clear(false);
    FastLED.setBrightness(clampBrightness(circuit.style.brightness));

    for (const auto& ledCommand : ledCommands)
    {
        if (ledCommand.ledIndex < 0 || ledCommand.ledIndex >= AppConstants::MaxLedCount)
        {
            continue;
        }

        CRGB ledColor = static_cast<uint32_t>(parseHtmlColor(ledCommand.color));
        ledColor.nscale8_video(ledCommand.brightness);
        leds[ledCommand.ledIndex] = ledColor;
    }

    FastLED.show();
    circuitVisible = true;
    lastRenderedCircuitId = circuit.circuitId;
    lastRenderedLedCount = static_cast<int>(ledCommands.size());
    return true;
}

const char* LedRenderer::getStatusLabel() const
{
    if (!initialized)
    {
        return "NotInitialized";
    }

    return circuitVisible ? "CircuitVisible" : "Ready";
}

const String& LedRenderer::getLastRenderedCircuitId() const
{
    return lastRenderedCircuitId;
}

int LedRenderer::getLastRenderedLedCount() const
{
    return lastRenderedLedCount;
}

uint32_t LedRenderer::parseHtmlColor(const String& color) const
{
    if (color.isEmpty())
    {
        return CRGB::White;
    }

    if (color[0] == '#' && color.length() == 7)
    {
        const long parsedValue = strtol(color.substring(1).c_str(), nullptr, 16);
        return static_cast<uint32_t>(parsedValue);
    }

    if (color.equalsIgnoreCase("red"))
    {
        return CRGB::Red;
    }

    if (color.equalsIgnoreCase("green"))
    {
        return CRGB::Green;
    }

    if (color.equalsIgnoreCase("blue"))
    {
        return CRGB::Blue;
    }

    if (color.equalsIgnoreCase("yellow"))
    {
        return CRGB::Yellow;
    }

    if (color.equalsIgnoreCase("orange"))
    {
        return CRGB::Orange;
    }

    if (color.equalsIgnoreCase("purple"))
    {
        return CRGB::Purple;
    }

    if (color.equalsIgnoreCase("pink"))
    {
        return CRGB::HotPink;
    }

    if (color.equalsIgnoreCase("cyan"))
    {
        return CRGB::Cyan;
    }

    if (color.equalsIgnoreCase("white"))
    {
        return CRGB::White;
    }

    return CRGB::White;
}

uint8_t LedRenderer::clampBrightness(int brightness) const
{
    if (brightness < 0)
    {
        return static_cast<uint8_t>(AppConstants::DefaultLedBrightness);
    }

    if (brightness > 255)
    {
        return 255;
    }

    return static_cast<uint8_t>(brightness);
}
