#pragma once

#include <Arduino.h>
#include <vector>

enum class PointKind
{
    Hole,
    Extra,
    Unknown
};

enum class VisualEffect
{
    Steady,
    Blink,
    Pulse,
    Unknown
};

enum class CircuitRole
{
    Start,
    Normal,
    Top,
    Foot,
    RightHand,
    LeftHand,
    Extra,
    Unknown
};

struct LedPointDto
{
    String pointId;
    int holeNumber = -1;
    String panelName;
    int ledIndex = -1;
    float x = 0.0f;
    float y = 0.0f;
    bool enabled = false;
    PointKind kind = PointKind::Unknown;
};

struct VisualStyleDto
{
    String defaultColor;
    int brightness = -1;
    VisualEffect effect = VisualEffect::Unknown;
    int fadeInMs = 0;
    int fadeOutMs = 0;
    int blinkPeriodMs = 0;
};

struct CircuitItemDto
{
    String pointId;
    CircuitRole role = CircuitRole::Unknown;
    String color;
    VisualEffect effect = VisualEffect::Unknown;
    bool enabled = true;
};

struct CircuitStepDto
{
    String pointId;
    int orderIndex = -1;
    int blinkCount = 0;
    int blinkPeriodMs = 0;
    int highlightBrightness = -1;
    int holdDurationMs = 0;
    int dimmedBrightness = -1;
    String highlightColor;
    String dimmedColor;
    bool autoAdvance = true;
    bool enabled = true;
};

struct CircuitDefinitionDto
{
    String circuitId;
    String name;
    String wallId;
    VisualStyleDto style;
    std::vector<CircuitItemDto> items;
    std::vector<CircuitStepDto> steps;
};

struct CircuitGlobalsDto
{
    String presetName;
    VisualEffect effect = VisualEffect::Steady;
    int defaultBrightness = 96;
    int dimmedBrightness = 48;
    String rightHandColor = "#C44536";
    String leftHandColor = "#247BA0";
    String startColor = "#FFFF00";
    String topColor = "#FF0000";
    int blinkCount = 3;
    int blinkPeriodMs = 250;
    int holdDurationMs = 2500;
};

struct CircuitMovementEditorialDto
{
    int pointRef = -1;
    int hand = -1;
    int role = -1;
    int sequence = -1;
};

struct CircuitEditorialDefinitionDto
{
    String circuitId;
    String name;
    String wallId;
    String difficulty;
    String inclination;
    CircuitGlobalsDto globals;
    std::vector<CircuitMovementEditorialDto> movements;
};

struct WallConfigDto
{
    String wallId;
    String wallName;
    String roomId;
    String roomName;
    String controllerId;
    int ledCount = 0;
    int brightnessLimit = 0;
    std::vector<LedPointDto> points;
};

const char* pointKindToString(PointKind value);
PointKind pointKindFromString(const String& value);

const char* visualEffectToString(VisualEffect value);
VisualEffect visualEffectFromString(const String& value);

const char* circuitRoleToString(CircuitRole value);
CircuitRole circuitRoleFromString(const String& value);
