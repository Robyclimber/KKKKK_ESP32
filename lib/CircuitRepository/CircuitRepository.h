#pragma once

#include <Arduino.h>
#include <vector>

#include "DomainTypes.h"

class CircuitRepository
{
public:
    void clear();
    bool hasCircuits() const;
    void setCircuitsCount(int value);
    int getCircuitsCount() const;
    bool setCircuits(const String& wallId, const std::vector<CircuitDefinitionDto>& circuits, String& validationError);
    const String& getWallId() const;
    const std::vector<CircuitDefinitionDto>& getCircuits() const;
    const CircuitDefinitionDto* findById(const String& circuitId) const;

private:
    bool validateCircuits(const String& wallId, const std::vector<CircuitDefinitionDto>& circuits, String& validationError) const;

    bool available = false;
    int circuitsCount = 0;
    String currentWallId;
    std::vector<CircuitDefinitionDto> currentCircuits;
};
