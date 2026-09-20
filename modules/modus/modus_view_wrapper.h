#pragma once

#include "scada/node_id.h"

#include <filesystem>

class WindowDefinition;

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
};
