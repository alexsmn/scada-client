#pragma once

#include "export/configuration/import_data.h"

class NodeService;
struct ExportData;

ImportData BuildImportData(NodeService& node_service,
                           const ExportData& export_data);
