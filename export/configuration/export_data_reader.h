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
  ExportData::Node ReadNode(const std::vector<ExportData::Property>& props);

  std::optional<ExportData::PropertyValue> ReadProperty(
      const scada::NodeId& prop_decl_id);

  ExportData::Property ParseProperty(std::u16string_view cell) const;

  std::optional<ExportData::PropertyValue> ParsePropertyValue(
      const NodeRef& prop_decl,
      std::u16string_view string_value) const;

  std::optional<ExportData::PropertyValue> ParseReferenceValue(
      const NodeRef& ref_type,
      std::u16string_view string_value) const;

  std::u16string& ReadCell();

  std::u16string* TryReadCell();

  void SkipCell() { ReadCell(); }

  // The node service is used to read the type system.
  NodeService& node_service_;
  CsvReader& reader_;

  std::u16string cell_;
};
