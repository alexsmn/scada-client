#include "components/debugger/qt/debugger_qt.h"

#include "aui/table.h"
#include "resources/common_resources.h"
#include "components/debugger/debugger_context.h"
#include "components/debugger/request_table_model.h"
#include "controller/command_registry.h"
#include "scada/session_service.h"

#include <QHBoxLayout>
#include <QSplitter>
#include <QTabWidget>
#include <QTextEdit>
#include <QWidget>
#include <format>

// Deubgger

Debugger::Debugger(DebuggerContext&& context)
    : DebuggerContext{std::move(context)} {
  request_table_model_ = std::make_shared<RequestTableModel>(session_service_);
}

void Debugger::Open() {
  QTabWidget* window = new QTabWidget;
  window->setWindowTitle("Debugger");
  window->addTab(CreateRequestView(window), "Requests");

  QObject::connect(window, &QWidget::close, &QObject::deleteLater);
  window->show();
}

QWidget* Debugger::CreateRequestView(QWidget* parent) {
  QSplitter* splitter = new QSplitter{parent};
  splitter->setOrientation(Qt::Horizontal);

  std::vector<aui::TableColumn> request_table_columns{
      {.id = 0, .title = u"ID", .alignment = aui::TableColumn::RIGHT},
      {.id = 1, .title = u"Phase", .alignment = aui::TableColumn::LEFT},
      {.id = 2, .title = u"Start Time", .alignment = aui::TableColumn::LEFT},
      {.id = 3, .title = u"Duration", .alignment = aui::TableColumn::LEFT},
      {.id = 4,
       .title = u"Title",
       .width = 200,
       .alignment = aui::TableColumn::LEFT}};

  aui::Table* request_table_view =
      new aui::Table{request_table_model_, std::move(request_table_columns)};

  request_table_view->setParent(parent);
  splitter->addWidget(request_table_view);

  QTextEdit* request_dump_view = new QTextEdit{parent};
  splitter->addWidget(request_dump_view);

  request_table_view->SetSelectionChangeHandler(
      [request_table_model = request_table_model_, request_table_view,
       request_dump_view] {
        if (int index = request_table_view->GetCurrentRow(); index != -1) {
          const RequestTableModel::Request& request =
              request_table_model->request(index);
          const std::string& dump = DumpRequest(request);
          request_dump_view->setText(QString::fromStdString(dump));
        }
      });

  return splitter;
}
