#pragma once

#include "aui/color.h"
#include "aui/types.h"
#include "controller/command_registry.h"
#include "controller/contents_model.h"
#include "controller/controller.h"
#include "controller/controller_context.h"
#include "controller/selection_model.h"
#include "controller/series_model.h"
#include "controller/time_model.h"
#include "graph/metrix_graph.h"

namespace scada {
struct RelativeTimeRange;
}

class GraphView : protected ControllerContext,
                  public Controller,
                  public ContentsModel,
                  public TimeModel,
                  public SeriesModel,
                  private Graph::Controller {
 public:
  explicit GraphView(const ControllerContext& context);

  bool FindColor(scada::aui::Color color) const;
  scada::aui::Color NewColor() const;

  void SetGraphColor(scada::aui::Color color);

  // Controller methods
  virtual bool IsWorking() const override;
  virtual std::unique_ptr<UiView> Init(
      const WindowDefinition& definition) override;
  virtual void Save(WindowDefinition& definition) override;
  virtual CommandHandler* GetCommandHandler(unsigned command_id) override;
  virtual SelectionModel* GetSelectionModel() override { return &selection_; }
  virtual ContentsModel* GetContentsModel() override { return this; }
  virtual TimeModel* GetTimeModel() override { return this; }
  virtual SeriesModel* GetSeriesModel() override { return this; }

  // SeriesModel — the configurable series' presentation, rendered by the shell
  // Inspector. `GetConfigurableLine()` is what "the" series means here: the
  // selected pane's primary line, falling back to the first pane that has one.
  virtual bool HasSeries() const override;
  virtual scada::aui::Color GetSeriesColor() const override;
  virtual void SetSeriesColor(scada::aui::Color color) override;
  virtual bool IsSeriesOnOwnPane() const override;
  virtual bool AreSeriesDotsShown() const override;
  virtual bool IsSeriesStepped() const override;

  // ContentsModel
  virtual void AddContainedItem(const scada::NodeId& node_id,
                                unsigned flags) override;
  virtual void RemoveContainedItem(const scada::NodeId& node_id) override;
  virtual NodeIdSet GetContainedItems() const override;

  // TimeModel
  virtual scada::RelativeTimeRange GetTimeRange() const override;
  virtual void SetTimeRange(
      const scada::RelativeTimeRange& time_range) override;

 private:
  std::u16string MakeTitle() const;

  void DeleteSelectedPane();

  // Tells the host that the configurable series or its presentation moved, so
  // the Inspector's series section re-reads this model. A no-op until a host
  // wires SeriesModel::change_handler.
  void NotifySeriesChanged();

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

  GraphRange prezoom_horizontal_range_;

  CommandRegistry command_registry_;
};
