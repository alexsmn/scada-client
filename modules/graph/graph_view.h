#pragma once

#include "aui/color.h"
#include "aui/types.h"
#include "base/awaitable.h"
#include "controller/command_registry.h"
#include "controller/contents_model.h"
#include "controller/controller.h"
#include "controller/controller_context.h"
#include "controller/period_comparison.h"
#include "controller/selection_model.h"
#include "controller/series_model.h"
#include "controller/time_model.h"
#include "export/export_model.h"
#include "export/image_export_model.h"
#include "graph/metrix_graph.h"

#include <filesystem>
#include <memory>

class GraphEmptyState;
class GraphExportTable;

namespace scada {
struct RelativeTimeRange;
}

class GraphView : protected ControllerContext,
                  public Controller,
                  public ContentsModel,
                  public TimeModel,
                  public SeriesModel,
                  public ImageExportModel,
                  public ExportModel,
                  private Graph::Controller {
 public:
  explicit GraphView(const ControllerContext& context);
  ~GraphView();

  bool FindColor(scada::aui::Color color) const;
  scada::aui::Color NewColor() const;

  void SetGraphColor(scada::aui::Color color);

  // Controller methods
  virtual std::unique_ptr<UiView> Init(
      const WindowDefinition& definition) override;
  virtual void Save(WindowDefinition& definition) override;
  virtual CommandHandler* GetCommandHandler(unsigned command_id) override;
  virtual SelectionModel* GetSelectionModel() override { return &selection_; }
  virtual ContentsModel* GetContentsModel() override { return this; }
  virtual TimeModel* GetTimeModel() override { return this; }
  virtual SeriesModel* GetSeriesModel() override { return this; }
  virtual ImageExportModel* GetImageExportModel() override { return this; }
  virtual ExportModel* GetExportModel() override { return this; }

  // ImageExportModel — the graph as it is drawn on screen, for printing and
  // for "Save as Image" (backlog 874).
  virtual QSize GetImageSize() const override;
  virtual void RenderImage(QPainter& painter) override;

  // ExportModel — the values of every series over the visible time range, for
  // CSV and Excel export. The graph used to leave only as a picture.
  virtual ExportData GetExportData() override;

  // Writes the graph to |path| as a PNG. Returns false when it cannot.
  bool SaveImage(const std::filesystem::path& path);

  // SeriesModel — the configurable series' presentation, rendered by the shell
  // Inspector. `GetConfigurableLine()` is what "the" series means here: the
  // selected pane's primary line, falling back to the first pane that has one.
  virtual bool HasSeries() const override;
  virtual scada::aui::Color GetSeriesColor() const override;
  virtual void SetSeriesColor(scada::aui::Color color) override;
  virtual bool IsSeriesOnOwnPane() const override;
  virtual bool AreSeriesDotsShown() const override;
  virtual bool IsSeriesStepped() const override;

  // The earlier period drawn dashed over each line (PeriodComparison).
  PeriodComparison comparison() const { return comparison_; }
  void SetComparison(PeriodComparison comparison);

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

  // True while some pane plots at least one line.
  bool HasAnyLine() const;

  // Shows the empty-state hint while nothing is plotted and hides it once a
  // line is (backlog 1125). Every change of the lines ends in
  // NotifySeriesChanged(), which calls this.
  void UpdateEmptyState();

  MetrixGraph::MetrixLine* GetConfigurableLine() const;

  void ChooseLineColor();
  void SetupLine();
  void ChooseGraphColor();

  void UndoZoom();

  // Asks for a file name and saves the graph there as a PNG.
  Awaitable<void> SaveImageAs();

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

  // A child of |graph_|, which owns it.
  GraphEmptyState* empty_state_ = nullptr;

  GraphRange prezoom_horizontal_range_;

  // Removes every comparison line, then — when comparing — adds one after each
  // line: the same item, shifted, dashed, in its line's colour.
  void SyncComparisonLines();
  // Gives each comparison line its line's colour, weight, dots and steps
  // again, after the operator restyled the line.
  void RestyleComparisonLines();

  PeriodComparison comparison_ = PeriodComparison::kNone;

  CommandRegistry command_registry_;

  // The snapshot the last GetExportData() handed out; it refers to this.
  std::unique_ptr<GraphExportTable> export_table_;
};
