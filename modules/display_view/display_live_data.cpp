#include "display_view/display_live_data.h"

#include "timed_data/timed_data_spec.h"

#include "base/utf_convert.h"

#include <algorithm>
#include <exception>

namespace {

// OPC UA StatusCode severity bits 30-31: 0b10 is Bad. Only the severity is
// read on the other side of the display ABI.
// OPC UA Part 4 §7.38.1 General,
// https://reference.opcfoundation.org/Core/Part4/v105/docs/7.38.1
constexpr int32_t kBadQuality = static_cast<int32_t>(0x80000000u);

// The text of a reading with no value. A value field must never fall back to
// whatever it showed before — the designer's saved number, or the last live
// one — since either reads as a good measurement (backlog 1005). It is the
// mark `FormatValue` appends to a bad value, so the field reads as the
// value's table cell does. Sent by the host as well as applied by the
// runtime, so a runtime that predates the quality-marking contract still
// shows it in a field that displays the host's text.
constexpr char kNoValueText[] = "?";

}  // namespace

// One subscription. Heap-allocated so the spec's handlers, which capture it,
// never see it move.
struct DisplayLiveData::Binding {
  std::string data_source;
  TimedDataSpec spec;
};

DisplayLiveData::DisplayLiveData(TimedDataService& timed_data_service,
                                 std::span<const std::string> data_sources,
                                 ReadingCallback callback)
    : callback_{std::move(callback)} {
  for (const std::string& data_source : data_sources) {
    auto binding = std::make_unique<Binding>();
    binding->data_source = data_source;

    Binding* raw = binding.get();
    const auto report = [this, raw] { Report(*raw); };
    raw->spec.property_change_handler = [report](const PropertySet&) {
      report();
    };
    raw->spec.node_modified_handler = report;
    raw->spec.deletion_handler = report;
    // An alarm raised or acknowledged changes no value, so it arrives here.
    raw->spec.event_change_handler = report;

    // A formula the service cannot use reports as not connected rather than
    // stopping the rest of the schematic from binding.
    try {
      raw->spec.Connect(timed_data_service, data_source);
    } catch (const std::exception&) {
    }

    bindings_.push_back(std::move(binding));
    Report(*raw);
  }
}

DisplayLiveData::~DisplayLiveData() = default;

std::optional<std::string> DisplayLiveData::FindDataSourceForNode(
    const scada::NodeId& node_id) const {
  if (node_id.is_null())
    return std::nullopt;
  for (const std::unique_ptr<Binding>& binding : bindings_) {
    if (binding->spec.connected() && binding->spec.node_id() == node_id)
      return binding->data_source;
  }
  return std::nullopt;
}

const DisplayLiveData::Binding* DisplayLiveData::FindBinding(
    std::string_view data_source) const {
  for (const std::unique_ptr<Binding>& binding : bindings_) {
    if (binding->data_source == data_source)
      return binding.get();
  }
  return nullptr;
}

bool DisplayLiveData::IsAlerting(std::string_view data_source) const {
  const Binding* binding = FindBinding(data_source);
  return binding && binding->spec.connected() && binding->spec.alerting();
}

bool DisplayLiveData::Acknowledge(std::string_view data_source) {
  if (!IsAlerting(data_source))
    return false;
  FindBinding(data_source)->spec.Acknowledge();
  return true;
}

std::vector<std::string> DisplayLiveData::AlertingDataSources() const {
  std::vector<std::string> result;
  for (const std::unique_ptr<Binding>& binding : bindings_) {
    if (binding->spec.connected() && binding->spec.alerting())
      result.push_back(binding->data_source);
  }
  return result;
}

std::vector<scada::NodeId> DisplayLiveData::ResolvedNodes() const {
  std::vector<scada::NodeId> nodes;
  for (const std::unique_ptr<Binding>& binding : bindings_) {
    if (!binding->spec.connected())
      continue;
    scada::NodeId node_id = binding->spec.node_id();
    if (!node_id.is_null() && std::ranges::find(nodes, node_id) == nodes.end())
      nodes.push_back(std::move(node_id));
  }
  return nodes;
}

// static
DisplayDataSourceReading DisplayLiveData::MakeReading(
    std::string_view data_source,
    const TimedDataSpec& spec) {
  DisplayDataSourceReading reading{.data_source = std::string{data_source}};
  if (!spec.connected()) {
    reading.quality = kBadQuality;
    reading.text = kNoValueText;
    return reading;
  }

  reading.alerting = spec.alerting();
  const scada::DataValue current = spec.current();
  if (current.qualifier.general_bad())
    reading.quality = kBadQuality;
  if (current.value.is_null()) {
    reading.text = kNoValueText;
    return reading;
  }

  // 2.5.7 read every value as a double and drew a position for anything
  // non-zero (`ModusElement::UpdateData`).
  reading.value = current.value.get_or(0.0);
  reading.state = *reading.value != 0.0 ? "closed" : "open";
  reading.text = UtfConvert<char>(spec.GetCurrentString());
  return reading;
}

void DisplayLiveData::Report(const Binding& binding) const {
  if (callback_)
    callback_(MakeReading(binding.data_source, binding.spec));
}
