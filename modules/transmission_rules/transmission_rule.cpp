#include "transmission_rules/transmission_rule.h"

#include "model/data_items_node_ids.h"
#include "model/devices_node_ids.h"
#include "scada/node_id.h"

#include <string>

std::u16string TransmissionProtocolLabel(
    const scada::NodeId& type_definition_id) {
  if (type_definition_id == scada::devices::id::ModbusTransmissionItemType)
    return u"Modbus";
  if (type_definition_id == scada::devices::id::Iec60870TransmissionItemType)
    return u"IEC 60870-5-104";
  if (type_definition_id == scada::devices::id::Iec61850TransmissionItemType)
    return u"IEC 61850";
  return {};
}

std::u16string TransmissionSignalTag(const scada::NodeId& source_type_id) {
  if (source_type_id == scada::data_items::id::DiscreteItemType)
    return u"TS";
  if (source_type_id == scada::data_items::id::AnalogItemType)
    return u"TI";
  return {};
}

std::u16string TransmissionRuleSummary(const std::u16string& source_name,
                                       scada::Int32 ioa) {
  const std::u16string name = source_name.empty() ? u"—" : source_name;
  // The address is ASCII digits; widen them to char16_t. Build the string once
  // (begin()/end() must iterate the same object).
  const std::string digits = std::to_string(ioa);
  // u"→" is the rightwards arrow that separates source from address, as in
  // the mockup's "Ua → 2001".
  return name + u" → " + std::u16string(digits.begin(), digits.end());
}
