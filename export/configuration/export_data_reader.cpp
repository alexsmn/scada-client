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

scada::Variant::Type GetBuiltInDataType(const NodeRef& data_type) {
  if (IsSubtypeOf(data_type, scada::id::Enumeration)) {
    return scada::Variant::INT32;
  }

  for (int i = 0; i < scada::Variant::COUNT; ++i) {
    auto built_in_data_type_id =
        scada::ToNodeId(static_cast<scada::Variant::Type>(i));
    if (IsSubtypeOf(data_type, built_in_data_type_id)) {
      return static_cast<scada::Variant::Type>(i);
    }
  }

  throw ResourceError{u"Неизвестный тип данных"};
}

}  // namespace

// ExportDataReader

ExportDataReader::ExportDataReader(NodeService& node_service, CsvReader& reader)
    : node_service_{node_service}, reader_{reader} {}

ExportData ExportDataReader::Read() {
  if (!reader_.NextRow())
    throw ResourceError{u"Нет строки заголовка"};

  // Skip Id, Parent, Type, Name.
  for (int i = 0; i < 4; ++i) {
    SkipCell();
  }

  std::vector<ExportData::Property> props;
  while (auto cell = TryReadCell()) {
    props.emplace_back(ParseProperty(*cell));
  }

  std::vector<ExportData::Node> nodes;
  while (reader_.NextRow()) {
    nodes.push_back(ReadNode(props));
  }

  return {std::move(props), std::move(nodes)};
}

ExportData::Property ExportDataReader::ParseProperty(
    std::u16string_view cell) const {
  auto prop_decl_id = ParseReferenceCell(cell);
  if (prop_decl_id.is_null()) {
    throw ResourceError{u"Неверный формат имени столбца"};
  }

  auto prop_decl = node_service_.GetNode(prop_decl_id);

  // The type system must be prefeteched before import starts.
  assert(prop_decl.fetched());

  bool reference = prop_decl.node_class() == scada::NodeClass::ReferenceType;

  // TODO: Read display name. It can be used on error reporting.
  return {.prop_decl_id = std::move(prop_decl_id), .reference = reference};
}

ExportData::Node ExportDataReader::ReadNode(
    const std::vector<ExportData::Property>& props) {
  // Id.
  auto node_id = NodeIdFromScadaString(base::UTF16ToUTF8(ReadCell()));

  // Parent.
  auto parent_id = NodeIdFromScadaString(base::UTF16ToUTF8(ReadCell()));
  if (parent_id.is_null()) {
    throw ResourceError{u"Группа не найдена"};
  }

  // Type.
  auto type_definition = node_service_.GetNode(ParseReferenceCell(ReadCell()));
  if (!type_definition) {
    throw ResourceError{u"Тип не найден"};
  }

  scada::LocalizedText display_name = ReadCell();

  // Props & refs.
  std::vector<ExportData::PropertyValue> prop_values;
  for (const auto& prop : props) {
    if (auto prop_value = ReadProperty(prop.prop_decl_id)) {
      prop_values.emplace_back(std::move(*prop_value));
    }
  }

  // TODO: Read display name.
  return ExportData::Node{.node_id = std::move(node_id),
                          .parent_id = std::move(parent_id),
                          .type_id = type_definition.node_id(),
                          .display_name = std::move(display_name),
                          .property_values = std::move(prop_values)};
}

std::optional<ExportData::PropertyValue> ExportDataReader::ReadProperty(
    const scada::NodeId& prop_decl_id) {
  auto prop_decl = node_service_.GetNode(prop_decl_id);
  if (!prop_decl) {
    throw ResourceError{base::StringPrintf(
        u"Свойство %ls не найдено",
        base::ASCIIToUTF16(NodeIdToScadaString(prop_decl_id)).c_str())};
  }

  // The type system must be prefeteched before import starts.
  assert(prop_decl.fetched());

  auto string_value = ReadCell();
  if (string_value.empty()) {
    return std::nullopt;
  }

  return prop_decl.node_class() == scada::NodeClass::ReferenceType
             ? ParseReferenceValue(prop_decl, string_value)
             : ParsePropertyValue(prop_decl, string_value);
}

std::optional<ExportData::PropertyValue> ExportDataReader::ParsePropertyValue(
    const NodeRef& prop_decl,
    std::u16string_view string_value) const {
  assert(prop_decl.fetched());

  scada::Variant new_value;
  auto data_type = GetBuiltInDataType(prop_decl.data_type());
  if (!StringToValue(string_value, data_type, new_value)) {
    throw ResourceError{base::StringPrintf(
        u"Невозможно преобразовать значение '%ls' как тип '%ls'",
        std::u16string{string_value}.c_str(), ToString(data_type).c_str())};
  }

  if (new_value.is_null()) {
    return std::nullopt;
  }

  return ExportData::PropertyValue{prop_decl.node_id(), std::move(new_value)};
}

std::optional<ExportData::PropertyValue> ExportDataReader::ParseReferenceValue(
    const NodeRef& ref_type,
    std::u16string_view string_value) const {
  assert(ref_type.fetched());
  assert(ref_type.node_class() == scada::NodeClass::ReferenceType);

  auto target_id = ParseReferenceCell(string_value);
  if (target_id.is_null()) {
    return std::nullopt;
  }

  // TODO: Read display name.
  return ExportData::PropertyValue{.prop_decl_id = ref_type.node_id(),
                                   .target_id = std::move(target_id),
                                   .reference = true};
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
