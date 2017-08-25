#pragma once

#include <memory>

#include "client/controller.h"
#include "client/contents_model.h"

#if defined(UI_VIEWS)
#include "ui/views/controls/table/table_controller.h"
#endif

class TimedDataModel;

#if defined(UI_QT)
class QTableView;
#endif

class TimedDataView : public Controller,
                      public ContentsModel
#if defined(UI_VIEWS)
                      , private views::TableController
#endif
{
 public:
  explicit TimedDataView(const ControllerContext& context);

  // Controller
  virtual UiView* Init(const WindowDefinition& definition) override;
  virtual void Save(WindowDefinition& definition) override;
  virtual bool IsWorking() const override;
  virtual CommandHandler* GetCommandHandler(unsigned command_id) override;
  virtual void ExecuteCommand(unsigned command) override;
  virtual ContentsModel* GetContentsModel() override { return this; }

  // ContentsModel
  virtual void AddContainedItem(const scada::NodeId& node_id, unsigned flags) override;

 private:
  base::string16 MakeTitle() const;
  void UpdateColumnTitles();

  void ShowSetupDialog();
  void ExportToExcel();
  
#if defined(UI_VIEWS)
  // views::TableController overrides
  virtual void ShowContextMenu(gfx::Point point) override;
#endif

  std::unique_ptr<TimedDataModel> model_;

#if defined(UI_QT)
  std::unique_ptr<QTableView> view_;
#elif defined(UI_VIEWS)
  std::unique_ptr<views::TableView> view_;
#endif
};