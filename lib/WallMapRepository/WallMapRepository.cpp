#include "WallMapRepository.h"

#include <set>

void WallMapRepository::clear()
{
    wallId = "";
    ledCount = 0;
    disabledPoints = 0;
    configLoaded = false;
    currentConfig = {};
}

void WallMapRepository::setConfigSummary(const String& nextWallId, int nextLedCount, int nextDisabledPoints)
{
    wallId = nextWallId;
    ledCount = nextLedCount;
    disabledPoints = nextDisabledPoints;
    configLoaded = !wallId.isEmpty() && ledCount > 0;
}

bool WallMapRepository::setConfig(const WallConfigDto& config, String& validationError)
{
    if (!validateConfig(config, validationError))
    {
        return false;
    }

    currentConfig = config;
    wallId = config.wallId;
    ledCount = config.ledCount;
    disabledPoints = 0;
    for (const auto& point : config.points)
    {
        if (!point.enabled)
        {
            disabledPoints++;
        }
    }

    configLoaded = true;
    return true;
}

bool WallMapRepository::hasConfig() const
{
    return configLoaded;
}

const String& WallMapRepository::getWallId() const
{
    return wallId;
}

int WallMapRepository::getLedCount() const
{
    return ledCount;
}

int WallMapRepository::getDisabledPoints() const
{
    return disabledPoints;
}

const WallConfigDto& WallMapRepository::getConfig() const
{
    return currentConfig;
}

bool WallMapRepository::hasPointId(const String& pointId) const
{
    return findPointById(pointId) != nullptr;
}

const LedPointDto* WallMapRepository::findPointById(const String& pointId) const
{
    for (const auto& point : currentConfig.points)
    {
        if (point.pointId == pointId)
        {
            return &point;
        }
    }

    return nullptr;
}

const LedPointDto* WallMapRepository::findPointByHoleNumber(int holeNumber) const
{
    for (const auto& point : currentConfig.points)
    {
        if (point.holeNumber == holeNumber)
        {
            return &point;
        }
    }

    return nullptr;
}

bool WallMapRepository::validateConfig(const WallConfigDto& config, String& validationError) const
{
    if (config.wallId.isEmpty())
    {
        validationError = "wallId is required";
        return false;
    }

    if (config.wallName.isEmpty())
    {
        validationError = "wallName is required";
        return false;
    }

    if (config.roomId.isEmpty())
    {
        validationError = "roomId is required";
        return false;
    }

    if (config.roomName.isEmpty())
    {
        validationError = "roomName is required";
        return false;
    }

    if (config.controllerId.isEmpty())
    {
        validationError = "controllerId is required";
        return false;
    }

    if (config.ledCount <= 0)
    {
        validationError = "ledCount must be > 0";
        return false;
    }

    if (config.brightnessLimit < 0 || config.brightnessLimit > 255)
    {
        validationError = "brightnessLimit must be between 0 and 255";
        return false;
    }

    if (config.points.empty())
    {
        validationError = "points must not be empty";
        return false;
    }

    std::set<String> pointIds;
    std::set<int> ledIndexes;

    for (const auto& point : config.points)
    {
        if (point.pointId.isEmpty())
        {
            validationError = "pointId is required";
            return false;
        }

        if (pointIds.find(point.pointId) != pointIds.end())
        {
            validationError = "duplicate pointId";
            return false;
        }

        pointIds.insert(point.pointId);

        if (point.ledIndex < 0 || point.ledIndex >= config.ledCount)
        {
            validationError = "ledIndex out of range";
            return false;
        }

        if (ledIndexes.find(point.ledIndex) != ledIndexes.end())
        {
            validationError = "duplicate ledIndex";
            return false;
        }

        ledIndexes.insert(point.ledIndex);

        if (point.kind == PointKind::Unknown)
        {
            validationError = "invalid point kind";
            return false;
        }
    }

    validationError = "";
    return true;
}
