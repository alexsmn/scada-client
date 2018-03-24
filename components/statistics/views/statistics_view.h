#pragma once

#include <memory>

#include "base/timer/timer.h"
#include "controller.h"
#include "ui/base/models/table_model.h"

namespace views {
class TableView;
}

class StatisticsView : public Controller,
                       private ui::TableModel {
 public:
  explicit StatisticsView(const ControllerContext& context);

  // Controller
  virtual views::View* Init(const WindowDefinition& definition) override;
 
 private:
  void UpdateStatistics();

	// ui::TableModel events
  virtual int GetRowCount() override;
	virtual void GetCell(ui::TableCell& cell) override;

  std::unique_ptr<views::TableView> table_;

  base::RepeatingTimer update_timer_;
};
