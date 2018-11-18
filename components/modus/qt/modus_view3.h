#pragma once

#include "base/files/file_path.h"
#include "components/modus/modus_view_wrapper.h"
#include "timed_data/timed_data_spec.h"

#include <functional>
#include <map>
#include <memory>
#include <schematic/document.h>
#include <schematic/view.h>

class TimedDataService;

class ModusView3 : public Schematic::View,
                   public ModusViewWrapper {
 public:
  explicit ModusView3(TimedDataService& timed_data_service);
  virtual ~ModusView3();

  // ModusViewWrapper
  virtual void Open(const base::FilePath& path) override;
  virtual base::FilePath GetPath() const override;
  virtual bool ShowContainedItem(const scada::NodeId& item_id) override;
  virtual htsde2::IHTSDEForm2* GetSdeForm() override;

 private:
  TimedDataService& timed_data_service_;

  base::FilePath path_;
  base::string16 title_;

  Schematic::Document document_;
};
