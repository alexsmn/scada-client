#include "components/timed_data/timed_data_view.h"

#include "base/win/win_util2.h"
#include "common/formula_util.h"
#include "common/node_service.h"
#include "common/scada_node_ids.h"
#include "common_resources.h"
#include "components/timed_data/timed_data_model.h"
#include "controller_factory.h"
#include "controls/table.h"
#include "services/dialog_service.h"
#include "window_definition.h"
#include "window_definition_util.h"

namespace {

const base::char16 kValueColumnTitle[] = L"Значение";

const ui::TableColumn s_columns[] = {
    {TimedDataModel::CID_TIME, L"Время", 150, ui::TableColumn::LEFT,
     ui::TableColumn::DataType::DateTime},
    {TimedDataModel::CID_VALUE, kValueColumnTitle, 150, ui::TableColumn::RIGHT},
    {TimedDataModel::CID_QUALITY, L"Качество", 65, ui::TableColumn::LEFT},
    {TimedDataModel::CID_COLLECTION_TIME, L"Время приема", 150,
     ui::TableColumn::LEFT, ui::TableColumn::DataType::DateTime},
};

}  // namespace

// TimedDataView

const WindowInfo kWindowInfo = {ID_TIMED_DATA_VIEW,
                                "TimeVal",
                                L"Данные",
                                WIN_INS | WIN_DISALLOW_NEW | WIN_CAN_PRINT,
                                0,
                                0,
                                0};

REGISTER_CONTROLLER(TimedDataView, kWindowInfo);

TimedDataView::TimedDataView(const ControllerContext& context)
    : Controller{context},
      model_{std::make_unique<TimedDataModel>(
          TimedDataModelContext{timed_data_service_})} {}

UiView* TimedDataView::Init(const WindowDefinition& definition) {
  if (const WindowItem* item = definition.FindItem("Item"))
    model_->SetFormula(item->GetString("path"));

  if (auto time_range = RestoreTimeRange(definition))
    model_->SetTimeRange(*time_range);

  view_.reset(new Table(*model_, {s_columns, s_columns + _countof(s_columns)}));
  view_->SetShowGrid(true);

  view_->SetContextMenuHandler([this](const UiPoint& point) {
    controller_delegate_.ShowPopupMenu(IDR_TIMEVAL_POPUP, point, true);
  });

  selection().SelectTimedData(model_->timed_data());

  return view_->CreateParentIfNecessary();
}

void TimedDataView::Save(WindowDefinition& definition) {
  WindowItem& item = definition.AddItem("Item");
  item.SetString("path", model_->timed_data().formula());
  SaveTimeRange(definition, model_->GetTimeRange());
}

std::string GetTimedDataUnits(const TimedDataSpec& spec) {
  auto node = spec.GetNode();
  if (!node)
    return std::string();
  return node[id::AnalogItemType_EngineeringUnits].value().get_or(
      std::string());
}

void TimedDataView::UpdateColumnTitles() {
  /*int index = view_->FindVisibleColumn(CID_VALUE);
  if (index == -1)
    return;

  base::string16 units =
  base::SysNativeMBToWide(GetTimedDataUnits(model_->timed_data()));
  base::string16 title = base::StringPrintf(L"%ls, %ls", kValueColumnTitle,
  units.c_str()); view_->SetVisibleColumnTitle(index, title);*/
}

base::string16 TimedDataView::MakeTitle() const {
  return model_->timed_data().GetTitle();
}

void TimedDataView::AddContainedItem(const scada::NodeId& node_id,
                                     unsigned flags) {
  model_->SetFormula(MakeNodeIdFormula(node_id));
}

CommandHandler* TimedDataView::GetCommandHandler(unsigned command_id) {
  return Controller::GetCommandHandler(command_id);
}

void TimedDataView::ExecuteCommand(unsigned command) {
  __super::ExecuteCommand(command);
}

bool TimedDataView::IsWorking() const {
  return !model_->timed_data().ready();
}

TimeModel* TimedDataView::GetTimeModel() {
  return model_.get();
}

ExportModel::ExportData TimedDataView::GetExportData() {
  return TableExportData{*model_, view_->columns()};
}
