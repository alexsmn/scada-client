#pragma once

#include <memory>

#include "controller.h"
#include "ui/base/models/table_model_observer.h"

#if defined(UI_VIEWS)
#include "ui/views/controls/table/table_controller.h"
#endif

class Table;
class WatchModel;

class WatchView : public Controller,
#if defined(UI_VIEWS)
                  private views::TableController,
#endif
                  private ui::TableModelObserver {
 public:
  explicit WatchView(const ControllerContext& context);
  virtual ~WatchView();

  // Controller
  virtual UiView* Init(const WindowDefinition& definition) override;
  virtual void Save(WindowDefinition& definition) override;
  virtual CommandHandler* GetCommandHandler(unsigned command_id) override;
  virtual bool IsCommandChecked(unsigned command) const override;
  virtual void ExecuteCommand(unsigned command) override;

 private:
  base::string16 MakeTitle() const;

  void SaveLog();

  // ui::TableModelObserver
  virtual void OnItemsAdded(int first, int count) override;

#if defined(UI_VIEWS)
  // GridController
  virtual void OnSelectionChanged(views::TableView& sender) override;
#endif

  std::unique_ptr<WatchModel> model_;

  bool auto_scroll_ = false;

  std::unique_ptr<Table> table_;
};
