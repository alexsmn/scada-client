#include "components/summary/summary_view.h"

#include "base/excel.h"
#include "client_utils.h"
#include "common_resources.h"
#include "components/summary/summary_model.h"
#include "controller_factory.h"
#include "controls/grid.h"
#include "print_util.h"
#include "services/dialog_service.h"
#include "time_range.h"
#include "window_definition.h"

const WindowInfo kWindowInfo = {ID_SUMMARY_VIEW, "Summ", L"Сводка",
                                WIN_INS | WIN_CAN_PRINT};

REGISTER_CONTROLLER(SummaryView, kWindowInfo);

SummaryView::SummaryView(const ControllerContext& context)
    : Controller{context},
      model_{std::make_unique<SummaryModel>(
          SummaryModelContext{timed_data_service_})} {}

UiView* SummaryView::Init(const WindowDefinition& definition) {
  model_->Load(definition);

  grid_.reset(new Grid(*model_, model_->row_model(), model_->column_model()));

  grid_->SetContextMenuHandler([this](const UiPoint& point) {
    controller_delegate_.ShowPopupMenu(0, point, true);
  });

  grid_->SetRowHeaderWidth(150);
  grid_->SetRowHeaderVisible(true);

  return grid_->CreateParentIfNecessary();
}

void SummaryView::Save(WindowDefinition& definition) {
  model_->Save(definition);
}

std::optional<SummaryModel::AggregationFunction> GetAggregationType(
    unsigned command_id) {
  switch (command_id) {
    case ID_AGGREGATION_COUNT:
      return SummaryModel::AggregationFunction::Count;
    case ID_AGGREGATION_LAST:
      return SummaryModel::AggregationFunction::Last;
    case ID_AGGREGATION_MIN:
      return SummaryModel::AggregationFunction::Min;
    case ID_AGGREGATION_MAX:
      return SummaryModel::AggregationFunction::Max;
    case ID_AGGREGATION_SUM:
      return SummaryModel::AggregationFunction::Sum;
    case ID_AGGREGATION_AVG:
      return SummaryModel::AggregationFunction::Avg;
    default:
      return std::nullopt;
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
  switch (command_id) {
    case ID_EXPORT:
      return this;
  }

  if (GetInterval(command_id))
    return this;

  if (GetAggregationType(command_id))
    return this;

  return __super::GetCommandHandler(command_id);
}

bool SummaryView::IsCommandChecked(unsigned command_id) const {
  if (auto interval = GetInterval(command_id))
    return model_->interval() == *interval;

  if (auto aggregation_function = GetAggregationType(command_id))
    return model_->aggregation_function() == *aggregation_function;

  return CommandHandler::IsCommandChecked(command_id);
}

void SummaryView::ExecuteCommand(unsigned command_id) {
  switch (command_id) {
    case ID_EXPORT:
      ExportToExcel();
      break;
  }

  if (auto interval = GetInterval(command_id)) {
    model_->SetInterval(*interval);
    return;
  }

  if (auto aggregation_function = GetAggregationType(command_id)) {
    model_->SetAggregationFunction(*aggregation_function);
    return;
  }

  __super::ExecuteCommand(command_id);
}

void SummaryView::ExportToExcel() {
  try {
    ExcelSheetModel sheet;

#if defined(UI_VIEWS)
    int row_count = grid_->row_model().GetCount();
    int column_count = grid_->column_model().GetCount();

    sheet.SetDataSize(row_count + 1, column_count + 1);

    // Column titles.
    for (int i = 0; i < column_count; ++i) {
      const auto& title = grid_->column_model().GetTitle(i);
      sheet.SetData(1, 2 + i, base::win::ScopedVariant(title.c_str()));
    }

    // Row titles.
    for (int i = 0; i < row_count; ++i) {
      const auto& title = grid_->row_model().GetTitle(i);
      sheet.SetData(2 + i, 1, base::win::ScopedVariant(title.c_str()));
    }

    // Cells.
    for (int i = 0; i < row_count; ++i) {
      for (int j = 0; j < column_count; ++j) {
        const auto& text = grid_->model().GetCellText(i, j);
        sheet.SetData(2 + i, 2 + j, base::win::ScopedVariant(text.c_str()));
      }
    }
#endif

    Excel excel;
    excel.NewWorkbook();
    excel.NewSheet(sheet);
    excel.SetVisible();

  } catch (HRESULT /*err*/) {
    dialog_service_.RunMessageBox(L"Ошибка при экспорте.", L"Экспорт",
                                  MessageBoxMode::Error);
  }
}

TimeModel* SummaryView::GetTimeModel() {
  return model_.get();
}

void SummaryView::Print(PrintService& print_service) {
  PrintGrid(PrintGridContext{print_service, *model_, grid_->column_model(),
                             grid_->row_model()});
}
