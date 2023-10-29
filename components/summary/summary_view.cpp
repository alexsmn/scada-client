#include "components/summary/summary_view.h"

#include "aui/grid.h"
#include "base/time_range.h"
#include "client_utils.h"
#include "common_resources.h"
#include "components/summary/summary_model.h"
#include "controller/controller_delegate.h"
#include "profile/window_definition.h"
#include "aui/dialog_service.h"

namespace {

constexpr std::pair<unsigned, scada::NumericId> kAggregateCommands[] = {
    {ID_AGGREGATION_COUNT, scada::id::AggregateFunction_Count},
    {ID_AGGREGATION_START, scada::id::AggregateFunction_Start},
    {ID_AGGREGATION_END, scada::id::AggregateFunction_End},
    {ID_AGGREGATION_MIN, scada::id::AggregateFunction_Minimum},
    {ID_AGGREGATION_MAX, scada::id::AggregateFunction_Maximum},
    {ID_AGGREGATION_SUM, scada::id::AggregateFunction_Total},
    {ID_AGGREGATION_AVG, scada::id::AggregateFunction_Average},
};

constexpr std::pair<unsigned, base::TimeDelta> kIntervalCommands[] = {
    {ID_INTERVAL_1M, base::TimeDelta::FromMinutes(1)},
    {ID_INTERVAL_5M, base::TimeDelta::FromMinutes(5)},
    {ID_INTERVAL_15M, base::TimeDelta::FromMinutes(15)},
    {ID_INTERVAL_30M, base::TimeDelta::FromMinutes(30)},
    {ID_INTERVAL_1H, base::TimeDelta::FromHours(1)},
    {ID_INTERVAL_12H, base::TimeDelta::FromHours(12)},
    {ID_INTERVAL_1D, base::TimeDelta::FromDays(1)},
};

}  // namespace

SummaryView::SummaryView(const ControllerContext& context)
    : ControllerContext{context},
      model_{std::make_shared<SummaryModel>(
          SummaryModelContext{node_service_, timed_data_service_})} {}

UiView* SummaryView::Init(const WindowDefinition& definition) {
  model_->Load(definition);

  grid_ = new aui::Grid{
      model_, std::shared_ptr<aui::HeaderModel>{model_, &model_->row_model()},
      std::shared_ptr<aui::HeaderModel>{model_, &model_->column_model()}};

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
      if (auto node_id = model_->timed_data(column).node_id();
          !node_id.is_null()) {
        node_ids.emplace(std::move(node_id));
      }
    }
    return node_ids;
  };

  grid_->SetContextMenuHandler([this](const aui::Point& point) {
    controller_delegate_.ShowPopupMenu(nullptr, 0, point, true);
  });

  grid_->SetRowHeaderWidth(150);
  grid_->SetRowHeaderVisible(true);

  if (auto* state = definition.FindItem("State"))
    grid_->RestoreState(state->attributes);

  for (const auto& [command_id, interval] : kIntervalCommands) {
    command_registry_.AddCommand(Command{command_id}
                                     .set_execute_handler([this, interval] {
                                       model_->SetInterval(interval);
                                     })
                                     .set_checked_handler([this, interval] {
                                       return model_->interval() == interval;
                                     }));
  }

  for (const auto& [command_id, aggregate_type] : kAggregateCommands) {
    command_registry_.AddCommand(
        Command{command_id}
            .set_execute_handler([this, aggregate_type = aggregate_type] {
              model_->SetAggregateType(aggregate_type);
            })
            .set_checked_handler([this, aggregate_type = aggregate_type] {
              return model_->aggregate_type() == aggregate_type;
            }));
  }

  return grid_->CreateParentIfNecessary();
}

void SummaryView::Save(WindowDefinition& definition) {
  model_->Save(definition);

  definition.AddItem("State").attributes = grid_->SaveState();
}

CommandHandler* SummaryView::GetCommandHandler(unsigned command_id) {
  return command_registry_.GetCommandHandler(command_id);
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
    if (auto node_id = timed_data.node_id(); !node_id.is_null()) {
      context.node_ids.emplace_back(std::move(node_id));
    }
  }

  if (const auto& rows = grid_->GetSelectedRows(); !rows.empty()) {
    auto [min_row, max_row] = std::minmax_element(rows.begin(), rows.end());
    auto start_time = model_->GetRowTime(*min_row);
    auto end_time = model_->GetRowTime(*max_row) + model_->interval();
    context.time_range = TimeRange{start_time, end_time};
  }

  return context;
}
