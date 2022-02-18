#pragma once

#include "base/containers/span.h"
#include "base/promise.h"
#include "base/strings/string16.h"
#include "window_definition.h"

#include <optional>

class NodeRef;

WindowDefinition MakeDeviceMetricsWindowDefinitionSync(
    std::u16string title,
    base::span<const NodeRef> devices);

promise<WindowDefinition> MakeDeviceMetricsWindowDefinition(
    const NodeRef& device);
