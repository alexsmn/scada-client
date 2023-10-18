#pragma once

#include "base/promise.h"
#include "controller/window_definition.h"

#include <optional>
#include <span>

class NodeRef;

WindowDefinition MakeDeviceMetricsWindowDefinitionSync(
    std::u16string title,
    std::span<const NodeRef> devices);

promise<WindowDefinition> MakeDeviceMetricsWindowDefinition(
    const NodeRef& device);
