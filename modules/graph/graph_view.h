#pragma once

#include "aui/color.h"
#include "aui/types.h"
#include "controller/command_registry.h"
#include "controller/contents_model.h"
#include "controller/controller.h"
#include "controller/controller_context.h"
#include "controller/selection_model.h"
#include "controller/time_model.h"
#include "graph/metrix_graph.h"

struct TimeRange;
class SeriesInspector;

class GraphView : protected ControllerContext,
                  public Controller,
                  public ContentsModel,
                  public TimeModel,
                  private Graph::Controller {
 public:
  explicit GraphView(const ControllerContext& context);

  bool FindColor(aui::Color color) const;
  aui::Color NewColor() const;

  void SetGraphColor(aui::Color color);

  // Controller methods
  virtual bool IsWorking() const override;
  virtual std::unique_ptr<UiView> Init(
      const WindowDefinition& definition) override;
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

  // Points the reshell series inspector at the currently configurable series
  // (no-op when the inspector is absent, i.e. under the legacy theme).
  void RefreshInspector();

  MetrixGraph::MetrixLine* GetConfigurableLine() const;

  void ChooseLineColor();
  void SetupLine();
  void ChooseGraphColor();

  void UndoZoom();

  // Delete all pane lines and remove items.
  void ClearPane(MetrixGraph::MetrixPane& pane);

  void ScrollToNow();

  void ToggleLegend();
  void ToggleLineProperty(unsigned command_id);
  void ToggleZoom();

  // Graph::Controller
  virtual void OnGraphModified() override;
  virtual void OnGraphSelectPane() override;
  virtual void OnLineItemChanged(GraphLine& line) override;
  virtual void OnSelectedCursorChanged() override;
  virtual void OnGraphActivated() override;

  SelectionModel selection_{{timed_data_service_}};

  MetrixGraph* graph_ = nullptr;

  // Reshell-only per-series inspector shown beside the chart; null under the
  // legacy theme. Owned by the returned container widget (Qt parent), not here.
  SeriesInspector* inspector_ = nullptr;

  GraphRange prezoom_horizontal_range_;

  CommandRegistry command_registry_;
};
