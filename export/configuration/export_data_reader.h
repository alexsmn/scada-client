#pragma once

#include "export/configuration/export_data.h"

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
  ExportData::Property ReadProperty(std::u16string_view cell);

  ExportData::Node ReadNode(const std::vector<ExportData::Property>& props);

  std::optional<ExportData::PropertyValue> ReadPropertyValue(
      const ExportData::Property& prop,
      const NodeRef& type_definition);

  std::u16string& ReadCell();

  std::u16string* TryReadCell();

  NodeService& node_service_;
  CsvReader& reader_;

  std::u16string cell_;
};
