#pragma once

#include <memory>

#include "client/controller.h"
#include "ui/base/models/header_model.h"

#if defined(UI_VIEWS)
#include "ui/views/controls/grid/grid_controller.h"
#endif

class TransmissionModel;                      
#if defined(UI_QT)
class QTableView;
#endif

class TransmissionView : public Controller
#if defined(UI_VIEWS)
                         , private views::GridController
#endif
{
 public:
  explicit TransmissionView(const ControllerContext& context);
  virtual ~TransmissionView();

  // Controller events
  virtual UiView* Init(const WindowDefinition& definition) override;
  virtual CommandHandler* GetCommandHandler(unsigned command_id) override;
  virtual void ExecuteCommand(unsigned command) override;
  virtual ContentsModel* GetContentsModel() override;

 private:
  void WriteCell(int row, int col, LPCTSTR text);

  void DeleteSelection();

#if defined(UI_VIEWS)
  // GridController overrides
  virtual bool CanEditCell(views::GridView& sender, int row, int column) override;
  virtual bool OnGridEditCellText(views::GridView& sender, int row, int column, const base::string16& text) override;
  virtual void ShowContextMenu(gfx::Point point) override;
#endif

  std::unique_ptr<TransmissionModel> model_;
  ui::ColumnHeaderModel column_model_;

#if defined(UI_QT)
  std::unique_ptr<QTableView> grid_;
#elif defined(UI_VIEWS)
  std::unique_ptr<views::GridView> grid_;
#endif
};
