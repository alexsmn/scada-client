#include "components/debugger/qt/debugger_qt.h"

#include "aui/table.h"
#include "controller/command_registry.h"
#include "common_resources.h"
#include "components/debugger/debugger_context.h"
#include "components/debugger/request_table_model.h"
#include "scada/session_service.h"

#include <QHBoxLayout>
#include <QSplitter>
#include <QTabWidget>
#include <QTextEdit>
#include <QWidget>
#include <format>

Debugger::Debugger(DebuggerContext&& context)
    : DebuggerContext{std::move(context)} {}

void Debugger::RegisterCommands(CommandRegistry& main_commands) {
  main_commands.AddCommand(
      Command{ID_OPEN_DEBUGGER}.set_execute_handler([this] { Open(); }));
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

  auto request_table_model =
      std::make_shared<RequestTableModel>(session_service_);

  std::vector<aui::TableColumn> request_table_columns{
      {.id = 0, .title = u"ID", .alignment = aui::TableColumn::RIGHT},
      {.id = 1, .title = u"Phase", .alignment = aui::TableColumn::LEFT},
      {.id = 2,
       .title = u"Title",
       .width = 200,
       .alignment = aui::TableColumn::LEFT}};

  aui::Table* request_table_view =
      new aui::Table{request_table_model, std::move(request_table_columns)};
  request_table_view->setParent(parent);
  splitter->addWidget(request_table_view);

  QTextEdit* text_edit = new QTextEdit{parent};
  splitter->addWidget(text_edit);

  request_table_view->SetSelectionChangeHandler([request_table_model,
                                                 request_table_view,
                                                 text_edit] {
    if (int index = request_table_view->GetCurrentRow(); index != -1) {
      const auto& request = request_table_model->request(index);
      text_edit->setText(QString::fromStdString(std::format(
          "Request:\n{}\nResponse:\n{}", request.body, request.response_body)));
    }
  });

  return splitter;
}
