#pragma once

struct DataServices;
struct DataServicesContext;

bool ExecuteLoginDialog(DataServicesContext&& services_context,
                        DataServices& services);
