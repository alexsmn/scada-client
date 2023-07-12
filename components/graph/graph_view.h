#pragma once

#include "command_registry.h"
#include "components/graph/metrix_graph.h"
#include "contents_model.h"
#include "controller.h"
#include "controller_context.h"
#include "controls/color.h"
#include "controls/types.h"
#include "selection_model.h"
#include "time_model.h"

struct TimeRange;

class GraphView : protected ControllerContext,
                  public Controller,
                  public ContentsModel,
                  public TimeModel,
                  private views::Graph::Controller {
 public:
  explicit GraphView(const ControllerContext& context);

  bool FindColor(aui::Color color) const;
  aui::Color NewColor() const;

  // Controller methods
  virtual bool IsWorking() const override;
  virtual UiView* Init(const WindowDefinition& definition) override;
  virtual void Save(WindowDefinition& definition) override;
  virtual CommandHandler* GetCommandHandler(unsigned command_id) override;
  virtual SelectionModel* GetSelectionModel() override { return &selection_; }
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
  std::u16string MakeTitle() const;

  void DeleteSelectedPane();

  void ChooseLineColor();

  void UndoZoom();

  // Delete all pane lines and remove items.
  void ClearPane(MetrixGraph::MetrixPane& pane);

  void ScrollToNow();

  void ToggleLegend();
  void ToggleLineProperty(unsigned command_id);
  void ToggleZoom();

  // views::Graph::Controller
  virtual void OnGraphModified() override;
  virtual void OnGraphSelectPane() override;
  virtual void OnLineItemChanged(views::GraphLine& line) override;

  SelectionModel selection_{{timed_data_service_}};

  MetrixGraph* graph_ = nullptr;

  views::GraphRange prezoom_horizontal_range_;

  CommandRegistry command_registry_;
};
