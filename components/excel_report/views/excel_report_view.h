#pragma once

#include "controller.h"
#include "controller_context.h"

#include <memory>

class ExcelView;

class ExcelReportView : protected ControllerContext, public Controller {
 public:
  explicit ExcelReportView(const ControllerContext& context);
  virtual ~ExcelReportView();

  // Controller
  virtual views::View* Init(const WindowDefinition& definition) override;

 private:
  std::unique_ptr<ExcelView> excel_;
};
