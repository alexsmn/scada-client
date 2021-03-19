#pragma once

#include <ostream>

class NodeService;
struct ImportData;

void PrintImportReport(std::wostream& report,
                       const ImportData& import_data,
                       NodeService& node_service);
