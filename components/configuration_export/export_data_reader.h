#pragma once

#include "components/configuration_export/export_data.h"

#include <optional>
#include <string>

class CsvReader;
class NodeRef;
class NodeService;

class ExportDataReader {
 public:
  ExportDataReader(NodeService& node_service, CsvReader& reader);

  ExportData Read();

 private:
  ExportData::Property ReadProperty(std::wstring_view cell);

  ExportData::Node ReadNode(const std::vector<ExportData::Property>& props);

  std::optional<ExportData::PropertyValue> ReadPropertyValue(
      const ExportData::Property& prop,
      const NodeRef& type_definition);

  std::wstring& ReadCell();

  std::wstring* TryReadCell();

  NodeService& node_service_;
  CsvReader& reader_;

  std::wstring cell_;
};
