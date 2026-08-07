#include "LedRenderer.h"

#include <FastLED.h>
#include <cmath>

#include "AppConstants.h"

namespace
{
CRGB leds[AppConstants::MaxLedCount];

float distanceToSegment(float px, float py, float ax, float ay, float bx, float by)
{
    const float dx = bx - ax;
    const float dy = by - ay;
    const float lengthSquared = dx * dx + dy * dy;
    if (lengthSquared <= 0.000001f)
    {
        const float ex = px - ax;
        const float ey = py - ay;
        return std::sqrt(ex * ex + ey * ey);
    }

    float projection = ((px - ax) * dx + (py - ay) * dy) / lengthSquared;
    projection = projection < 0.0f ? 0.0f : (projection > 1.0f ? 1.0f : projection);
    const float nearestX = ax + projection * dx;
    const float nearestY = ay + projection * dy;
    const float ex = px - nearestX;
    const float ey = py - nearestY;
    return std::sqrt(ex * ex + ey * ey);
}

bool nearSegment(float x, float y, float ax, float ay, float bx, float by, float thickness)
{
    return distanceToSegment(x, y, ax, ay, bx, by) <= thickness;
}
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

bool LedRenderer::showAllLeds(int ledCount, const String& color, int brightness)
{
    if (!initialized || ledCount <= 0 || ledCount > AppConstants::MaxLedCount)
    {
        return false;
    }

    FastLED.clear(false);
    FastLED.setBrightness(clampBrightness(brightness));
    const CRGB ledColor = static_cast<uint32_t>(parseHtmlColor(color));

    for (int ledIndex = 0; ledIndex < ledCount; ledIndex++)
    {
        leds[ledIndex] = ledColor;
    }

    FastLED.show();
    circuitVisible = true;
    lastRenderedCircuitId = "__all_leds_test__";
    lastRenderedLedCount = ledCount;
    return true;
}

bool LedRenderer::setLedRange(int startLedNumber,
                              int endLedNumber,
                              bool turnOn,
                              const String& color,
                              int brightness)
{
    if (!initialized || startLedNumber <= 0 || endLedNumber < startLedNumber ||
        endLedNumber > AppConstants::MaxLedCount)
    {
        return false;
    }

    FastLED.setBrightness(clampBrightness(brightness));
    const CRGB ledColor = turnOn ? static_cast<uint32_t>(parseHtmlColor(color)) : CRGB::Black;

    // The public API is intentionally 1-based; the LED buffer is 0-based.
    for (int ledNumber = startLedNumber; ledNumber <= endLedNumber; ledNumber++)
    {
        leds[ledNumber - 1] = ledColor;
    }

    FastLED.show();
    circuitVisible = turnOn;
    lastRenderedCircuitId = "__led_range_test__";
    lastRenderedLedCount = endLedNumber - startLedNumber + 1;
    return true;
}

bool LedRenderer::runStartupAnimation(const WallConfigDto& config)
{
    if (!initialized || config.points.empty())
    {
        return false;
    }

    float minX = config.points.front().x;
    float maxX = minX;
    float minY = config.points.front().y;
    float maxY = minY;
    int enabledCount = 0;
    for (const auto& point : config.points)
    {
        if (!point.enabled || point.ledIndex < 0 || point.ledIndex >= AppConstants::MaxLedCount)
        {
            continue;
        }

        minX = point.x < minX ? point.x : minX;
        maxX = point.x > maxX ? point.x : maxX;
        minY = point.y < minY ? point.y : minY;
        maxY = point.y > maxY ? point.y : maxY;
        enabledCount++;
    }

    const float width = maxX - minX;
    const float height = maxY - minY;
    if (enabledCount == 0 || width <= 0.001f || height <= 0.001f)
    {
        return false;
    }

    const uint8_t brightness = static_cast<uint8_t>(config.brightnessLimit > 0 && config.brightnessLimit < 256
                                                        ? config.brightnessLimit
                                                        : AppConstants::DefaultLedBrightness);
    FastLED.setBrightness(brightness);
    const unsigned long startedAt = millis();
    constexpr unsigned long durationMs = 15000UL;
    constexpr unsigned long logoStartMs = 11200UL;
    int finalLogoLedCount = 0;

    while (millis() - startedAt < durationMs)
    {
        const unsigned long elapsed = millis() - startedAt;
        FastLED.clear(false);
        finalLogoLedCount = 0;

        int ordinal = 0;
        for (const auto& point : config.points)
        {
            if (!point.enabled || point.ledIndex < 0 || point.ledIndex >= AppConstants::MaxLedCount)
            {
                continue;
            }

            const float x = (point.x - minX) / width;
            const float y = (point.y - minY) / height;
            CRGB color = CRGB::Black;

            if (elapsed < 2800UL)
            {
                const uint8_t ramp = static_cast<uint8_t>((elapsed * 255UL) / 2800UL);
                const uint8_t pulse = static_cast<uint8_t>(120 + 135 * std::fabs(std::sin((elapsed + ordinal * 19) * 0.012f)));
                color = CHSV(static_cast<uint8_t>(ordinal * 7 + elapsed / 8), 255, static_cast<uint8_t>((ramp * pulse) / 255));
            }
            else if (elapsed < 6500UL)
            {
                const float wave = std::sin((x * 16.0f) - elapsed * 0.014f) + std::sin((y * 19.0f) + elapsed * 0.018f);
                const uint8_t value = std::fabs(wave) > 1.15f ? 255 : 55;
                color = CHSV(static_cast<uint8_t>(elapsed / 7 + x * 150.0f + y * 70.0f), 255, value);
            }
            else if (elapsed < 9000UL)
            {
                const float sweep = std::fmod(elapsed * 0.00125f, 1.45f) - 0.2f;
                const float distance = std::fabs((x * 0.72f + (1.0f - y) * 0.28f) - sweep);
                if (distance < 0.045f)
                {
                    color = CRGB::White;
                }
                else if (distance < 0.13f)
                {
                    color = CHSV(static_cast<uint8_t>(15 + ordinal * 3), 255, 230);
                }
                else
                {
                    color = CHSV(static_cast<uint8_t>(ordinal * 5 + elapsed / 12), 255, 35);
                }
            }
            else if (elapsed < logoStartMs)
            {
                const uint32_t hash = static_cast<uint32_t>(ordinal * 1103515245UL + (elapsed / 70UL) * 12345UL);
                const bool spark = (hash % 17UL) < 3UL;
                if (spark)
                {
                    color = CRGB::White;
                }
                else
                {
                    color = CHSV(static_cast<uint8_t>(ordinal * 11 + elapsed / 5), 255, 75);
                }
            }
            else
            {
                const float reveal = static_cast<float>(elapsed - logoStartMs) / 1500.0f;
                const float thickness = 0.038f;
                const bool mountain =
                    nearSegment(x, y, 0.02f, 0.78f, 0.23f, 0.40f, thickness) ||
                    nearSegment(x, y, 0.23f, 0.40f, 0.42f, 0.73f, thickness) ||
                    nearSegment(x, y, 0.31f, 0.76f, 0.60f, 0.27f, thickness) ||
                    nearSegment(x, y, 0.60f, 0.27f, 0.97f, 0.79f, thickness);
                const bool snow =
                    nearSegment(x, y, 0.18f, 0.49f, 0.23f, 0.40f, thickness * 0.8f) ||
                    nearSegment(x, y, 0.23f, 0.40f, 0.28f, 0.49f, thickness * 0.8f) ||
                    nearSegment(x, y, 0.54f, 0.37f, 0.60f, 0.27f, thickness * 0.8f) ||
                    nearSegment(x, y, 0.60f, 0.27f, 0.67f, 0.37f, thickness * 0.8f);
                const float headDistance = std::sqrt((x - 0.52f) * (x - 0.52f) + (y - 0.34f) * (y - 0.34f));
                const bool head = std::fabs(headDistance - 0.055f) <= thickness;
                const bool climber = head ||
                    nearSegment(x, y, 0.52f, 0.40f, 0.51f, 0.58f, thickness) ||
                    nearSegment(x, y, 0.51f, 0.45f, 0.39f, 0.52f, thickness) ||
                    nearSegment(x, y, 0.51f, 0.46f, 0.67f, 0.39f, thickness) ||
                    nearSegment(x, y, 0.51f, 0.58f, 0.42f, 0.73f, thickness) ||
                    nearSegment(x, y, 0.51f, 0.58f, 0.64f, 0.72f, thickness);

                if ((mountain || climber) && (reveal >= 1.0f || x <= reveal))
                {
                    color = climber ? CRGB(255, 95, 8) : (snow ? CRGB::White : CRGB(0, 95, 255));
                    finalLogoLedCount++;
                }
            }

            leds[point.ledIndex] = color;
            ordinal++;
        }

        FastLED.show();
        delay(33);
    }

    circuitVisible = true;
    lastRenderedCircuitId = "__startup_routelab_logo__";
    lastRenderedLedCount = finalLogoLedCount;
    return finalLogoLedCount > 0;
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
