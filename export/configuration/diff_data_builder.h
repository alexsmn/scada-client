#pragma once

#include "export/configuration/diff_data.h"

class NodeService;
struct ExportData;

DiffData BuildDiffData(const ExportData& old_export_data,
                       const ExportData& new_export_data);
