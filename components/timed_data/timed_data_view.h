#pragma once

#include "contents_model.h"
#include "controller.h"

#include <memory>

class Table;
class TimedDataModel;

class TimedDataView : public Controller,
                      public ContentsModel
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
  virtual TimeModel* GetTimeModel() override;

  // ContentsModel
  virtual void AddContainedItem(const scada::NodeId& node_id,
                                unsigned flags) override;

 private:
  base::string16 MakeTitle() const;
  void UpdateColumnTitles();

  void ExportToExcel();

  std::unique_ptr<TimedDataModel> model_;

  std::unique_ptr<Table> view_;
};
