#pragma once

#include "base/any_executor.h"

#include "base/awaitable.h"
#include "scada/data_services.h"

#include <memory>
#include <optional>

struct DataServices;
struct DataServicesContext;

// Coroutine: `services_context` is taken by value (CP.53) — the dialog is
// created on first resume, which happens after the caller's frame may have
// died, so a reference parameter would dangle.
Awaitable<std::optional<DataServices>> ExecuteLoginDialog(
    AnyExecutor executor,
    DataServicesContext services_context);
