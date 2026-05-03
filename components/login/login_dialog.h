#pragma once

#include "base/any_executor.h"

#include "base/awaitable.h"
#include "scada/data_services.h"

#include <memory>
#include <optional>

struct DataServices;
struct DataServicesContext;

Awaitable<std::optional<DataServices>> ExecuteLoginDialog(
    AnyExecutor executor,
    DataServicesContext&& services_context);
