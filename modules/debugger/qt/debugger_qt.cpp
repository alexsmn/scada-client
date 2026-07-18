#include "modules/debugger/qt/debugger_qt.h"

#include "aui/qt/theme_qt.h"
#include "aui/severity_colors.h"
#include "aui/table.h"
#include "aui/translation.h"
#include "resources/common_resources.h"
#include "modules/debugger/debugger_context.h"
#include "modules/debugger/request_table_model.h"
#include "controller/command_registry.h"
#include "scada/session_service.h"

#include <QFileDialog>
#include <QHBoxLayout>
#include <QLineEdit>
#include <QPushButton>
#include <QSplitter>
#include <QTabWidget>
#include <QTextEdit>
#include <QVBoxLayout>
#include <QWidget>
#include <format>
#include <fstream>

namespace {

// The design tokens for the active reshell theme. The toolbar is only built
// under a token theme (CreateRequestView gates on it).
const scada::aui::ThemeTokens& DebuggerTokens() {
  scada::aui::Theme theme = scada::aui::Theme::kDark;
  switch (scada::aui::GetSeverityTheme()) {
    case scada::aui::SeverityTheme::kLight:
      theme = scada::aui::Theme::kLight;
      break;
    case scada::aui::SeverityTheme::kHighContrast:
      theme = scada::aui::Theme::kHighContrast;
      break;
    default:
      break;
  }
  return scada::aui::GetThemeTokens(theme);
}

QString Tr(std::string_view text) {
  return QString::fromStdU16String(Translate(text));
}

// Concatenated dump of every request currently shown (after the filter).
std::string DumpTrace(RequestTableModel& model) {
  std::string dump;
  for (int i = 0; i < model.GetRowCount(); ++i)
    dump += DumpRequest(model.request(i)) + "\n\n";
  return dump;
}

}  // namespace

// Debugger

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

  // Legacy look: the bare splitter, as before.
  if (scada::aui::GetSeverityTheme() == scada::aui::SeverityTheme::kLegacy)
    return splitter;

  // Reshell: a themed trace toolbar (Pause / Clear / filter / Save) above the
  // frame trace + detail, matching client/docs/ui-mockups/screens/debugger.html.
  const scada::aui::ThemeTokens& tokens = DebuggerTokens();
  auto* container = new QWidget{parent};
  container->setObjectName(QStringLiteral("debuggerView"));
  container->setStyleSheet(QStringLiteral("#debuggerView{background:%1;}")
                               .arg(tokens.bg.name()));
  auto* layout = new QVBoxLayout{container};
  layout->setContentsMargins(0, 0, 0, 0);
  layout->setSpacing(0);

  auto* toolbar = new QWidget;
  toolbar->setObjectName(QStringLiteral("debuggerToolbar"));
  toolbar->setStyleSheet(
      QStringLiteral("#debuggerToolbar{background:%1;border-bottom:1px solid "
                     "%2;}")
          .arg(tokens.bg_elevated.name(), tokens.border.name()));
  auto* bar = new QHBoxLayout{toolbar};
  bar->setContentsMargins(10, 5, 10, 5);
  bar->setSpacing(8);

  const QString button_style =
      QStringLiteral("QPushButton{background:%1;color:%2;border:1px solid %3;"
                     "border-radius:4px;padding:4px 12px;}")
          .arg(tokens.surface_muted.name(), tokens.fg.name(),
               tokens.border_strong.name());

  RequestTableModel* model = request_table_model_.get();

  auto* pause = new QPushButton{Tr("Pause")};
  pause->setObjectName(QStringLiteral("debuggerPause"));
  pause->setStyleSheet(button_style);
  QObject::connect(pause, &QPushButton::clicked, pause, [model, pause] {
    const bool paused = !model->paused();
    model->SetPaused(paused);
    pause->setText(paused ? QString::fromStdU16String(Translate("Resume"))
                          : QString::fromStdU16String(Translate("Pause")));
  });
  bar->addWidget(pause);

  auto* clear = new QPushButton{Tr("Clear")};
  clear->setStyleSheet(button_style);
  QObject::connect(clear, &QPushButton::clicked, clear,
                   [model, request_dump_view] {
                     model->Clear();
                     request_dump_view->clear();
                   });
  bar->addWidget(clear);

  auto* filter = new QLineEdit;
  filter->setObjectName(QStringLiteral("debuggerFilter"));
  filter->setClearButtonEnabled(true);
  filter->setPlaceholderText(Tr("Filter title or id"));
  filter->setStyleSheet(
      QStringLiteral("QLineEdit{background:%1;border:1px solid %2;"
                     "border-radius:4px;padding:4px 8px;color:%3;}")
          .arg(tokens.surface_muted.name(), tokens.border.name(),
               tokens.fg.name()));
  QObject::connect(filter, &QLineEdit::textChanged, filter,
                   [model](const QString& text) {
                     model->SetFilter(text.toStdU16String());
                   });
  bar->addWidget(filter, 1);

  auto* save = new QPushButton{Tr("Save trace")};
  save->setStyleSheet(button_style);
  QObject::connect(save, &QPushButton::clicked, save, [model, container] {
    const QString path = QFileDialog::getSaveFileName(
        container, QString::fromStdU16String(Translate("Save trace")),
        QStringLiteral("trace.txt"));
    if (path.isEmpty())
      return;
    std::ofstream file{path.toStdString()};
    file << DumpTrace(*model);
  });
  bar->addWidget(save);

  layout->addWidget(toolbar);
  layout->addWidget(splitter, 1);
  return container;
}
