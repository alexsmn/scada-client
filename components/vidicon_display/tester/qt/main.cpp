#include <QApplication>

#include "common_resources.h"
#include "components/vidicon_display/qt/vidicon_display_view.h"
#include "window_definition.h"
#include "window_info.h"

int main(int argc, char* argv[]) {
  QApplication qapp(argc, argv);

  VidiconDisplayView vidicon_display_view;

  WindowDefinition definition{GetWindowInfo(ID_VIDICON_DISPLAY_VIEW)};
  auto* widget = vidicon_display_view.Init(definition);
  widget->show();

  return qapp.exec();
}