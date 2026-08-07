#include "DomainTypes.h"

const char* pointKindToString(PointKind value)
{
    switch (value)
    {
        case PointKind::Hole:
            return "hole";
        case PointKind::Extra:
            return "extra";
        case PointKind::Unknown:
        default:
            return "unknown";
    }
}

PointKind pointKindFromString(const String& value)
{
    if (value.equalsIgnoreCase("hole"))
    {
        return PointKind::Hole;
    }

    if (value.equalsIgnoreCase("extra"))
    {
        return PointKind::Extra;
    }

    return PointKind::Unknown;
}

const char* visualEffectToString(VisualEffect value)
{
    switch (value)
    {
        case VisualEffect::Steady:
            return "steady";
        case VisualEffect::Blink:
            return "blink";
        case VisualEffect::Pulse:
            return "pulse";
        case VisualEffect::Unknown:
        default:
            return "unknown";
    }
}

VisualEffect visualEffectFromString(const String& value)
{
    if (value.equalsIgnoreCase("steady"))
    {
        return VisualEffect::Steady;
    }

    if (value.equalsIgnoreCase("blink"))
    {
        return VisualEffect::Blink;
    }

    if (value.equalsIgnoreCase("pulse"))
    {
        return VisualEffect::Pulse;
    }

    return VisualEffect::Unknown;
}

const char* circuitRoleToString(CircuitRole value)
{
    switch (value)
    {
        case CircuitRole::Start:
            return "start";
        case CircuitRole::Normal:
            return "normal";
        case CircuitRole::Top:
            return "top";
        case CircuitRole::Foot:
            return "foot";
        case CircuitRole::RightHand:
            return "rightHand";
        case CircuitRole::LeftHand:
            return "leftHand";
        case CircuitRole::Extra:
            return "extra";
        case CircuitRole::Unknown:
        default:
            return "unknown";
    }
}

CircuitRole circuitRoleFromString(const String& value)
{
    if (value.equalsIgnoreCase("start"))
    {
        return CircuitRole::Start;
    }

    if (value.equalsIgnoreCase("normal"))
    {
        return CircuitRole::Normal;
    }

    if (value.equalsIgnoreCase("top"))
    {
        return CircuitRole::Top;
    }

    if (value.equalsIgnoreCase("foot"))
    {
        return CircuitRole::Foot;
    }

    if (value.equalsIgnoreCase("rightHand"))
    {
        return CircuitRole::RightHand;
    }

    if (value.equalsIgnoreCase("leftHand"))
    {
        return CircuitRole::LeftHand;
    }

    if (value.equalsIgnoreCase("extra"))
    {
        return CircuitRole::Extra;
    }

    return CircuitRole::Unknown;
}
