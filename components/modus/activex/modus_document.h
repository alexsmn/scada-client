#pragma once

#include "aui/handlers.h"
#include "common/aliases.h"

#include <string_view>
#include <unordered_map>
#include <wrl/client.h>

class FileCache;
class TimedDataService;
class TimedDataSpec;

namespace SDECore {
struct ISDEDocument50;
struct IUIEventInfo;
}  // namespace SDECore

namespace std::filesystem {
class path;
}

namespace htsde2 {
struct IHTSDEForm2;
}

namespace modus {

class ModusObject;

struct ModusDocumentContext {
  const AliasResolver alias_resolver_;
  TimedDataService& timed_data_service_;
  FileCache& file_cache_;

  const std::function<void(const std::u16string& title)> title_callback_;
  const std::function<void(std::u16string_view hyperlink)> navigation_callback_;
  const std::function<void(const TimedDataSpec& selection)> selection_callback_;
  const ContextMenuHandler context_menu_callback_;
  const std::function<void()> enable_internal_render_callback_;
};

class ModusDocument : private ModusDocumentContext {
 public:
  enum class MouseButton { Left, Right };

  ModusDocument(ModusDocumentContext&& context, htsde2::IHTSDEForm2& sde_form);
  ~ModusDocument();

  htsde2::IHTSDEForm2& sde_form() { return *sde_form_.Get(); }
  SDECore::ISDEDocument50* sde_document() { return sde_document_.Get(); }

  const std::u16string& title() const { return title_; }

  void InitFromFilePath(const std::filesystem::path& path);
  void InitFromState(std::string_view state);

  // Find entity by TRID. Method has linear complexity.
  ModusObject* FindObject(const scada::NodeId& node_id);

  bool ShowContainedItem(const scada::NodeId& node_id);

  std::string SaveState() const;

  void OnDocPopup(bool& popup);
  void OnDocClick(MouseButton button, SDECore::IUIEventInfo& ui_event_info);
  void OnDocDblClick(SDECore::IUIEventInfo& ui_event_info);

 private:
  class EventSink;

  void PostInit();

  void CreateEventSink();

  Microsoft::WRL::ComPtr<EventSink> event_sink_;

  Microsoft::WRL::ComPtr<htsde2::IHTSDEForm2> sde_form_;
  Microsoft::WRL::ComPtr<SDECore::ISDEDocument50> sde_document_;

  std::vector<std::unique_ptr<modus::ModusObject>> objects_;

  using ObjectId = long;
  std::unordered_map<ObjectId, modus::ModusObject*> object_map_;

  std::u16string title_;
};

}  // namespace modus
