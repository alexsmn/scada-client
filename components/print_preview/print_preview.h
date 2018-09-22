#pragma once

#include <functional>

class DialogService;
class PrintService;

using PrintHandler = std::function<void()>;

void ShowPrintPreviewDialog(DialogService& dialog_service,
                            PrintService& print_service,
                            const PrintHandler& print_handler);
