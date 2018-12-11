#pragma once

#include <memory>

#include "controller.h"
#include "ui/base/models/table_model_observer.h"

class Table;
class WatchModel;

class WatchView : public Controller,
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
  virtual void Print(PrintService& print_service) override;

 private:
  base::string16 MakeTitle() const;

  void SaveLog();

  // ui::TableModelObserver
  virtual void OnItemsAdded(int first, int count) override;

  std::unique_ptr<WatchModel> model_;

  bool auto_scroll_ = false;

  std::unique_ptr<Table> table_;
};
