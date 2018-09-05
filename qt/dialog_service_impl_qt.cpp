#include "qt/dialog_service_impl_qt.h"

#include <QFileDialog>
#include <QMessageBox>

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
    base::StringPiece16 title,
    const std::filesystem::path& default_path) {
  return QFileDialog::getSaveFileName(
             parent_widget, QString::fromWCharArray(title.data(), title.size()),
             QString::fromStdWString(default_path.wstring()))
      .toStdWString();
}
