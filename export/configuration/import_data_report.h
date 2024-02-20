#pragma once

class NodeService;
struct ImportData;

void ShowImportReport(const ImportData& import_data, NodeService& node_service);

class ScopedImportReportSuppressor {
 public:
  ScopedImportReportSuppressor();
  ~ScopedImportReportSuppressor();
};