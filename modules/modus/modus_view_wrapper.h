#pragma once

#include "scada/node_id.h"

#include <cstdint>
#include <filesystem>

class WindowDefinition;

class ModusViewWrapper {
 public:
  virtual ~ModusViewWrapper() = default;

  // Opens the document named by `definition`. `document_kind` is a
  // `TcVdsRuntimeDocumentKind` naming which Modus renderer the document is to
  // be read with; the controller derives it from the window definition and the
  // profile (see `IsModus2`) rather than letting the runtime guess from the
  // file extension, so that the operator's «Use Modus runtime renderer» choice
  // reaches the renderer. Kept as `int32_t` so this header does not drag the
  // VDS runtime C API into everything that drives a Modus view.
  virtual void Open(const WindowDefinition& definition,
                    int32_t document_kind) = 0;

  virtual void Save(WindowDefinition& definition) = 0;

  virtual std::filesystem::path GetPath() const = 0;

  virtual bool ShowContainedItem(const scada::NodeId& item_id) = 0;
};
