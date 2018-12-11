#pragma once

#include "contents_model.h"
#include "controller.h"

#if defined(UI_VIEWS)
#include "ui/gfx/font.h"
#endif

#if defined(UI_VIEWS)
namespace WTL {
template <bool t_bManaged>
class CImageListT;
typedef CImageListT<true> CImageListManaged;
}  // namespace WTL
#endif

class Table;
class TableModel;

class TableView : public Controller, public ContentsModel {
 public:
  explicit TableView(const ControllerContext& context);
  virtual ~TableView();

  void DeleteSelection();

  // Controller
  virtual UiView* Init(const WindowDefinition& definition) override;
  virtual void Save(WindowDefinition& definition) override;
  virtual ContentsModel* GetContentsModel() override { return this; }
  virtual void Print(PrintService& print_service) override;

  // ContentsModel
  virtual void AddContainedItem(const scada::NodeId& node_id,
                                unsigned flags) override;
  virtual void RemoveContainedItem(const scada::NodeId& node_id) override;
  virtual NodeIdSet GetContainedItems() const override;

  // CommandHandler
  virtual CommandHandler* GetCommandHandler(unsigned command_id);
  virtual void ExecuteCommand(unsigned command);

 private:
  void MoveRow(bool up);

  NodeIdSet GetMultipleSelection();

  void OnSelectionChanged();
  void OnDoubleClick();
  bool OnKeyPressed(KeyCode key_code);

  std::unique_ptr<TableModel> model_;
  std::unique_ptr<Table> view_;

#if defined(UI_VIEWS)
  std::unique_ptr<WTL::CImageListManaged> image_list_;

  gfx::Font new_row_font_;
#endif
};
