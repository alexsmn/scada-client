#pragma once

#include <memory>

#include "controller.h"

class ExcelView;

class ExcelReportView : public Controller {
 public:
  explicit ExcelReportView(const ControllerContext& context);
  virtual ~ExcelReportView();

  // Controller
  virtual views::View* Init(const WindowDefinition& definition) override;

 private:
  std::unique_ptr<ExcelView> excel_;
};
