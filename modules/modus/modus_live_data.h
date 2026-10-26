#pragma once

#include "modus/modus_view_wrapper.h"
#include "scada/node_id.h"

#include <functional>
#include <memory>
#include <optional>
#include <span>
#include <string>
#include <string_view>
#include <vector>

class TimedDataService;
class TimedDataSpec;

// Keeps a Modus schematic's bound elements live (backlog 915).
//
// A schematic names what it shows through each element's `ключ_привязки`; the
// display runtime enumerates those formulas, and this subscribes to every one
// through the timed-data service — so an alias, a node id and an expression
// over them all resolve exactly as they do in a table — and reports each
// reading as it changes, starting with the one it has on connecting.
//
// This is what 2.5.7's ActiveX `ModusElement::UpdateData` did, minus the
// drawing: a discrete value is a position (non-zero closed, zero open), any
// value is also formatted as text with its units and quality mark, and a
// reading that is not connected or not good is reported as bad.
class ModusLiveData {
 public:
  using ReadingCallback = std::function<void(const ModusDataSourceReading&)>;

  // Subscribes to `data_sources`. `callback` runs once per source before the
  // constructor returns, and again on every change.
  ModusLiveData(TimedDataService& timed_data_service,
                std::span<const std::string> data_sources,
                ReadingCallback callback);
  ~ModusLiveData();

  ModusLiveData(const ModusLiveData&) = delete;
  ModusLiveData& operator=(const ModusLiveData&) = delete;

  // The data source whose subscription resolved to `node_id`, or nullopt —
  // what «Мнемосхема» for an object highlights (backlog 910).
  std::optional<std::string> FindDataSourceForNode(
      const scada::NodeId& node_id) const;

  // The objects the subscriptions have resolved to so far, once each — what
  // the file cache indexes the schematic under, so «Мнемосхема» for an object
  // can find it (backlog 910).
  std::vector<scada::NodeId> ResolvedNodes() const;

  // The latest reading for one data source, as last reported.
  static ModusDataSourceReading MakeReading(std::string_view data_source,
                                            const TimedDataSpec& spec);

 private:
  struct Binding;

  void Report(const Binding& binding) const;

  ReadingCallback callback_;
  std::vector<std::unique_ptr<Binding>> bindings_;
};
