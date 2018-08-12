#pragma once

#include <memory>

#include "contents_model.h"
#include "controller.h"
#include "time_model.h"

#if defined(UI_VIEWS)
#include "ui/views/controls/table/table_controller.h"
#endif

#if defined(UI_VIEWS)
namespace WTL {
template <bool t_bManaged>
class CImageListT;
typedef CImageListT<false> CImageList;
typedef CImageListT<true> CImageListManaged;
}  // namespace WTL
#endif

class Table;
class EventTableModel;

class EventView : public Controller,
                  public ContentsModel,
                  public TimeModel
#if defined(UI_VIEWS)
    ,
                  protected views::TableController
#endif
{
 public:
  EventView(const ControllerContext& context, bool is_panel);
  virtual ~EventView();

  bool CanAcknowledgeSelection() const;
  void AcknowledgeSelection();

  // Controller
  virtual bool IsWorking() const override;
  virtual UiView* Init(const WindowDefinition& definition) override;
  virtual void Save(WindowDefinition& definition) override;
  virtual CommandHandler* GetCommandHandler(unsigned command_id) override;
  virtual bool IsCommandChecked(unsigned command) const override;
  virtual bool IsCommandEnabled(unsigned command) const override;
  virtual void ExecuteCommand(unsigned command) override;
  virtual ContentsModel* GetContentsModel() override { return this; }
  virtual TimeModel* GetTimeModel() override;

  // ContentsModel
  virtual void AddContainedItem(const scada::NodeId& node_id,
                                unsigned flags) override;
  virtual void RemoveContainedItem(const scada::NodeId& node_id) override;
  virtual NodeIdSet GetContainedItems() const override;

  // TimeModel
  virtual TimeRange GetTimeRange() const override;
  virtual void SetTimeRange(const TimeRange& time_range) override;

 protected:
#if defined(UI_VIEWS)
  // views::TableController overrides
  virtual void OnSelectionChanged(views::TableView& sender) override;
  virtual bool OnDoubleClick() override;
  virtual bool OnKeyPressed(views::TableView& sender,
                            ui::KeyboardCode key_code) override;
#endif

 private:
  base::string16 MakeTitle() const;

  void ExportToExcel();
  void SelectSeverity();

  NodeIdSet EventView::GetSelectedNodeIds() const;

  void OnSelectionChanged();

  bool is_panel_;

  std::unique_ptr<EventTableModel> model_;
  std::unique_ptr<Table> table_;

#if defined(UI_VIEWS)
  // TODO: Use gfx image list.
  std::unique_ptr<WTL::CImageList> severities_image_list_;
#endif
};
