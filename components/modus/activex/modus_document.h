#pragma once

#include "base/files/file_path.h"
#include "common/aliases.h"
#include "components/modus/activex/modus.h"
#include "controls/types.h"

#include <atlbase.h>

#include <atlcom.h>
#include <wrl/client.h>
#include <functional>
#include <map>

class FileCache;
class TimedDataService;
class TimedDataSpec;

namespace modus {

class ModusLoader;
class ModusObject;

struct ModusDocumentContext {
  const AliasResolver alias_resolver_;
  TimedDataService& timed_data_service_;
  FileCache& file_cache_;

  const std::function<void(const base::string16& title)> title_callback_;
  const std::function<void(base::StringPiece16 hyperlink)> navigation_callback_;
  const std::function<void(const TimedDataSpec& selection)> selection_callback_;
  const ContextMenuHandler context_menu_callback_;
};

class ModusDocument
    : protected ModusDocumentContext,
      public ATL::IDispEventImpl<1,
                                 ModusDocument,
                                 &__uuidof(htsde2::IHTSDEForm2Events),
                                 &__uuidof(htsde2::__htsde2),
                                 0xFFFF,
                                 0xFFFF> {
 public:
  ModusDocument(ModusDocumentContext&& context,
                htsde2::IHTSDEForm2& sde_form,
                const base::FilePath& path);
  ~ModusDocument();

  htsde2::IHTSDEForm2& sde_form() { return *sde_form_.Get(); }
  const base::string16& title() const { return title_; }

  // Find entity by TRID. Method has linear complexity.
  ModusObject* FindObject(const scada::NodeId& node_id);

  bool ShowContainedItem(const scada::NodeId& item_id);

  BEGIN_SINK_MAP(ModusDocument)
  SINK_ENTRY_EX(1, __uuidof(htsde2::IHTSDEForm2Events), 0x0000000b, OnDocPopup)
  SINK_ENTRY_EX(1,
                __uuidof(htsde2::IHTSDEForm2Events),
                0x0000001a,
                OnDocClick)  // click
  SINK_ENTRY_EX(1,
                __uuidof(htsde2::IHTSDEForm2Events),
                0x00000012,
                OnDocDblClick)  // double-click
  SINK_ENTRY_EX(1,
                __uuidof(htsde2::IHTSDEForm2Events),
                0x00000019,
                OnDocRightClick)  // right-click
  END_SINK_MAP()

  STDMETHOD_(void, OnDocPopup)(ISDEDocument50* doc, VARIANT_BOOL* popup);
  STDMETHOD_(void, OnDocClick)
  (ISDEDocument50* doc, SDECore::IUIEventInfo* info);
  STDMETHOD_(void, OnDocRightClick)
  (ISDEDocument50* doc, SDECore::IUIEventInfo* info);
  STDMETHOD_(void, OnDocDblClick)
  (ISDEDocument50* doc, SDECore::IUIEventInfo* info);

 protected:
  enum class MouseButton { Left, Right };

  void HandleClick(MouseButton button, SDECore::IUIEventInfo* info);

  Microsoft::WRL::ComPtr<htsde2::IHTSDEForm2> sde_form_;
  Microsoft::WRL::ComPtr<SDECore::ISDEDocument50> sde_document_;

  std::vector<std::unique_ptr<modus::ModusObject>> objects_;

  using ObjectId = long;
  std::map<ObjectId, modus::ModusObject*> object_map_;

  base::string16 title_;

  friend class modus::ModusLoader;
};

}  // namespace modus
