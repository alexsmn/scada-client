#pragma once

#include "services/dialog_service.h"

#include <QMessageBox>

class DialogServiceImplQt final : public DialogService {
 public:
  // DialogService
  virtual gfx::NativeView GetDialogOwningWindow() const override;
  virtual MessageBoxResult RunMessageBox(base::StringPiece16 message,
                                         base::StringPiece16 title,
                                         MessageBoxMode mode) override;

  QWidget* parent_widget = nullptr;
};

inline MessageBoxResult DialogServiceImplQt::RunMessageBox(
    base::StringPiece16 message,
    base::StringPiece16 title,
    MessageBoxMode mode) {
  auto* message_box = new QMessageBox{parent_widget};
  message_box->setText(QString::fromWCharArray(message.data(), message.size()));
  message_box->setWindowTitle(QString::fromWCharArray(title.data(), title.size()));

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

inline gfx::NativeView DialogServiceImplQt::GetDialogOwningWindow() const {
  return nullptr;
}
