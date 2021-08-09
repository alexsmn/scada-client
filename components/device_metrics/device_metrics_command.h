#pragma once

#include "base/containers/span.h"
#include "base/promise.h"
#include "window_definition.h"

#include <optional>

class NodeRef;

WindowDefinition MakeDeviceMetricsWindowDefinitionSync(
    std::wstring title,
    base::span<const NodeRef> devices);

promise<WindowDefinition> MakeDeviceMetricsWindowDefinition(
    const NodeRef& device);
