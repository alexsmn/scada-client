#include "components/summary/summary_view.h"

#include "base/excel.h"
#include "client_utils.h"
#include "common_resources.h"
#include "components/summary/summary_model.h"
#include "controller_factory.h"
#include "controls/grid.h"
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

#if defined(UI_VIEWS)
  grid_->SetRowHeaderWidth(150);
  grid_->SetRowHeadersVisible(true);
#endif

  return grid_->CreateParentIfNecessary();
}

void SummaryView::Save(WindowDefinition& definition) {
  model_->Save(definition);
}

CommandHandler* SummaryView::GetCommandHandler(unsigned command_id) {
  switch (command_id) {
    case ID_EXPORT:
      return this;
  }

  return __super::GetCommandHandler(command_id);
}

void SummaryView::ExecuteCommand(unsigned command) {
  switch (command) {
    case ID_EXPORT:
      ExportToExcel();
      break;
    default:
      __super::ExecuteCommand(command);
      break;
  }
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
