#pragma once

#include "base/promise.h"
#include "scada/data_services.h"

#include <memory>
#include <optional>

class Executor;
struct DataServices;
struct DataServicesContext;

promise<std::optional<DataServices>> ExecuteLoginDialog(
    std::shared_ptr<Executor> executor,
    DataServicesContext&& services_context);
