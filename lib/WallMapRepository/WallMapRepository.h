#pragma once

#include <Arduino.h>
#include <vector>

#include "DomainTypes.h"

class WallMapRepository
{
public:
    void clear();
    void setConfigSummary(const String& wallId, int ledCount, int disabledPoints);
    bool setConfig(const WallConfigDto& config, String& validationError);
    bool hasConfig() const;
    const String& getWallId() const;
    int getLedCount() const;
    int getDisabledPoints() const;
    const WallConfigDto& getConfig() const;
    bool hasPointId(const String& pointId) const;
    const LedPointDto* findPointById(const String& pointId) const;

private:
    bool validateConfig(const WallConfigDto& config, String& validationError) const;

    String wallId;
    int ledCount = 0;
    int disabledPoints = 0;
    bool configLoaded = false;
    WallConfigDto currentConfig;
};
