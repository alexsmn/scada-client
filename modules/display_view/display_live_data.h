#pragma once

#include "scada/node_id.h"

#include <cstdint>
#include <functional>
#include <memory>
#include <optional>
#include <span>
#include <string>
#include <string_view>
#include <vector>

class TimedDataService;
class TimedDataSpec;

// One data source's latest reading, in the terms the display runtime takes
// (backlog 915): a position keyword for `положение`-bound Modus elements, the
// number a Vidicon shape's formulas compute from, the formatted value for
// elements that show text, an OPC UA StatusCode whose severity bits say
// whether the reading is good, and whether its object has an alarm nobody has
// acknowledged.
struct DisplayDataSourceReading {
  std::string data_source;
  // "closed" / "open", or empty when there is no value to read one from.
  std::string state;
  // The value as a number, or nullopt when there is none.
  std::optional<double> value;
  // The value as tables show it, units and quality mark included; "?" when
  // there is no value, so a value field never keeps a number nothing reports.
  std::string text;
  int32_t quality = 0;
  // The object has an unacknowledged alarm — 2.5.7's «НеКвитировано» style.
  bool alerting = false;
};

// Keeps a schematic's bound elements live (backlog 915).
//
// A schematic names what it shows — a Modus element through its
// `ключ_привязки`, a Vidicon shape through the address in its `Tag` rows; the
// display runtime enumerates those formulas, and this subscribes to every one
// through the timed-data service — so an alias, a node id, a `{…}` channel
// path and an expression over them all resolve exactly as they do in a
// table — and reports each reading as it changes, starting with the one it
// has on connecting.
//
// This is what 2.5.7's ActiveX `ModusElement::UpdateData` did, minus the
// drawing: a discrete value is a position (non-zero closed, zero open), any
// value is also formatted as text with its units and quality mark, a reading
// that is not connected or not good is reported as bad, and an object with
// unacknowledged events is reported as alerting.
class DisplayLiveData {
 public:
  using ReadingCallback = std::function<void(const DisplayDataSourceReading&)>;

  // Subscribes to `data_sources`. `callback` runs once per source before the
  // constructor returns, and again on every change.
  DisplayLiveData(TimedDataService& timed_data_service,
                  std::span<const std::string> data_sources,
                  ReadingCallback callback);
  ~DisplayLiveData();

  DisplayLiveData(const DisplayLiveData&) = delete;
  DisplayLiveData& operator=(const DisplayLiveData&) = delete;

  // The data source whose subscription resolved to `node_id`, or nullopt —
  // what «Мнемосхема» for an object highlights (backlog 910).
  std::optional<std::string> FindDataSourceForNode(
      const scada::NodeId& node_id) const;

  // The objects the subscriptions have resolved to so far, once each — what
  // the file cache indexes the schematic under, so «Мнемосхема» for an object
  // can find it (backlog 910).
  std::vector<scada::NodeId> ResolvedNodes() const;

  // Whether the object `data_source` resolved to has an unacknowledged alarm.
  bool IsAlerting(std::string_view data_source) const;

  // Acknowledges the alarms of the object `data_source` resolved to, and
  // returns false when it had none — 2.5.7's double-click, which followed the
  // element's hyperlink only when there was nothing to acknowledge.
  bool Acknowledge(std::string_view data_source);

  // The data sources whose objects currently have an unacknowledged alarm.
  std::vector<std::string> AlertingDataSources() const;

  // The latest reading for one data source, as last reported.
  static DisplayDataSourceReading MakeReading(std::string_view data_source,
                                              const TimedDataSpec& spec);

 private:
  struct Binding;

  const Binding* FindBinding(std::string_view data_source) const;
  void Report(const Binding& binding) const;

  ReadingCallback callback_;
  std::vector<std::unique_ptr<Binding>> bindings_;
};
