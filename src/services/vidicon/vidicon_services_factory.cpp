#include "services/data_services_factory.h"

#include "base/strings/string_util.h"
#include "services/vidicon/vidicon_session.h"

bool CreateVidiconServices(const DataServicesContext& context, DataServices& services) {
  auto vidicon_session = std::make_shared<VidiconSession>();
  services = {
      vidicon_session,
      vidicon_session,
      vidicon_session,
      vidicon_session,
      vidicon_session,
      vidicon_session,
      vidicon_session,
      vidicon_session,
  };
  return true;
}

