#pragma once

#include "base/promise.h"
#include "window_definition.h"

#include <optional>

class NodeRef;

promise<WindowDefinition> MakeDeviceMetricsWindowDefinition(
    const NodeRef& device);
