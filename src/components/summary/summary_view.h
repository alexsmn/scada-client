#pragma once

#include <memory>

#include "controller.h"

#if defined(UI_QT)
class QTableView;
#elif defined(UI_VIEWS)
namespace views {
class GridView;
}
#endif

class SummaryModel;

class SummaryView : public Controller {
 public:
  explicit SummaryView(const ControllerContext& context);

  // Controller
  virtual UiView* Init(const WindowDefinition& definition) override;
  virtual void Save(WindowDefinition& definition) override;
  virtual CommandHandler* GetCommandHandler(unsigned command_id) override;
  virtual void ExecuteCommand(unsigned command) override;

 private:
  void ShowSetupDialog();
  void ExportToExcel();

  std::unique_ptr<SummaryModel> model_;

#if defined(UI_QT)
  std::unique_ptr<QTableView> grid_;
#elif defined(UI_VIEWS)
  std::unique_ptr<views::GridView> grid_;
#endif
};
