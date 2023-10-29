#pragma once

#if defined(UI_QT)
#include <QPrinter>
#endif

class PrintService {
 public:
#if defined(UI_QT)
  QPrinter printer{QPrinter::HighResolution};
#endif
};
