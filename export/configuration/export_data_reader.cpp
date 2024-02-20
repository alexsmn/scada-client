#include "export/configuration/export_data_reader.h"

#include "aui/resource_error.h"
#include "base/csv_reader.h"
#include "base/string_piece_util.h"
#include "base/strings/stringprintf.h"
#include "base/strings/utf_string_conversions.h"
#include "common/format.h"
#include "model/node_id_util.h"
#include "node_service/node_service.h"
#include "node_service/node_util.h"

namespace {

scada::NodeId ParseReferenceCell(std::u16string_view s) {
  auto p = s.rfind(L'@');
  if (p == s.npos)
    return scada::NodeId();
  auto n = s.substr(p + 1);
  return NodeIdFromScadaString(base::UTF16ToUTF8(AsStringPiece(n)));
}

scada::NodeId GetBuiltInDataTypeId(const NodeRef& data_type) {
  for (int i = scada::Variant::EMPTY + 1; i < scada::Variant::COUNT; ++i) {
    auto built_in_data_type_id =
        scada::ToNodeId(static_cast<scada::Variant::Type>(i));
    assert(!built_in_data_type_id.is_null());
    if (IsSubtypeOf(data_type, built_in_data_type_id))
      return built_in_data_type_id;
  }
  return data_type.node_id();
}

}  // namespace

// ExportDataReader

ExportDataReader::ExportDataReader(NodeService& node_service, CsvReader& reader)
    : node_service_{node_service}, reader_{reader} {}

ExportData ExportDataReader::Read() {
  if (!reader_.NextRow())
    throw ResourceError{u"Нет строки заголовка"};

  // Skip Id, Parent, Type, Name.
  for (int i = 0; i < 4; ++i)
    ReadCell();

  std::vector<ExportData::Property> props;
  while (auto cell = TryReadCell())
    props.emplace_back(ReadProperty(*cell));

  std::vector<ExportData::Node> nodes;
  while (reader_.NextRow())
    nodes.push_back(ReadNode(props));

  return {std::move(props), std::move(nodes)};
}

ExportData::Property ExportDataReader::ReadProperty(std::u16string_view cell) {
  auto prop_type_id = ParseReferenceCell(cell);
  if (prop_type_id.is_null())
    throw ResourceError{u"Неверный формат имени столбца"};

  auto prop_decl = node_service_.GetNode(prop_type_id);
  bool reference = prop_decl.node_class() == scada::NodeClass::ReferenceType;

  // TODO: Read display name.
  return {std::move(prop_type_id), {}, reference};
}

ExportData::Node ExportDataReader::ReadNode(
    const std::vector<ExportData::Property>& props) {
  // Id.
  auto node_id = NodeIdFromScadaString(base::UTF16ToUTF8(ReadCell()));

  // Parent.
  auto parent_id = NodeIdFromScadaString(base::UTF16ToUTF8(ReadCell()));
  if (parent_id.is_null())
    throw ResourceError{u"Группа не найдена"};

  // Type.
  auto type_definition = node_service_.GetNode(ParseReferenceCell(ReadCell()));
  if (!type_definition)
    throw ResourceError{u"Тип не найден"};

  scada::LocalizedText display_name = ReadCell();

  // Props & refs.
  std::vector<ExportData::PropertyValue> prop_values;
  for (const auto& prop : props) {
    if (auto prop_value = ReadPropertyValue(prop, type_definition))
      prop_values.emplace_back(std::move(*prop_value));
  }

  // TODO: Read display name.
  return {
      std::move(node_id),
      std::move(parent_id),
      {},
      type_definition.node_id(),
      std::move(display_name),
      std::move(prop_values),
  };
}

std::optional<ExportData::PropertyValue> ExportDataReader::ReadPropertyValue(
    const ExportData::Property& prop,
    const NodeRef& type_definition) {
  auto string_value = ReadCell();

  if (prop.reference) {
    if (type_definition.target(prop.node_id)) {
      auto target_id = ParseReferenceCell(string_value);
      // TODO: Read display name.
      return ExportData::PropertyValue{
          prop.node_id, {}, std::move(target_id), {}, true};
    }

  } else {
    auto property_declaration = type_definition[prop.node_id];
    if (!property_declaration) {
      throw ResourceError{base::StringPrintf(
          u"Свойство %ls не найдено",
          base::ASCIIToUTF16(NodeIdToScadaString(prop.node_id)).c_str())};
    }

    scada::Variant new_value;
    auto built_in_data_type_id =
        GetBuiltInDataTypeId(property_declaration.data_type());
    if (!StringToValue(string_value, built_in_data_type_id, new_value)) {
      auto data_type = node_service_.GetNode(built_in_data_type_id);
      auto data_type_name =
          data_type ? ToString16(data_type.display_name()) : u"(Неизвестный)";
      throw ResourceError{
          base::StringPrintf(u"Невозможно распознать значение '%ls' как '%ls'",
                             string_value.c_str(), data_type_name.c_str())};
    }

    return ExportData::PropertyValue{prop.node_id, std::move(new_value)};
  }

  return std::nullopt;
}

std::u16string& ExportDataReader::ReadCell() {
  auto* cell = TryReadCell();
  if (!cell)
    throw ResourceError(u"Количество ячеек в строке меньше ожидаемого");
  return *cell;
}

std::u16string* ExportDataReader::TryReadCell() {
  if (!reader_.NextCell(cell_))
    return nullptr;
  return &cell_;
}
