#include "qt/dialog_service_impl_qt.h"

#include "base/strings/string_util.h"

#include <QFileDialog>
#include <QMessageBox>

namespace {

std::string JoinStrings(span<const base::StringPiece> strings, char separator) {
  if (strings.empty())
    return {};

  std::string result = strings[0].as_string();
  for (size_t i = 1; i < strings.size(); ++i) {
    result += separator;
    result.append(strings[i].data(), strings[i].size());
  }
  return result;
}

QString MakeFilter(const DialogService::Filter& filter) {
  assert(!filter.extensions.empty());

  QString result =
      QString::fromWCharArray(filter.title.data(), filter.title.size());
  result += " (";
  result += QString::fromStdString(JoinStrings(filter.extensions, ' '));
  result += ')';

  return result;
}

QString MakeFilter(span<const DialogService::Filter> filters) {
  if (filters.empty())
    return {};

  QString result = MakeFilter(filters.front());
  for (auto i = std::next(filters.begin()); i != filters.end(); ++i) {
    result += ";;";
    result += MakeFilter(*i);
  }

  return result;
}

}  // namespace

MessageBoxResult DialogServiceImplQt::RunMessageBox(base::StringPiece16 message,
                                                    base::StringPiece16 title,
                                                    MessageBoxMode mode) {
  auto* message_box = new QMessageBox{parent_widget};
  message_box->setText(QString::fromWCharArray(message.data(), message.size()));
  message_box->setWindowTitle(
      QString::fromWCharArray(title.data(), title.size()));

  switch (mode) {
    case MessageBoxMode::Info:
      message_box->setIcon(QMessageBox::Information);
      break;

    case MessageBoxMode::Error:
      message_box->setIcon(QMessageBox::Critical);
      break;

    case MessageBoxMode::QuestionYesNo:
    case MessageBoxMode::QuestionYesNoDefaultNo:
      message_box->setIcon(QMessageBox::Question);
      message_box->setStandardButtons(QMessageBox::Yes | QMessageBox::No);
      break;
  }

  if (mode == MessageBoxMode::QuestionYesNoDefaultNo)
    message_box->setDefaultButton(QMessageBox::No);

  auto button = message_box->exec();

  switch (button) {
    case QMessageBox::Yes:
      return MessageBoxResult::Yes;
    case QMessageBox::No:
      return MessageBoxResult::No;
    case QMessageBox::Ok:
      return MessageBoxResult::Ok;
    case QMessageBox::Cancel:
      return MessageBoxResult::Cancel;
    default:
      return MessageBoxResult::Ok;
  }
}

gfx::NativeView DialogServiceImplQt::GetDialogOwningWindow() const {
  return nullptr;
}

QWidget* DialogServiceImplQt::GetParentWidget() const {
  return parent_widget;
}

std::filesystem::path DialogServiceImplQt::SelectOpenFile(
    base::StringPiece16 title) {
  return QFileDialog::getOpenFileName(
             parent_widget, QString::fromWCharArray(title.data(), title.size()))
      .toStdWString();
}

std::filesystem::path DialogServiceImplQt::SelectSaveFile(
    const SaveParams& params) {
  return QFileDialog::getSaveFileName(
             parent_widget,
             QString::fromWCharArray(params.title.data(), params.title.size()),
             QString::fromStdWString(params.default_path.wstring()),
             MakeFilter(params.filters))
      .toStdWString();
}
