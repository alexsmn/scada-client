#pragma once

#include "display/view/display_document.h"
#include "scada/node_id.h"

#include <filesystem>

class WindowDefinition;

class ModusViewWrapper {
 public:
  virtual ~ModusViewWrapper() = default;

  // Opens the document named by `definition`. `document_kind` names which
  // Modus reader the document is to be read with; the controller derives it
  // from the window definition and the profile (see `IsModus2`) rather than
  // letting the renderer guess from the file extension, so that the operator's
  // «Use Modus runtime renderer» choice reaches the renderer.
  //
  // It was an `int32_t` until ADR 0012 phase 3, so that this header did not
  // drag the plugin's C API into everything driving a Modus view. The renderer
  // is linked now, so the enum travels directly.
  virtual void Open(const WindowDefinition& definition,
                    scada::display::view::DocumentKind document_kind) = 0;

  virtual void Save(WindowDefinition& definition) = 0;

  virtual std::filesystem::path GetPath() const = 0;

  virtual bool ShowContainedItem(const scada::NodeId& item_id) = 0;
};
