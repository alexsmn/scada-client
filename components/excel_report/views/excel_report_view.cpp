#include "components/excel_report/views/excel_report_view.h"

#include "controller_factory.h"
#include "views/client_application_views.h"
#include "ui/views/controls/activex_control.h"

// ExcelView

class ExcelView : public views::ActiveXControl {
 public:
  ExcelView() : ActiveXControl(*g_application_views) {}

 protected:
  // NativeControl
  virtual void NativeControlCreated(HWND window_handle);
};

void ExcelView::NativeControlCreated(HWND window_handle) {
  __super::NativeControlCreated(window_handle);

  CreateControl(OLESTR("{00024500-0000-0000-C000-000000000046}"));
}

// ExcelReportView

REGISTER_CONTROLLER(ExcelReportView, ID_EXCEL_REPORT_VIEW);

ExcelReportView::ExcelReportView(const ControllerContext& context)
    : Controller(context),
      excel_(new ExcelView) {
}

ExcelReportView::~ExcelReportView() {
}

views::View* ExcelReportView::Init(const WindowDefinition& definition) {
  return excel_.get();
}