#include "services/data_services_factory.h"

#include "base/strings/string_util.h"
#include "services/vidicon/vidicon_session.h"

DataServices CreateVidiconServices(const DataServicesContext& context) {
  auto vidicon_session = std::make_shared<VidiconSession>();
  return {
      vidicon_session,
      vidicon_session,
      vidicon_session,
      vidicon_session,
      vidicon_session,
      vidicon_session,
      vidicon_session,
      vidicon_session,
  };
}

