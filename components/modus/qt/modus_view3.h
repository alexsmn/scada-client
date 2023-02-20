#pragma once

#include "base/files/file_path.h"
#include "components/modus/modus_view_wrapper.h"
#include "modus_binding3.h"

#include <schematic/document.h>
#include <schematic/view.h>
#include <QGraphicsScene>
#include <QGraphicsView>
#include <functional>
#include <map>
#include <memory>

class TimedDataService;

// Use schematic library.
class ModusView3 : public QGraphicsView,
                   public ModusViewWrapper {
 public:
  explicit ModusView3(TimedDataService& timed_data_service);
  virtual ~ModusView3();

  // ModusViewWrapper
  virtual void Open(const std::filesystem::path& path) override;
  virtual std::filesystem::path GetPath() const override;
  virtual bool ShowContainedItem(const scada::NodeId& item_id) override;

 private:
  void CreateBindings(Schematic::Element& element);

  TimedDataService& timed_data_service_;

  std::filesystem::path path_;
  std::wstring title_;

  Schematic::Document document_;

  QGraphicsScene scene_;
  Schematic::View view_{scene_};

  std::vector<ModusBinding3> bindings_;
};
