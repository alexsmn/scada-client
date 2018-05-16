#include "components/timed_data/timed_data_view.h"

#include "base/excel.h"
#include "base/strings/sys_string_conversions.h"
#include "base/win/scoped_variant.h"
#include "client_utils.h"
#include "common/formula_util.h"
#include "common/node_service.h"
#include "common/scada_node_ids.h"
#include "common_resources.h"
#include "components/timed_data/timed_data_model.h"
#include "controller_factory.h"
#include "services/dialog_service.h"
#include "window_definition.h"

#if defined(UI_QT)
#include "controls/qt/table.h"
#elif defined(UI_VIEWS)
#include "skia/ext/skia_utils_win.h"
#include "ui/views/controls/table/table_view.h"
#endif

#include <ATLComTime.h>

namespace {

const base::char16 kValueColumnTitle[] = L"Значение";

void ValueToVariant(const scada::Variant& value,
                    base::win::ScopedVariant& result) {
  switch (value.type()) {
    case scada::Variant::BOOL:
      result.Set(value.as_bool());
      break;
    case scada::Variant::INT32:
      result.Set(value.as_int32());
      break;
    case scada::Variant::INT64:
      result.Set(value.as_int64());
      break;
    case scada::Variant::DOUBLE:
      result.Set(value.as_double());
      break;
    case scada::Variant::STRING:
      result.Set(base::SysNativeMBToWide(value.as_string()).c_str());
      break;
  }
}

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
                    ui::TableColumn::LEFT)};

}  // namespace

// TimedDataView

REGISTER_CONTROLLER(TimedDataView, ID_TIMED_DATA_VIEW);

TimedDataView::TimedDataView(const ControllerContext& context)
    : Controller{context},
      model_{std::make_unique<TimedDataModel>(
          TimedDataModelContext{timed_data_service_})} {}

UiView* TimedDataView::Init(const WindowDefinition& definition) {
  if (const WindowItem* item = definition.FindItem("Item"))
    model_->SetFormula(item->GetString("path"));

#if defined(UI_QT)
  view_.reset(new Table(*model_, {s_columns, s_columns + _countof(s_columns)}));
  return view_.get();

#elif defined(UI_VIEWS)
  view_.reset(new views::TableView(*model_));
  view_->set_show_grid(true);
  view_->set_controller(this);

  view_->SetColumns(_countof(s_columns), s_columns);

  return &view_->CreateParentIfNecessary();
#endif
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

#if defined(UI_VIEWS)
void TimedDataView::ShowContextMenu(gfx::Point point) {
  controller_delegate_.ShowPopupMenu(IDR_TIMEVAL_POPUP, point, true);
}
#endif

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
      ExportToExcel();
      break;
    default:
      __super::ExecuteCommand(command);
      break;
  }
}

void TimedDataView::ExportToExcel() {
  try {
    if (model_->empty())
      throw E_FAIL;

    ExcelSheetModel sheet;

    sheet.SetDataSize(model_->GetRowCount(), _countof(s_columns));
    for (size_t i = 0; i < _countof(s_columns); ++i) {
      const ui::TableColumn& column = s_columns[i];
      sheet.SetData(1, i + 1, base::win::ScopedVariant(column.title.c_str()));
    }

    int row = 2;
    for (const auto& data_value : *model_) {
      base::win::ScopedVariant value;
      ValueToVariant(data_value.value, value);
      sheet.SetData(
          row, 1,
          base::win::ScopedVariant(
              COleDateTime(data_value.source_timestamp.ToFileTime()), VT_DATE));
      sheet.SetData(row, 2, value);
      base::string16 qualifier_string = FormatQuality(data_value.qualifier);
      sheet.SetData(row, 3, base::win::ScopedVariant(qualifier_string.c_str()));
      sheet.SetData(
          row, 4,
          base::win::ScopedVariant(
              COleDateTime(data_value.server_timestamp.ToFileTime()), VT_DATE));
      ++row;
    }

    Excel excel;
    excel.NewWorkbook();
    excel.NewSheet(sheet);
    excel.SetVisible();

  } catch (HRESULT /*err*/) {
    dialog_service_.RunMessageBox(L"Ошибка при экспорте.", L"Экспорт",
                                  MessageBoxMode::Error);
  }
}

bool TimedDataView::IsWorking() const {
  return !model_->timed_data().ready();
}

TimeModel* TimedDataView::GetTimeModel() {
  return model_.get();
}
