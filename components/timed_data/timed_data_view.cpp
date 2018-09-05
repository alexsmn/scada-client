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

namespace {

const base::char16 kValueColumnTitle[] = L"Значение";

const base::char16 kExportTitle[] = L"Экспорт";

const ui::TableColumn s_columns[] = {
    ui::TableColumn(TimedDataModel::CID_TIME,
                    L"Время",
                    150,
                    ui::TableColumn::LEFT),
    ui::TableColumn(TimedDataModel::CID_VALUE,
                    kValueColumnTitle,
                    150,
                    ui::TableColumn::RIGHT),
    ui::TableColumn(TimedDataModel::CID_QUALITY,
                    L"Качество",
                    65,
                    ui::TableColumn::LEFT),
    ui::TableColumn(TimedDataModel::CID_COLLECTION_TIME,
                    L"Время приема",
                    150,
                    ui::TableColumn::LEFT),
};

}  // namespace

// TimedDataView

const WindowInfo kWindowInfo = {ID_TIMED_DATA_VIEW,
                                "TimeVal",
                                L"Времена и значения",
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

  view_.reset(new Table(*model_, {s_columns, s_columns + _countof(s_columns)}));
  view_->SetShowGrid(true);

  view_->SetContextMenuHandler([this](const UiPoint& point) {
    controller_delegate_.ShowPopupMenu(IDR_TIMEVAL_POPUP, point, true);
  });

  return view_->CreateParentIfNecessary();
}

void TimedDataView::Save(WindowDefinition& definition) {
  WindowItem& item = definition.AddItem("Item");
  item.SetString("path", model_->timed_data().formula());
}

std::string GetTimedDataUnits(const rt::TimedDataSpec& spec) {
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
  switch (command_id) {
    case ID_EXPORT:
      return this;
  }

  return __super::GetCommandHandler(command_id);
}

void TimedDataView::ExecuteCommand(unsigned command) {
  switch (command) {
    case ID_EXPORT:
      Export();
      break;
    default:
      __super::ExecuteCommand(command);
      break;
  }
}

void TimedDataView::Export() {
  auto title = model_->timed_data().GetTitle();
  auto path = dialog_service_.SelectSaveFile(kExportTitle, title + L".csv");
  if (path.empty())
    return;

  try {
    model_->ExportToCsv(path);

  } catch (const std::runtime_error&) {
    dialog_service_.RunMessageBox(L"Ошибка при экспорте.", kExportTitle,
                                  MessageBoxMode::Error);
    return;
  }

  if (dialog_service_.RunMessageBox(
          L"Экспорт завершен. Открыть файл сейчас?", kExportTitle,
          MessageBoxMode::QuestionYesNo) == MessageBoxResult::Yes)
    win_util::OpenWithAssociatedProgram(path);
}

bool TimedDataView::IsWorking() const {
  return !model_->timed_data().ready();
}

TimeModel* TimedDataView::GetTimeModel() {
  return model_.get();
}
