#include "data_services_factory.h"

#include "base/strings/string_util.h"

extern DataServices CreateVidiconServices(const DataServicesContext& context);
extern DataServices CreateScadaServices(const DataServicesContext& context);
extern DataServices CreateOpcUaServices(const DataServicesContext& context);

REGISTER_DATA_SERVICES("Scada", L"Телеконтроль", CreateScadaServices);
REGISTER_DATA_SERVICES("Vidicon", L"Видикон", CreateVidiconServices);
REGISTER_DATA_SERVICES("OpcUa", L"OPC UA", CreateOpcUaServices);

namespace {

DataServicesInfoList& GetMutableDataServicesInfoList() {
  static DataServicesInfoList s_list;
  return s_list;
}

} // namespace

const DataServicesInfoList& GetDataServicesInfoList() {
  return GetMutableDataServicesInfoList();
}

void RegisterDataServices(DataServicesInfo info) {
  GetMutableDataServicesInfoList().emplace_back(std::move(info));
}

bool EqualDataServicesName(base::StringPiece name1, base::StringPiece name2) {
  return base::EqualsCaseInsensitiveASCII(name1, name2);
}

bool CreateDataServices(base::StringPiece name, const DataServicesContext& context, DataServices& services) {
  for (auto& info : GetDataServicesInfoList()) {
    if (EqualDataServicesName(info.name, name)) {
      services = info.factory_method(context);
      return true;
    }
  }
  return false;
}