#include "aui/qt/dialog_service_impl_qt.h"

#include "base/strings/string_util.h"
#include "aui/qt/dialog_util.h"

#include <QAbstractButton>
#include <QFileDialog>
#include <QMessageBox>

namespace {

std::string JoinStrings(base::span<const std::string_view> strings,
                        char separator) {
  if (strings.empty())
    return {};

  auto result = std::string{strings[0]};
  for (size_t i = 1; i < strings.size(); ++i) {
    result += separator;
    result.append(strings[i].data(), strings[i].size());
  }
  return result;
}

QString MakeFilter(const DialogService::Filter& filter) {
  assert(!filter.extensions.empty());

  QString result = QString::fromUtf16(filter.title.data(), filter.title.size());
  result += " (";
  result += QString::fromStdString(JoinStrings(filter.extensions, ' '));
  result += ')';

  return result;
}

QString MakeFilter(base::span<const DialogService::Filter> filters) {
  if (filters.empty())
    return {};

  QString result = MakeFilter(filters[0]);
  for (auto i = std::next(filters.begin()); i != filters.end(); ++i) {
    result += ";;";
    result += MakeFilter(*i);
  }

  return result;
}

MessageBoxResult MapQtMesageBoxResult(int result) {
  switch (result) {
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

}  // namespace

promise<MessageBoxResult> DialogServiceImplQt::RunMessageBox(
    std::u16string_view message,
    std::u16string_view title,
    MessageBoxMode mode) {
  auto message_box = std::make_unique<QMessageBox>(parent_widget);
  message_box->setModal(true);
  message_box->setText(QString::fromUtf16(message.data(), message.size()));
  message_box->setWindowTitle(QString::fromUtf16(title.data(), title.size()));

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

    default:
      assert(false);
      break;
  }

  if (mode == MessageBoxMode::QuestionYesNoDefaultNo)
    message_box->setDefaultButton(QMessageBox::No);

  promise<MessageBoxResult> promise;
  QObject::connect(
      message_box.get(), &QMessageBox::finished,
      [promise, message_box = message_box.get()](int result) mutable {
        promise.resolve(MapQtMesageBoxResult(result));
        message_box->deleteLater();
      });

  message_box.release()->show();
  return promise;
}

UiView* DialogServiceImplQt::GetDialogOwningWindow() const {
  return nullptr;
}

UiView* DialogServiceImplQt::GetParentWidget() const {
  return parent_widget;
}

promise<std::filesystem::path> DialogServiceImplQt::SelectOpenFile(
    std::u16string_view title) {
  auto caption = QString::fromUtf16(title.data(), title.size());
  auto dialog =
      std::make_unique<QFileDialog>(parent_widget, std::move(caption));
  dialog->setAcceptMode(QFileDialog::AcceptOpen);
  return StartModalDialog(std::move(dialog)).then([](QFileDialog* dialog) {
    auto files = dialog->selectedFiles();
    if (files.size() != 1) {
      // Unexpected.
      throw std::exception{};
    }
    return std::filesystem::path{files.at(0).toStdU16String()};
  });
}

promise<std::filesystem::path> DialogServiceImplQt::SelectSaveFile(
    const SaveParams& params) {
  auto caption = QString::fromUtf16(params.title.data(), params.title.size());

  auto dialog =
      std::make_unique<QFileDialog>(parent_widget, std::move(caption));
  dialog->setAcceptMode(QFileDialog::AcceptSave);
  dialog->setFileMode(QFileDialog::AnyFile);
  dialog->setDirectory(
      QString::fromStdU16String(params.default_path.u16string()));
  dialog->selectNameFilter(MakeFilter(params.filters));

  return StartModalDialog(std::move(dialog)).then([](QFileDialog* dialog) {
    auto files = dialog->selectedFiles();
    if (files.size() != 1) {
      // Unexpected.
      throw std::exception{};
    }
    return std::filesystem::path{files.at(0).toStdU16String()};
  });
}
