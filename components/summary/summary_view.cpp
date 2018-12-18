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

bool Convert(const scada::Variant& source, base::win::ScopedVariant& target) {
  assert(!source.is_array());
  if (source.is_array())
    return false;

  static_assert(scada::Variant::COUNT == 19);

  switch (source.type()) {
    case scada::Variant::Type::EMPTY:
      target.Reset();
      return true;
    case scada::Variant::Type::BOOL:
      target.Set(source.get<scada::Boolean>());
      return true;
    case scada::Variant::Type::INT8:
      target.Set(source.get<scada::Int8>());
      return true;
    case scada::Variant::Type::UINT8:
      target.Set(source.get<scada::UInt8>());
      return true;
    case scada::Variant::Type::INT16:
      target.Set(source.get<scada::Int16>());
      return true;
    case scada::Variant::Type::UINT16:
      target.Set(source.get<scada::UInt16>());
      return true;
    case scada::Variant::Type::INT32:
      target.Set(source.get<scada::Int32>());
      return true;
    case scada::Variant::Type::UINT32:
      target.Set(source.get<scada::UInt32>());
      return true;
    case scada::Variant::Type::INT64:
      target.Set(source.get<scada::Int64>());
      return true;
    case scada::Variant::Type::UINT64:
      target.Set(source.get<scada::UInt64>());
      return true;
    case scada::Variant::Type::DOUBLE:
      target.Set(source.get<scada::Double>());
      return true;
    case scada::Variant::STRING:
      target.Set(base::SysNativeMBToWide(source.get<scada::String>()).c_str());
      return true;
    case scada::Variant::LOCALIZED_TEXT:
      target.Set(source.get<scada::LocalizedText>().c_str());
      return true;
    case scada::Variant::DATE_TIME:
      target.Set(static_cast<DATE>(source.get<scada::DateTime>().ToDoubleT()));
      return true;
    default:
      assert(false);
      return false;
  }
}

const WindowInfo kWindowInfo = {ID_SUMMARY_VIEW, "Summ", L"Сводка",
                                WIN_INS | WIN_CAN_PRINT};

REGISTER_CONTROLLER(SummaryView, kWindowInfo);

SummaryView::SummaryView(const ControllerContext& context)
    : Controller{context},
      model_{std::make_unique<SummaryModel>(
          SummaryModelContext{node_service_, timed_data_service_})} {}

UiView* SummaryView::Init(const WindowDefinition& definition) {
  model_->Load(definition);

  grid_.reset(new Grid(*model_, model_->row_model(), model_->column_model()));

  grid_->SetSelectionChangeHandler([this] {
    auto columns = grid_->GetSelectedColumns();
    if (columns.empty())
      selection().Clear();
    else if (columns.size() == 1)
      selection().SelectTimedData(model_->timed_data(columns[0]));
    else
      selection().SelectMultiple();
  });

  selection().multiple_handler = [this] {
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

  return grid_->CreateParentIfNecessary();
}

void SummaryView::Save(WindowDefinition& definition) {
  model_->Save(definition);
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
  switch (command_id) {
    case ID_EXPORT:
      return this;
  }

  if (GetInterval(command_id))
    return this;

  if (!GetAggregateType(command_id).is_null())
    return this;

  return __super::GetCommandHandler(command_id);
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

  switch (command_id) {
    case ID_EXPORT:
      ExportToExcel();
      break;
    default:
      __super::ExecuteCommand(command_id);
  }
}

void SummaryView::ExportToExcel() {
  ExcelSheetModel sheet;

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
  base::win::ScopedVariant data;
  for (int i = 0; i < row_count; ++i) {
    for (int j = 0; j < column_count; ++j) {
      const auto& data_value = model_->GetDataValue(i, j);
      if (Convert(data_value.value, data))
        sheet.SetData(2 + i, 2 + j, std::move(data));
    }
  }

  try {
    Excel excel;
    excel.NewWorkbook();
    excel.NewSheet(sheet);
    excel.SetVisible();

  } catch (HRESULT /*err*/) {
    dialog_service_.RunMessageBox(
        L"Не удалось выполнить экспорт. Проверьте корректность установки "
        L"Microsoft Excel.",
        L"Экспорт", MessageBoxMode::Error);
  }
}

ContentsModel* SummaryView::GetContentsModel() {
  return model_.get();
}

TimeModel* SummaryView::GetTimeModel() {
  return model_.get();
}

void SummaryView::Print(PrintService& print_service) {
  PrintGrid(PrintGridContext{print_service, *model_, grid_->column_model(),
                             grid_->row_model()});
}
