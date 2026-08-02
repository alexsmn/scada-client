#pragma once

class NodeService;
struct DiffData;

void ShowDiffReport(const DiffData& diff_data, NodeService& node_service);

class ScopedImportReportSuppressor {
 public:
  ScopedImportReportSuppressor();
  ~ScopedImportReportSuppressor();
};