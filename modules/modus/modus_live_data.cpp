#include "modus/modus_live_data.h"

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

}  // namespace

// One subscription. Heap-allocated so the spec's handlers, which capture it,
// never see it move.
struct ModusLiveData::Binding {
  std::string data_source;
  TimedDataSpec spec;
};

ModusLiveData::ModusLiveData(TimedDataService& timed_data_service,
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

ModusLiveData::~ModusLiveData() = default;

std::optional<std::string> ModusLiveData::FindDataSourceForNode(
    const scada::NodeId& node_id) const {
  if (node_id.is_null())
    return std::nullopt;
  for (const std::unique_ptr<Binding>& binding : bindings_) {
    if (binding->spec.connected() && binding->spec.node_id() == node_id)
      return binding->data_source;
  }
  return std::nullopt;
}

std::vector<scada::NodeId> ModusLiveData::ResolvedNodes() const {
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
ModusDataSourceReading ModusLiveData::MakeReading(std::string_view data_source,
                                                  const TimedDataSpec& spec) {
  ModusDataSourceReading reading{.data_source = std::string{data_source}};
  if (!spec.connected()) {
    reading.quality = kBadQuality;
    return reading;
  }

  const scada::DataValue current = spec.current();
  if (current.qualifier.general_bad())
    reading.quality = kBadQuality;
  if (current.value.is_null())
    return reading;

  // 2.5.7 read every value as a double and drew a position for anything
  // non-zero (`ModusElement::UpdateData`).
  reading.state = current.value.get_or(0.0) != 0.0 ? "closed" : "open";
  reading.text = UtfConvert<char>(spec.GetCurrentString());
  return reading;
}

void ModusLiveData::Report(const Binding& binding) const {
  if (callback_)
    callback_(MakeReading(binding.data_source, binding.spec));
}
