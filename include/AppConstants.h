#pragma once

namespace AppConstants
{
constexpr const char* FirmwareVersion = APP_FW_VERSION;
constexpr const char* ApiVersion = APP_API_VERSION;
constexpr const char* DeviceName = "KKKKK-ESP32";
constexpr const char* ProvisioningSsid = "KKKKK-ESP32-Setup";
constexpr const char* ProvisioningPassword = "kkkkk1234";

constexpr unsigned long SerialBaudRate = 115200UL;
constexpr unsigned long WifiConnectTimeoutMs = 15000UL;
constexpr unsigned long StatusLogIntervalMs = 10000UL;
constexpr int LedDataPin = 5;
constexpr int MaxLedCount = 512;
constexpr int DefaultLedBrightness = 96;
constexpr int SignLedDataPin = 23;
constexpr int SignLedPhysicalCount = 50;
constexpr int SignMatrixRows = 5;
constexpr int SignMatrixColumns = 8;
constexpr unsigned long SignScrollFrameMs = 120UL;
constexpr unsigned long StartupSignDurationMs = 5000UL;
constexpr int ButtonNextCircuitPin = 18;
constexpr int ButtonStartStopPin = 19;
constexpr int ButtonResetPin = 21;
constexpr unsigned long ButtonPollIntervalMs = 20UL;
constexpr unsigned long ButtonDebounceMs = 50UL;
constexpr unsigned long ButtonLongPressMs = 1500UL;
} // namespace AppConstants
