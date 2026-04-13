#pragma once

#include <functional>

#if defined(UI_QT)
#include <QPrinter>
#endif

class DialogService;

using PrintHandler = std::function<void()>;

class PrintService {
 public:
  void ShowPrintPreviewDialog(DialogService& dialog_service,
                              const PrintHandler& print_handler);

#if defined(UI_QT)
  QPrinter printer{QPrinter::HighResolution};
#endif
};
