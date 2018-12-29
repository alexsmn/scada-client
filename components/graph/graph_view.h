#pragma once

#include "components/graph/metrix_graph.h"
#include "contents_model.h"
#include "controller.h"
#include "controls/types.h"
#include "time_model.h"

#if defined(UI_VIEWS)
#include "ui/views/context_menu_controller.h"
#endif

struct TimeRange;

class GraphView : public Controller,
                  public ContentsModel,
                  public TimeModel,
                  private views::Graph::Controller {
 public:
  explicit GraphView(const ControllerContext& context);

  // Controller methods
  virtual bool IsWorking() const override;
  virtual bool CanClose() const override;
  virtual UiView* Init(const WindowDefinition& definition) override;
  virtual void Save(WindowDefinition& definition) override;
  virtual CommandHandler* GetCommandHandler(unsigned command_id) override;
  virtual bool IsCommandChecked(unsigned command_id) const override;
  virtual void ExecuteCommand(unsigned command_id) override;
  virtual ContentsModel* GetContentsModel() override { return this; }
  virtual TimeModel* GetTimeModel() override { return this; }

  // ContentsModel
  virtual void AddContainedItem(const scada::NodeId& node_id,
                                unsigned flags) override;
  virtual void RemoveContainedItem(const scada::NodeId& node_id) override;
  virtual NodeIdSet GetContainedItems() const override;

  // TimeModel
  virtual TimeRange GetTimeRange() const override;
  virtual void SetTimeRange(const TimeRange& time_range) override;
  virtual bool IsTimeRequired() const override { return true; }

 private:
  base::string16 MakeTitle() const;

  void DeleteSelectedPane();

  void ShowSetupDialog();
  void ChooseLineColor();

  void UndoZoom();

  // Delete all pane lines and remove items.
  void ClearPane(MetrixGraph::MetrixPane& pane);

  bool FindColor(SkColor color) const;
  SkColor NewColor() const;

  // views::Graph::Controller
  virtual void OnGraphModified() override;
  virtual void OnGraphSelectPane() override;
  virtual void OnLineItemChanged(views::GraphLine& line) override;

  std::unique_ptr<MetrixGraph> graph_;

  views::GraphRange prezoom_horizontal_range_;
};
