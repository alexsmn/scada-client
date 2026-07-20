#include "modules/timed_data/timed_data_controller.h"

#include "app/string_const.h"
#include "aui/dialog_service.h"
#include "aui/models/mirror_table_model.h"
#include "aui/table.h"
#include "base/time/default_clock.h"
#include "common/formula_util.h"
#include "controller/controller_delegate.h"
#include "model/data_items_node_ids.h"
#include "model/scada_node_ids.h"
#include "node_service/node_service.h"
#include "profile/profile.h"
#include "profile/window_definition.h"
#include "profile/window_definition_util.h"
#include "resources/common_resources.h"

#if defined(UI_QT)
#include <QHeaderView>
#endif

namespace {

const scada::aui::TableColumn s_columns[] = {
    {TimedDataModel::CID_TIME, kSourceTimestampTitle, 150,
     scada::aui::TableColumn::LEFT,
     scada::aui::TableColumn::DataType::DateTime},
    {TimedDataModel::CID_VALUE, kValueTitle, 150,
     scada::aui::TableColumn::RIGHT, scada::aui::TableColumn::DataType::General,
     /*monospace=*/true},
    {TimedDataModel::CID_QUALITY, u"Quality", 65,
     scada::aui::TableColumn::LEFT},
    {TimedDataModel::CID_COLLECTION_TIME, kServerTimestampTitle, 150,
     scada::aui::TableColumn::LEFT,
     scada::aui::TableColumn::DataType::DateTime},
};

struct MirrorTableModelHolder {
  explicit MirrorTableModelHolder(std::shared_ptr<TimedDataModel> model)
      : model_{std::move(model)} {}

  const std::shared_ptr<TimedDataModel> model_;
  scada::aui::MirrorTableModel mirror_model{*model_};
};

}  // namespace

// TimedDataController

TimedDataController::TimedDataController(const ControllerContext& context)
    : ControllerContext{context} {}

std::unique_ptr<UiView> TimedDataController::Init(
    const WindowDefinition& definition) {
  model_ = std::make_shared<TimedDataModel>(
      TimedDataModelContext{.clock_ = *scada::base::DefaultClock::GetInstance(),
                            .timed_data_service_ = timed_data_service_});

  model_->Init(definition);

  auto mirror_table_holder = std::make_shared<MirrorTableModelHolder>(model_);

  mirror_model_ = std::shared_ptr<scada::aui::MirrorTableModel>(
      mirror_table_holder, &mirror_table_holder->mirror_model);

  if (const auto* item = definition.FindItem("View")) {
    mirror_model_->SetMirrored(
        item->GetBool("mirrored", profile_.timed_data.mirrored));
  } else {
    mirror_model_->SetMirrored(profile_.timed_data.mirrored);
  }

  auto view = std::make_unique<scada::aui::Table>(
      mirror_model_, std::vector<scada::aui::TableColumn>(std::begin(s_columns),
                                                          std::end(s_columns)));
  view->SetShowGrid(true);

  view->SetContextMenuHandler([this](const scada::aui::Point& point) {
    // No view-specific static items: the node commands are supplied by the
    // generic cross-platform context menu (the former `IDR_ITEM_POPUP` carried
    // only the dynamic `<Item>` placeholder).
    controller_delegate_.ShowPopupMenu(nullptr, /*resource_id=*/0, point, true);
  });

#if defined(UI_QT)
  view->horizontalHeader()->setSectionsClickable(true);
  view->horizontalHeader()->setSortIndicatorShown(true);
  view->horizontalHeader()->setSortIndicator(
      0, mirror_model_->mirrored() ? Qt::DescendingOrder : Qt::AscendingOrder);
  QObject::connect(view->horizontalHeader(), &QHeaderView::sectionClicked,
                   [this](int logical_index) {
                     if (logical_index == 0) {
                       mirror_model_->SetMirrored(!mirror_model_->mirrored());
                       profile_.timed_data.mirrored = mirror_model_->mirrored();
                     }
                     // WARNING: Must reset sort indicator if |logical_index !=
                     // 0|.
                     view_->horizontalHeader()->setSortIndicator(
                         0, mirror_model_->mirrored() ? Qt::DescendingOrder
                                                      : Qt::AscendingOrder);
                   });
#endif

  view_ = view.get();

  selection_.SelectTimedData(model_->timed_data());

  return std::unique_ptr<UiView>{view.release()->CreateParentIfNecessary()};
}

void TimedDataController::Save(WindowDefinition& definition) {
  WindowItem& item = definition.AddItem("Item");
  item.SetString("path", model_->timed_data().formula());
  SaveTimeRange(definition, model_->GetTimeRange());
  definition.AddItem("View").SetBool("mirrored", mirror_model_->mirrored());
}

std::string GetTimedDataUnits(const TimedDataSpec& spec) {
  return spec.node()[scada::data_items::id::AnalogItemType_EngineeringUnits]
      .value()
      .get_or(std::string());
}

void TimedDataController::UpdateColumnTitles() {}

std::u16string TimedDataController::MakeTitle() const {
  return model_->timed_data().GetTitle();
}

void TimedDataController::AddContainedItem(const scada::NodeId& node_id,
                                           unsigned flags) {
  model_->SetFormula(MakeNodeIdFormula(node_id));
}

CommandHandler* TimedDataController::GetCommandHandler(unsigned command_id) {
  return command_registry_.GetCommandHandler(command_id);
}

bool TimedDataController::IsWorking() const {
  return !model_->timed_data().ready();
}

TimeModel* TimedDataController::GetTimeModel() {
  return model_.get();
}

ExportModel::ExportData TimedDataController::GetExportData() {
  return TableExportData{*mirror_model_, view_->columns()};
}

std::optional<OpenContext> TimedDataController::GetOpenContext() const {
  const auto& node = model_->timed_data().node();
  if (!node)
    return std::nullopt;

  OpenContext context;
  context.node = node;

  const auto& selected_rows = view_->GetSelectedRows();
  if (selected_rows.size() >= 2) {
    auto p = std::minmax_element(selected_rows.begin(), selected_rows.end());
    const int row1 = mirror_model_->MapToSource(*p.first);
    const int row2 = mirror_model_->MapToSource(*p.second);
    const auto& first_data_value = model_->value(std::min(row1, row2));
    const auto& last_data_value = model_->value(std::max(row1, row2));
    context.time_range = TimeRange{first_data_value.source_timestamp,
                                   last_data_value.source_timestamp +
                                       scada::Duration::FromMilliseconds(1)};
  }

  return context;
}
