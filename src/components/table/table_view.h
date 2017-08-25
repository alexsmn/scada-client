#pragma once

#include "controller.h"
#include "contents_model.h"

#if defined(UI_VIEWS)
#include "ui/gfx/font.h"
#include "ui/views/controls/table/table_controller.h"
#endif

#if defined(UI_VIEWS)
namespace WTL {
template <bool t_bManaged> class CImageListT;
typedef CImageListT<true> CImageListManaged;
}
#endif

class Table;
class TableModel;

class TableView : public Controller,
                  public ContentsModel
#if defined(UI_VIEWS)
                  , private views::TableController
#endif
{
public:
  explicit TableView(const ControllerContext& context);
  virtual ~TableView();

  void DeleteSelection();

  // Controller
  virtual bool CanClose() const override;
  virtual UiView* Init(const WindowDefinition& definition) override;
  virtual void Save(WindowDefinition& definition) override;
  virtual ContentsModel* GetContentsModel() override { return this; }

  // ContentsModel
  virtual void AddContainedItem(const scada::NodeId& node_id, unsigned flags) override;
  virtual void RemoveContainedItem(const scada::NodeId& node_id) override;
  virtual NodeIdSet GetContainedItems() const override;

  // CommandHandler
  virtual CommandHandler* GetCommandHandler(unsigned command_id);
  virtual void ExecuteCommand(unsigned command);

private:
  void MoveRow(bool up);

  NodeIdSet GetMultipleSelection();

#if defined(UI_QT)
  void OnSelectionChanged();
 
#elif defined(UI_VIEWS)
  // TableController
  virtual bool OnDrawCell(views::TableView& sender, gfx::Canvas* canvas,
                          int row, int visible_column_index,
                          const gfx::Rect& rect) override;
  virtual views::ComboTextfield* OnCreateEditor(views::TableView& sender,
                                                int row, int column_id) override;
  virtual bool OnEditCellText(views::TableView& sender, int row, int column_id,
                              const base::string16& text) override;
  virtual bool CanEditCell(views::TableView& sender, int row, int column_id) override;
  virtual void OnGetAutocompleteList(views::TableView& sender,
                                     const base::string16& text, int& start,
                                     std::vector<base::string16>& list) override;
  virtual void OnSelectionChanged(views::TableView& sender) override;
  virtual bool OnDoubleClick() override;
  virtual void ShowContextMenu(gfx::Point point) override;
  virtual bool OnKeyPressed(views::TableView& sender, ui::KeyboardCode key_code) override;
#endif

  std::unique_ptr<TableModel> model_;
  std::unique_ptr<Table> view_;

#if defined(UI_VIEWS)
  std::unique_ptr<WTL::CImageListManaged> image_list_;
  
  gfx::Font new_row_font_;
#endif
};
