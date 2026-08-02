#pragma once

#include <functional>

enum class TelemetryType { Startup, Shutdown };

struct TelemetryEvent {
  TelemetryType type;
};

using TelemetrySender = std::function<void(const TelemetryEvent& event)>;
