#include "components/excel_report/views/excel_report_view.h"
#include "controller_factory.h"

const WindowInfo kWindowInfo = {
    ID_EXCEL_REPORT_VIEW, "ExcelReport", L"Отчет", 0, 0, 0, 0};

REGISTER_CONTROLLER(ExcelReportView, kWindowInfo);
