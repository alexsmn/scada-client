#include "base/threading/thread_task_runner_handle.h"
#include "client_paths.h"
#include "common_resources.h"
#include "components/vidicon_display/vidicon_display_component.h"
#include "components/vidicon_display/vidicon_display_view.h"
#include "qt/message_loop_qt.h"
#include "window_definition.h"
#include "window_info.h"

#include <QApplication>
#include <QToolBar>
#include <QVBoxLayout>
#include <QWidget>

QWidget* CreateVidiconDisplayView(QWidget& root_widget) {
  VidiconDisplayView* vidicon_display_view = new VidiconDisplayView;
  WindowDefinition definition{kVidiconDisplayWindowInfo};
  definition.path = R"(c:\ProgramData\Telecontrol\SCADA Client\PS-110.vds)";
  auto* widget = vidicon_display_view->Init(definition);
  widget->setParent(&root_widget);
  QObject::connect(widget, &QObject::destroyed,
                   [vidicon_display_view] { delete vidicon_display_view; });
  root_widget.layout()->addWidget(widget);
  return widget;
}

int main(int argc, char* argv[]) {
  client::RegisterPathProvider();

  QApplication qapp(argc, argv);

  base::ThreadTaskRunnerHandle message_loop{new MessageLoopQt};

  QWidget root_widget;
  root_widget.setLayout(new QVBoxLayout);

  QWidget* opened_vidicon_display_view = nullptr;

  QToolBar* toolbar = new QToolBar{&root_widget};
  toolbar->setMaximumHeight(20);
  toolbar->addAction("Open", [&root_widget, &opened_vidicon_display_view] {
    opened_vidicon_display_view = CreateVidiconDisplayView(root_widget);
  });
  toolbar->addAction("Close", [&opened_vidicon_display_view] {
    opened_vidicon_display_view->deleteLater();
    opened_vidicon_display_view = nullptr;
  });
  root_widget.layout()->addWidget(toolbar);

  root_widget.resize(1000, 500);
  root_widget.show();

  return QApplication::exec();
}