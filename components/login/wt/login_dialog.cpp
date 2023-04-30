#include "components/login/wt/login_dialog.h"

#include "components/login/login_controller.h"
#include "services/dialog_service.h"

#include <Wt/WDialog.h>
#include <Wt/WLabel.h>
#include <Wt/WLineEdit.h>
#include <Wt/WMessageBox.h>
#include <Wt/WPushButton.h>

namespace {

MessageBoxResult ToMessageBoxResult(Wt::StandardButton button) {
  switch (button) {
    case Wt::StandardButton::Ok:
      return MessageBoxResult::Ok;
    case Wt::StandardButton::Yes:
      return MessageBoxResult::Yes;
    case Wt::StandardButton::No:
      return MessageBoxResult::No;
    default:
      assert(false);
      return MessageBoxResult::Cancel;
  }
}

Wt::Icon ToWtIcon(MessageBoxMode mode) {
  switch (mode) {
    case MessageBoxMode::Info:
      return Wt::Icon::Information;
    case MessageBoxMode::Error:
      return Wt::Icon::Critical;
    case MessageBoxMode::QuestionYesNo:
    case MessageBoxMode::QuestionYesNoDefaultNo:
      return Wt::Icon::Question;
    default:
      return Wt::Icon::None;
  }
}

Wt::WFlags<Wt::StandardButton> ToStandardButtons(MessageBoxMode mode) {
  switch (mode) {
    case MessageBoxMode::QuestionYesNo:
    case MessageBoxMode::QuestionYesNoDefaultNo:
      return Wt::StandardButton::Yes | Wt::StandardButton::No;
    default:
      return Wt::StandardButton::Ok;
  }
}

}  // namespace

class TestDialogService : public DialogService {
 public:
  explicit TestDialogService(Wt::WWidget& parent) : parent_{parent} {}

  virtual UiView* GetDialogOwningWindow() const override { return nullptr; }

  virtual UiView* GetParentWidget() const override { return nullptr; }

  virtual promise<MessageBoxResult> RunMessageBox(
      std::u16string_view message,
      std::u16string_view title,
      MessageBoxMode mode) override {
    auto promise = make_promise<MessageBoxResult>();

    auto actual_title = title.empty() ? u"Title" : std::u16string{title};
    auto message_box = parent_.addChild(std::make_unique<Wt::WMessageBox>(
        actual_title, std::u16string{message}, ToWtIcon(mode),
        ToStandardButtons(mode)));

    message_box->buttonClicked().connect(
        [this, message_box, promise]() mutable {
          auto button = message_box->buttonResult();
          parent_.removeChild(message_box);
          promise.resolve(ToMessageBoxResult(button));
        });

    message_box->show();

    return promise;
  }

  virtual promise<std::filesystem::path> SelectOpenFile(
      std::u16string_view title) override {
    return {};
  }

  virtual promise<std::filesystem::path> SelectSaveFile(
      const SaveParams& params) override {
    return {};
  }

 private:
  Wt::WWidget& parent_;
};

class LoginDialog : public std::enable_shared_from_this<LoginDialog> {
 public:
  LoginDialog(std::shared_ptr<Executor> task_runner,
              Wt::WWidget& parent,
              DataServicesContext services_context,
              promise<std::optional<DataServices>> login_promise)
      : parent_{parent},
        controller_{
            std::make_shared<LoginController>(std::move(task_runner),
                                              std::move(services_context),
                                              dialog_service_)},
        login_promise_{std::move(login_promise)} {
    Wt::WLabel* user_name_label =
        dialog_->contents()->addNew<Wt::WLabel>("User Name:");
    Wt::WLineEdit* user_name_edit =
        dialog_->contents()->addNew<Wt::WLineEdit>();
    user_name_edit->setText(controller_->user_name);
    user_name_label->setBuddy(user_name_edit);

    Wt::WLabel* password_label =
        dialog_->contents()->addNew<Wt::WLabel>("Password:");
    Wt::WLineEdit* password_edit = dialog_->contents()->addNew<Wt::WLineEdit>();
    password_edit->setText(controller_->password);
    password_label->setBuddy(password_edit);

    dialog_->contents()->addStyleClass("form-group");

    Wt::WPushButton* ok = dialog_->footer()->addNew<Wt::WPushButton>("OK");
    ok->setDefault(true);

    Wt::WPushButton* cancel =
        dialog_->footer()->addNew<Wt::WPushButton>("Cancel");
    dialog_->rejectWhenEscapePressed();

    ok->clicked().connect([=] {
      EnableControls(false);
      controller_->user_name = user_name_edit->text();
      controller_->password = password_edit->text();
      controller_->Login();
    });

    cancel->clicked().connect(dialog_, &Wt::WDialog::reject);
  }

  void Show() {
    controller_->completion_handler =
        [this, ref = shared_from_this()](DataServices services) {
          dialog_->accept();
          login_promise_.resolve(std::move(services));
        };
    controller_->error_handler = [this, ref = shared_from_this()] {
      EnableControls(true);
    };

    dialog_->finished().connect(
        [this, ref = shared_from_this()] { parent_.removeChild(dialog_); });

    dialog_->show();
  }

 private:
  void EnableControls(bool enable) {
    if (enable)
      dialog_->enable();
    else
      dialog_->disable();
  }

  Wt::WWidget& parent_;
  promise<std::optional<DataServices>> login_promise_;

  Wt::WDialog* dialog_ =
      parent_.addChild(std::make_unique<Wt::WDialog>("Login"));

  TestDialogService dialog_service_{*dialog_};
  const std::shared_ptr<LoginController> controller_;
};

promise<std::optional<DataServices>> ExecuteLoginDialog(
    std::shared_ptr<Executor> executor,
    Wt::WWidget& parent,
    DataServicesContext&& services_context) {
  auto promise = make_promise<std::optional<DataServices>>();
  auto login_dialog = std::make_shared<LoginDialog>(
      std::move(executor), parent, std::move(services_context), promise);
  login_dialog->Show();
  return promise;
}
