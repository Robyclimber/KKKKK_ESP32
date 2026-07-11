#include "CircuitRepository.h"

#include <set>

void CircuitRepository::clear()
{
    available = false;
    circuitsCount = 0;
    currentWallId = "";
    currentCircuits.clear();
}

bool CircuitRepository::hasCircuits() const
{
    return available;
}

void CircuitRepository::setCircuitsCount(int value)
{
    circuitsCount = value;
    available = circuitsCount > 0;
}

int CircuitRepository::getCircuitsCount() const
{
    return circuitsCount;
}

bool CircuitRepository::setCircuits(const String& wallId, const std::vector<CircuitDefinitionDto>& circuits, String& validationError)
{
    if (!validateCircuits(wallId, circuits, validationError))
    {
        return false;
    }

    currentWallId = wallId;
    currentCircuits = circuits;
    circuitsCount = static_cast<int>(currentCircuits.size());
    available = circuitsCount > 0;
    return true;
}

const String& CircuitRepository::getWallId() const
{
    return currentWallId;
}

const std::vector<CircuitDefinitionDto>& CircuitRepository::getCircuits() const
{
    return currentCircuits;
}

const CircuitDefinitionDto* CircuitRepository::findById(const String& circuitId) const
{
    for (const auto& circuit : currentCircuits)
    {
        if (circuit.circuitId == circuitId)
        {
            return &circuit;
        }
    }

    return nullptr;
}

bool CircuitRepository::validateCircuits(const String& wallId, const std::vector<CircuitDefinitionDto>& circuits, String& validationError) const
{
    if (wallId.isEmpty())
    {
        validationError = "wallId is required";
        return false;
    }

    if (circuits.empty())
    {
        validationError = "circuits must not be empty";
        return false;
    }

    std::set<String> circuitIds;
    for (const auto& circuit : circuits)
    {
        if (circuit.circuitId.isEmpty())
        {
            validationError = "circuitId is required";
            return false;
        }

        if (circuit.name.isEmpty())
        {
            validationError = "circuit name is required";
            return false;
        }

        if (circuit.wallId.isEmpty() || circuit.wallId != wallId)
        {
            validationError = "circuit wallId mismatch";
            return false;
        }

        if (circuitIds.find(circuit.circuitId) != circuitIds.end())
        {
            validationError = "duplicate circuitId";
            return false;
        }

        if (circuit.items.empty() && circuit.steps.empty())
        {
            validationError = "circuit must contain items or steps";
            return false;
        }

        if (circuit.style.brightness < -1 || circuit.style.brightness > 255)
        {
            validationError = "style brightness out of range";
            return false;
        }

        std::set<String> pointIds;
        for (const auto& item : circuit.items)
        {
            if (item.pointId.isEmpty())
            {
                validationError = "circuit item pointId is required";
                return false;
            }

            if (item.role == CircuitRole::Unknown)
            {
                validationError = "circuit item role is invalid";
                return false;
            }

            if (pointIds.find(item.pointId) != pointIds.end())
            {
                validationError = "duplicate pointId in circuit";
                return false;
            }

            pointIds.insert(item.pointId);
        }

        if (!circuit.steps.empty())
        {
            std::set<int> orderIndexes;
            for (const auto& step : circuit.steps)
            {
                if (step.pointId.isEmpty())
                {
                    validationError = "circuit step pointId is required";
                    return false;
                }

                if (step.orderIndex < 0)
                {
                    validationError = "circuit step orderIndex is required";
                    return false;
                }

                if (orderIndexes.find(step.orderIndex) != orderIndexes.end())
                {
                    validationError = "duplicate orderIndex in circuit steps";
                    return false;
                }

                if (step.blinkCount < 0)
                {
                    validationError = "step blinkCount must be >= 0";
                    return false;
                }

                if (step.blinkCount > 0 && step.blinkPeriodMs <= 0)
                {
                    validationError = "step blinkPeriodMs must be > 0";
                    return false;
                }

                if (step.highlightBrightness < -1 || step.highlightBrightness > 255)
                {
                    validationError = "step highlightBrightness out of range";
                    return false;
                }

                if (step.dimmedBrightness < -1 || step.dimmedBrightness > 255)
                {
                    validationError = "step dimmedBrightness out of range";
                    return false;
                }

                if (step.highlightBrightness >= 0 &&
                    step.dimmedBrightness >= 0 &&
                    step.dimmedBrightness > step.highlightBrightness)
                {
                    validationError = "step dimmedBrightness cannot exceed highlightBrightness";
                    return false;
                }

                orderIndexes.insert(step.orderIndex);
            }
        }

        circuitIds.insert(circuit.circuitId);
    }

    validationError = "";
    return true;
}
