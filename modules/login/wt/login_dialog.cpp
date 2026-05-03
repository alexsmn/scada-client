#include "modules/login/wt/login_dialog.h"

#include "modules/login/login_controller.h"
#include "aui/dialog_service.h"
#include "base/callback_awaitable.h"
#include "net/net_executor_adapter.h"

#include <Wt/WDialog.h>
#include <Wt/WLabel.h>
#include <Wt/WLineEdit.h>
#include <Wt/WMessageBox.h>
#include <Wt/WPushButton.h>

#include <memory>
#include <optional>
#include <string>
#include <type_traits>
#include <utility>

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
  TestDialogService(AnyExecutor executor, Wt::WWidget& parent)
      : executor_{std::move(executor)}, parent_{parent} {}

  virtual UiView* GetDialogOwningWindow() const override { return nullptr; }

  virtual UiView* GetParentWidget() const override { return nullptr; }

  virtual Awaitable<MessageBoxResult> RunMessageBox(
      std::u16string_view message,
      std::u16string_view title,
      MessageBoxMode mode) override {
    return RunMessageBoxAsync(std::u16string{message}, std::u16string{title},
                              mode);
  }

  virtual Awaitable<std::filesystem::path> SelectOpenFile(
      std::u16string_view title) override {
    throw std::exception{};
  }

  virtual Awaitable<std::filesystem::path> SelectSaveFile(
      const SaveParams& params) override {
    throw std::exception{};
  }

 private:
  Awaitable<MessageBoxResult> RunMessageBoxAsync(std::u16string message,
                                                 std::u16string title,
                                                 MessageBoxMode mode) {
    auto actual_title = title.empty() ? u"Title" : std::move(title);
    auto message_box = parent_.addChild(std::make_unique<Wt::WMessageBox>(
        std::move(actual_title), std::move(message), ToWtIcon(mode),
        ToStandardButtons(mode)));

    message_box->show();

    auto [button] = co_await CallbackToAwaitable<Wt::StandardButton>(
        executor_, [message_box](auto callback) mutable {
          auto completion =
              std::make_shared<std::decay_t<decltype(callback)>>(
                  std::move(callback));
          message_box->buttonClicked().connect(
              [message_box, completion]() mutable {
                (*completion)(message_box->buttonResult());
              });
        });
    parent_.removeChild(message_box);
    co_return ToMessageBoxResult(button);
  }

  AnyExecutor executor_;
  Wt::WWidget& parent_;
};

class LoginDialog : public std::enable_shared_from_this<LoginDialog> {
 public:
  LoginDialog(AnyExecutor task_runner,
              Wt::WWidget& parent,
              DataServicesContext services_context)
      : executor_{std::move(task_runner)},
        parent_{parent},
        controller_{
            std::make_shared<LoginController>(executor_,
                                              std::move(services_context),
                                              dialog_service_)} {
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

  Awaitable<std::optional<DataServices>> RunAsync() {
    controller_->completion_handler =
        [this, ref = shared_from_this()](DataServices services) {
          result_ = std::move(services);
          dialog_->accept();
        };
    controller_->error_handler = [this, ref = shared_from_this()] {
      EnableControls(true);
    };

    dialog_->show();

    co_await CallbackToAwaitable<>(
        executor_, [dialog = dialog_](auto callback) {
          auto completion =
              std::make_shared<std::decay_t<decltype(callback)>>(
                  std::move(callback));
          dialog->finished().connect([completion]() mutable {
            (*completion)();
          });
        });

    parent_.removeChild(dialog_);
    co_return std::move(result_);
  }

 private:
  void EnableControls(bool enable) {
    if (enable)
      dialog_->enable();
    else
      dialog_->disable();
  }

  AnyExecutor executor_;
  Wt::WWidget& parent_;
  std::optional<DataServices> result_;

  Wt::WDialog* dialog_ =
      parent_.addChild(std::make_unique<Wt::WDialog>("Login"));

  TestDialogService dialog_service_{executor_, *dialog_};
  const std::shared_ptr<LoginController> controller_;
};

Awaitable<std::optional<DataServices>> RunLoginDialogAsync(
    std::shared_ptr<LoginDialog> login_dialog) {
  co_return co_await login_dialog->RunAsync();
}

Awaitable<std::optional<DataServices>> ExecuteLoginDialog(
    AnyExecutor executor,
    Wt::WWidget& parent,
    DataServicesContext&& services_context) {
  auto login_dialog = std::make_shared<LoginDialog>(
      executor, parent, std::move(services_context));
  co_return co_await RunLoginDialogAsync(std::move(login_dialog));
}
