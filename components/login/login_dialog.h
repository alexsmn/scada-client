#pragma once

#include <memory>

class Executor;
struct DataServices;
struct DataServicesContext;

bool ExecuteLoginDialog(std::shared_ptr<Executor> executor,
                        DataServicesContext&& services_context,
                        DataServices& services);
