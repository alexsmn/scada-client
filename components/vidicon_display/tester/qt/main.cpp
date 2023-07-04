#include "base/test/test_executor.h"
#include "base/threading/thread_task_runner_handle.h"
#include "client_paths.h"
#include "common_resources.h"
#include "components/vidicon_display/tester/qt/variable_timed_data_service.h"
#include "components/vidicon_display/vidicon_display_component.h"
#include "components/vidicon_display/vidicon_display_view.h"
#include "qt/message_loop_qt.h"
#include "services/vidicon/vidicon_client.h"
#include "window_definition.h"
#include "window_info.h"

#include <QApplication>
#include <QSplitter>
#include <QTableWidget>
#include <QToolBar>
#include <QVBoxLayout>
#include <QWidget>

struct State {
  std::shared_ptr<Executor> executor = std::make_shared<TestExecutor>();
  VariableStorage variable_storage;
  VariableTimedDataService timed_data_service{variable_storage};
  vidicon::VidiconClient vidicon_client{
      {.executor_ = executor, .timed_data_service_ = timed_data_service}};
};

QWidget* CreateVidiconDisplayView(QSplitter& splitter, State& state) {
  VidiconDisplayView* vidicon_display_view =
      new VidiconDisplayView{state.vidicon_client};
  WindowDefinition definition{kVidiconDisplayWindowInfo};
  // definition.path = R"(c:\ProgramData\Telecontrol\SCADA Client\PS-110.vds)";
  definition.path = R"(c:\ProgramData\Telecontrol\SCADA Client\по-258.vds)";
  auto* widget = vidicon_display_view->Init(definition);
  widget->setParent(&splitter);
  QObject::connect(widget, &QObject::destroyed,
                   [vidicon_display_view] { delete vidicon_display_view; });
  splitter.addWidget(widget);
  return widget;
}

int main(int argc, char* argv[]) {
  client::RegisterPathProvider();

  QApplication qapp(argc, argv);
  base::ThreadTaskRunnerHandle message_loop{new MessageLoopQt};
  State state;

  QWidget root_widget;
  root_widget.setLayout(new QVBoxLayout);

  QWidget* opened_vidicon_display_view = nullptr;

  QToolBar* toolbar = new QToolBar{&root_widget};
  toolbar->setMaximumHeight(20);
  root_widget.layout()->addWidget(toolbar);

  auto* splitter = new QSplitter{&root_widget};
  splitter->setOrientation(Qt::Horizontal);
  root_widget.layout()->addWidget(splitter);

  auto* variable_table = new QTableWidget{&root_widget};
  variable_table->setColumnCount(2);
  variable_table->setHorizontalHeaderLabels({"Name", "Value"});
  QObject::connect(
      variable_table, &QTableWidget::itemChanged, [&](QTableWidgetItem* item) {
        if (item->column() == 1) {
          auto name =
              variable_table->item(item->row(), 0)->text().toStdString();
          auto new_value = item->text().toInt();
          state.variable_storage.SetVariableValue(name, new_value);
        }
      });
  splitter->addWidget(variable_table);

  toolbar->addAction("Open", [&] {
    opened_vidicon_display_view = CreateVidiconDisplayView(*splitter, state);
  });

  toolbar->addAction("Close", [&opened_vidicon_display_view] {
    opened_vidicon_display_view->deleteLater();
    opened_vidicon_display_view = nullptr;
  });

  toolbar->addAction("Variables", [&] {
    variable_table->clear();

    int index = 0;
    for (const auto& [name, data_value] :
         state.variable_storage.variable_values) {
      variable_table->insertRow(index);
      variable_table->setItem(
          index, 0, new QTableWidgetItem{QString::fromStdString(name)});
      variable_table->setItem(index, 1,
                              new QTableWidgetItem{QString::fromStdString(
                                  ToString(data_value.value))});
      index++;
    }
  });

  root_widget.resize(1000, 500);
  root_widget.show();

  return QApplication::exec();
}