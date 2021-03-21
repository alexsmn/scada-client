#include "components/summary/summary_view.h"

#include "client_utils.h"
#include "common_resources.h"
#include "components/summary/summary_model.h"
#include "controller_delegate.h"
#include "controller_factory.h"
#include "controls/grid.h"
#include "services/dialog_service.h"
#include "time_range.h"
#include "window_definition.h"

SummaryView::SummaryView(const ControllerContext& context)
    : ControllerContext{context},
      model_{std::make_unique<SummaryModel>(
          SummaryModelContext{node_service_, timed_data_service_})} {}

UiView* SummaryView::Init(const WindowDefinition& definition) {
  model_->Load(definition);

  grid_.reset(new Grid(*model_, model_->row_model(), model_->column_model()));

  grid_->SetSelectionChangeHandler([this] {
    auto columns = grid_->GetSelectedColumns();
    if (columns.empty())
      selection_.Clear();
    else if (columns.size() == 1)
      selection_.SelectTimedData(model_->timed_data(columns[0]));
    else
      selection_.SelectMultiple();
  });

  selection_.multiple_handler = [this] {
    NodeIdSet node_ids;
    for (int column : grid_->GetSelectedColumns()) {
      if (auto node = model_->timed_data(column).GetNode())
        node_ids.emplace(node.node_id());
    }
    return node_ids;
  };

  grid_->SetContextMenuHandler([this](const UiPoint& point) {
    controller_delegate_.ShowPopupMenu(0, point, true);
  });

  grid_->SetRowHeaderWidth(150);
  grid_->SetRowHeaderVisible(true);

  if (auto* state = definition.FindItem("State"))
    grid_->RestoreState(state->attributes);

  return grid_->CreateParentIfNecessary();
}

void SummaryView::Save(WindowDefinition& definition) {
  model_->Save(definition);

  definition.AddItem("State").attributes = grid_->SaveState();
}

scada::NodeId GetAggregateType(unsigned command_id) {
  switch (command_id) {
    case ID_AGGREGATION_COUNT:
      return scada::id::AggregateFunction_Count;
    case ID_AGGREGATION_START:
      return scada::id::AggregateFunction_Start;
    case ID_AGGREGATION_END:
      return scada::id::AggregateFunction_End;
    case ID_AGGREGATION_MIN:
      return scada::id::AggregateFunction_Minimum;
    case ID_AGGREGATION_MAX:
      return scada::id::AggregateFunction_Maximum;
    case ID_AGGREGATION_SUM:
      return scada::id::AggregateFunction_Total;
    case ID_AGGREGATION_AVG:
      return scada::id::AggregateFunction_Average;
    default:
      return {};
  }
}

std::optional<base::TimeDelta> GetInterval(unsigned command_id) {
  switch (command_id) {
    case ID_INTERVAL_1M:
      return base::TimeDelta::FromMinutes(1);
    case ID_INTERVAL_5M:
      return base::TimeDelta::FromMinutes(5);
    case ID_INTERVAL_15M:
      return base::TimeDelta::FromMinutes(15);
    case ID_INTERVAL_30M:
      return base::TimeDelta::FromMinutes(30);
    case ID_INTERVAL_1H:
      return base::TimeDelta::FromHours(1);
    case ID_INTERVAL_12H:
      return base::TimeDelta::FromHours(12);
    case ID_INTERVAL_1D:
      return base::TimeDelta::FromDays(1);
    default:
      return std::nullopt;
  }
}

CommandHandler* SummaryView::GetCommandHandler(unsigned command_id) {
  if (GetInterval(command_id))
    return this;

  if (!GetAggregateType(command_id).is_null())
    return this;

  return Controller::GetCommandHandler(command_id);
}

bool SummaryView::IsCommandChecked(unsigned command_id) const {
  if (auto interval = GetInterval(command_id))
    return model_->interval() == *interval;

  auto aggregate_type = GetAggregateType(command_id);
  if (!aggregate_type.is_null())
    return model_->aggregate_type() == aggregate_type;

  return CommandHandler::IsCommandChecked(command_id);
}

void SummaryView::ExecuteCommand(unsigned command_id) {
  if (auto interval = GetInterval(command_id)) {
    model_->SetInterval(*interval);
    return;
  }

  auto aggregate_type = GetAggregateType(command_id);
  if (!aggregate_type.is_null()) {
    model_->SetAggregateType(std::move(aggregate_type));
    return;
  }

  __super::ExecuteCommand(command_id);
}

ContentsModel* SummaryView::GetContentsModel() {
  return model_.get();
}

TimeModel* SummaryView::GetTimeModel() {
  return model_.get();
}

ExportModel* SummaryView::GetExportModel() {
  return model_.get();
}

std::optional<OpenContext> SummaryView::GetOpenContext() const {
  const auto& selected_columns = grid_->GetSelectedColumns();
  if (selected_columns.empty())
    return std::nullopt;

  OpenContext context;

  for (auto i : selected_columns) {
    const auto& timed_data = model_->timed_data(i);
    if (const auto& node = timed_data.GetNode())
      context.node_ids.emplace_back(node.node_id());
  }

  if (const auto& rows = grid_->GetSelectedRows(); !rows.empty()) {
    auto [min_row, max_row] = std::minmax_element(rows.begin(), rows.end());
    auto start_time = model_->GetRowTime(*min_row);
    auto end_time = model_->GetRowTime(*max_row) + model_->interval();
    context.time_range = TimeRange{start_time, end_time};
  }

  return context;
}
