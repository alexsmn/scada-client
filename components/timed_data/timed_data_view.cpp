#include "components/timed_data/timed_data_view.h"

#include "base/win/win_util2.h"
#include "common/formula_util.h"
#include "common_resources.h"
#include "controller_delegate.h"
#include "controller_factory.h"
#include "controls/table.h"
#include "model/data_items_node_ids.h"
#include "model/scada_node_ids.h"
#include "node_service/node_service.h"
#include "services/dialog_service.h"
#include "services/profile.h"
#include "window_definition.h"
#include "window_definition_util.h"

namespace {

const wchar_t kValueColumnTitle[] = L"Значение";

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

TimedDataView::TimedDataView(const ControllerContext& context)
    : ControllerContext{context} {}

UiView* TimedDataView::Init(const WindowDefinition& definition) {
  if (const WindowItem* item = definition.FindItem("Item"))
    model_.SetFormula(item->GetString("path"));

  if (auto time_range = RestoreTimeRange(definition))
    model_.SetTimeRange(*time_range);

  if (const auto* item = definition.FindItem("View")) {
    mirror_model_.SetMirrored(
        item->GetBool("mirrored", profile_.timed_data.mirrored));
  } else {
    mirror_model_.SetMirrored(profile_.timed_data.mirrored);
  }

  view_ = std::make_unique<Table>(
      mirror_model_,
      std::vector<ui::TableColumn>(s_columns, s_columns + _countof(s_columns)));
  view_->SetShowGrid(true);

  view_->SetContextMenuHandler([this](const UiPoint& point) {
    controller_delegate_.ShowPopupMenu(IDR_ITEM_POPUP, point, true);
  });

#if defined(UI_QT)
  view_->horizontalHeader()->setSectionsClickable(true);
  view_->horizontalHeader()->setSortIndicatorShown(true);
  view_->horizontalHeader()->setSortIndicator(
      0, mirror_model_.mirrored() ? Qt::DescendingOrder : Qt::AscendingOrder);
  QObject::connect(view_->horizontalHeader(), &QHeaderView::sectionClicked,
                   [this](int logical_index) {
                     if (logical_index == 0) {
                       mirror_model_.SetMirrored(!mirror_model_.mirrored());
                       profile_.timed_data.mirrored = mirror_model_.mirrored();
                     }
                     // WARNING: Must reset sort indicator if |logical_index !=
                     // 0|.
                     view_->horizontalHeader()->setSortIndicator(
                         0, mirror_model_.mirrored() ? Qt::DescendingOrder
                                                     : Qt::AscendingOrder);
                   });
#endif

  selection_.SelectTimedData(model_.timed_data());

  return view_->CreateParentIfNecessary();
}

void TimedDataView::Save(WindowDefinition& definition) {
  WindowItem& item = definition.AddItem("Item");
  item.SetString("path", model_.timed_data().formula());
  SaveTimeRange(definition, model_.GetTimeRange());
  definition.AddItem("View").SetBool("mirrored", mirror_model_.mirrored());
}

std::string GetTimedDataUnits(const TimedDataSpec& spec) {
  auto node = spec.GetNode();
  if (!node)
    return std::string();
  return node[data_items::id::AnalogItemType_EngineeringUnits].value().get_or(
      std::string());
}

void TimedDataView::UpdateColumnTitles() {
  /*int index = view_->FindVisibleColumn(CID_VALUE);
  if (index == -1)
    return;

  std::wstring units =
  base::SysNativeMBToWide(GetTimedDataUnits(model_.timed_data()));
  std::wstring title = base::StringPrintf(L"%ls, %ls", kValueColumnTitle,
  units.c_str()); view_->SetVisibleColumnTitle(index, title);*/
}

std::wstring TimedDataView::MakeTitle() const {
  return model_.timed_data().GetTitle();
}

void TimedDataView::AddContainedItem(const scada::NodeId& node_id,
                                     unsigned flags) {
  model_.SetFormula(MakeNodeIdFormula(node_id));
}

CommandHandler* TimedDataView::GetCommandHandler(unsigned command_id) {
  return Controller::GetCommandHandler(command_id);
}

void TimedDataView::ExecuteCommand(unsigned command) {
  __super::ExecuteCommand(command);
}

bool TimedDataView::IsWorking() const {
  return !model_.timed_data().ready();
}

TimeModel* TimedDataView::GetTimeModel() {
  return &model_;
}

ExportModel::ExportData TimedDataView::GetExportData() {
  return TableExportData{mirror_model_, view_->columns()};
}

std::optional<OpenContext> TimedDataView::GetOpenContext() const {
  const auto& node = model_.timed_data().GetNode();
  const auto& node_id = node.node_id();
  if (node_id.is_null())
    return std::nullopt;

  OpenContext context;

  context.title = model_.timed_data().GetTitle();
  context.node_ids.emplace_back(node_id);

  const auto& selected_rows = view_->GetSelectedRows();
  if (selected_rows.size() >= 2) {
    auto p = std::minmax_element(selected_rows.begin(), selected_rows.end());
    const int row1 = mirror_model_.MapToSource(*p.first);
    const int row2 = mirror_model_.MapToSource(*p.second);
    const auto& first_data_value = model_.value(std::min(row1, row2));
    const auto& last_data_value = model_.value(std::max(row1, row2));
    context.time_range = TimeRange{first_data_value.source_timestamp,
                                   last_data_value.source_timestamp +
                                       scada::Duration::FromMilliseconds(1)};
  }

  return std::move(context);
}