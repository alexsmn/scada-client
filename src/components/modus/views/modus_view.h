#pragma once

#include "base/files/file_util.h"
#include "base/win/scoped_comptr.h"
#include "core/configuration_types.h"
#include "client/components/modus/modus.h"
#include "client/components/modus/modus_view_wrapper.h"
#include "ui/views/controls/activex_control.h"

#include <atlbase.h>
#include <atlcom.h>
#include <functional>
#include <map>

class FileCache;
class ModusController;
class NodeRefService;
class TimedDataService;

namespace base {
class FilePath;
}

namespace rt {
class TimedDataSpec;
}

namespace modus {
class ModusLoader;
class ModusObject;
}

struct ModusViewContext {
  NodeRefService& node_service_;
  TimedDataService& timed_data_service_;
  FileCache& file_cache_;
};

class ModusView : public views::ActiveXControl,
                  public ModusViewWrapper,
                  public ATL::IDispEventImpl<1, ModusView,
                                             &__uuidof(htsde2::IHTSDEForm2Events),
                                             &__uuidof(htsde2::__htsde2), 0xFFFF, 0xFFFF>,
                  private views::ActiveXControl::Controller,
                  private ModusViewContext {
 public:
  explicit ModusView(ModusViewContext&& context);
  virtual ~ModusView();

  // Find entity by TRID. Method has linear complexity.
  modus::ModusObject* FindObject(scada::NodeId trid);

  BEGIN_SINK_MAP(ModusView)
    SINK_ENTRY_EX(1, __uuidof(htsde2::IHTSDEForm2Events), 0x0000000b, OnDocPopup)
    SINK_ENTRY_EX(1, __uuidof(htsde2::IHTSDEForm2Events), 0x0000001a, OnDocClick)		// click
    SINK_ENTRY_EX(1, __uuidof(htsde2::IHTSDEForm2Events), 0x00000012, OnDocDblClick)	// double-click
    SINK_ENTRY_EX(1, __uuidof(htsde2::IHTSDEForm2Events), 0x00000019, OnDocClick)		// right-click
  END_SINK_MAP()

  STDMETHOD_(void, OnDocPopup)(ISDEDocument50* doc, VARIANT_BOOL* popup);
  STDMETHOD_(void, OnDocClick)(ISDEDocument50* doc, SDECore::IUIEventInfo* info);
  STDMETHOD_(void, OnDocDblClick)(ISDEDocument50* doc, SDECore::IUIEventInfo* info);

  // ModusViewWrapper
  virtual void Open(const base::FilePath& path) override;
  virtual base::FilePath GetPath() const override;
  virtual bool ShowContainedItem(const scada::NodeId& item_id) override;

 protected:
  friend class ModusController;
  friend class modus::ModusLoader;

  void OpenInternal(const base::FilePath& display);
  void DeleteObjects();

  // views::ActiveXControl::Controller
  virtual void OnControlCreated(views::ActiveXControl& sender) override;
  virtual void OnContractDestroyed(views::ActiveXControl& sender) override;

  scada::NodeId selected_item_;
  base::FilePath path_;

  typedef std::list<modus::ModusObject*> Objects;
  Objects objects_;

  typedef std::map<long, modus::ModusObject*> ObjectMap;
  ObjectMap object_map_;

  base::win::ScopedComPtr<htsde2::IHTSDEForm2> sde_form_;
  base::win::ScopedComPtr<SDECore::ISDEDocument50> sde_document_;

  base::string16 title_;

  std::function<void(const base::string16& title)> title_callback_;
  std::function<void(const base::FilePath& path)> navigation_callback_;
  std::function<void(const rt::TimedDataSpec& selection)> selection_callback_;
  std::function<void(const gfx::Point& point)> popup_menu_callback_;
};