#pragma once

#include "scada/node_id.h"

#include <cstdint>
#include <filesystem>
#include <string>
#include <string_view>
#include <vector>

class WindowDefinition;

// One data source's latest reading, in the terms the display runtime takes
// (backlog 915): a position keyword for `положение`-bound elements, the
// formatted value for `текст`-bound ones, and an OPC UA StatusCode whose
// severity bits say whether the reading is good.
struct ModusDataSourceReading {
  std::string data_source;
  // "closed" / "open", or empty when there is no value to read one from.
  std::string state;
  // The value as tables show it, units and quality mark included; empty when
  // there is no value.
  std::string text;
  int32_t quality = 0;
};

class ModusViewWrapper {
 public:
  virtual ~ModusViewWrapper() = default;

  // Opens the document named by `definition`.
  //
  // It took a `DocumentKind` alongside, derived from the window definition and
  // a profile flag, until backlog 491 -- which the reader then discarded,
  // because SDE and XSDE are unrelated encodings and only the extension can
  // choose between them. The reader makes that choice itself now.
  virtual void Open(const WindowDefinition& definition) = 0;

  virtual void Save(WindowDefinition& definition) = 0;

  virtual std::filesystem::path GetPath() const = 0;

  virtual bool ShowContainedItem(const scada::NodeId& item_id) = 0;

  // The open document's live bindings (backlog 915). Defaulted so a renderer
  // that draws its own values — the ActiveX one — need not implement them.

  // The open document's own title, or empty when it has none.
  virtual std::u16string GetTitle() const { return {}; }

  // The data sources the open document's elements are bound to, once each.
  virtual std::vector<std::string> GetDataSources() const { return {}; }

  // Shows one data source's latest reading on every element bound to it.
  virtual void SetDataSourceReading(const ModusDataSourceReading& reading) {}

  // Selects the first element bound to `data_source`, as a click on it would,
  // and returns false when no element is (backlog 910).
  virtual bool SelectDataSourceElement(std::string_view data_source) {
    return false;
  }
};
