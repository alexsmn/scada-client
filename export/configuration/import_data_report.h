#pragma once

#include <ostream>

class NodeService;
struct ImportData;

using u16ostream = std::basic_ostream<char16_t>;

void PrintImportReport(u16ostream& report,
                       const ImportData& import_data,
                       NodeService& node_service);
