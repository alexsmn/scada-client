#pragma once

#include "base/any_executor.h"
#include "base/awaitable.h"
#include "base/promise.h"
#include "profile/window_definition.h"

#include <optional>
#include <span>

class NodeRef;

WindowDefinition MakeDeviceMetricsWindowDefinitionSync(
    std::u16string title,
    std::span<const NodeRef> devices);

promise<WindowDefinition> MakeDeviceMetricsWindowDefinition(
    AnyExecutor executor,
    const NodeRef& device);

Awaitable<WindowDefinition> MakeDeviceMetricsWindowDefinitionAsync(
    AnyExecutor executor,
    const NodeRef& device);
