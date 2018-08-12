#include "components/excel_report/views/excel_report_view.h"

#include "controller_factory.h"
#include "ui/views/controls/activex_control.h"
#include "views/activex_host.h"

// ExcelView

class ExcelView : public views::ActiveXControl {
 public:
  using ActiveXControl::ActiveXControl;

 protected:
  // NativeControl
  virtual void NativeControlCreated(HWND window_handle);
};

void ExcelView::NativeControlCreated(HWND window_handle) {
  __super::NativeControlCreated(window_handle);

  CreateControl(OLESTR("{00024500-0000-0000-C000-000000000046}"));
}

// ExcelReportView

const WindowInfo kWindowInfo = {
    ID_EXCEL_REPORT_VIEW, "ExcelReport", L"Отчет", WIN_CAN_PRINT, 0, 0, 0};

REGISTER_CONTROLLER(ExcelReportView, kWindowInfo);

ExcelReportView::ExcelReportView(const ControllerContext& context)
    : Controller{context}, excel_(new ExcelView{ActiveXHost::instance()}) {}

ExcelReportView::~ExcelReportView() {}

views::View* ExcelReportView::Init(const WindowDefinition& definition) {
  return excel_.get();
}
