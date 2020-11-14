#include "views/dialog_service_impl_views.h"

#include "base/run_loop.h"
#include "base/win/win_util2.h"
#include "ui/base/dialogs/select_file_dialog.h"

#include <atlbase.h>

#include <atlapp.h>
#include <atluser.h>

namespace {

template <class Handler>
class FileSelector final : public ui::SelectFileDialog::Listener {
 public:
  explicit FileSelector(Handler&& handler)
      : handler_{std::forward<Handler>(handler)} {}

  virtual void FileSelected(const base::FilePath& path,
                            int index,
                            void* params) override {
    handler_(path);
    delete this;
  }

  virtual void FileSelectionCanceled(void* params) override {
    handler_({});
    delete this;
  }

 private:
  Handler handler_;
};

template <class Handler>
auto* CreateFileSelector(Handler&& handler) {
  return new FileSelector<Handler>{std::forward<Handler>(handler)};
}

}  // namespace

MessageBoxResult DialogServiceImplViews::RunMessageBox(
    base::StringPiece16 message,
    base::StringPiece16 title,
    MessageBoxMode mode) {
  const unsigned kFlags[] = {
      MB_ICONINFORMATION | MB_OK,
      MB_ICONSTOP | MB_OK,
      MB_ICONQUESTION | MB_YESNO,
      MB_ICONQUESTION | MB_YESNO | MB_DEFBUTTON2,
  };
  static_assert(std::size(kFlags) ==
                static_cast<std::size_t>(MessageBoxMode::Count));

  auto title_string = title.as_string();
  if (title_string.empty())
    title_string = win_util::GetWindowText(dialog_owning_window);

  int result = ::AtlMessageBox(
      dialog_owning_window, message.as_string().c_str(), title_string.c_str(),
      kFlags[static_cast<std::size_t>(mode)]);

  switch (result) {
    case IDOK:
      return MessageBoxResult::Ok;
    case IDYES:
      return MessageBoxResult::Yes;
    case IDNO:
      return MessageBoxResult::No;
    case IDCANCEL:
      return MessageBoxResult::Cancel;
    default:
      return MessageBoxResult::Ok;
  }
}

gfx::NativeView DialogServiceImplViews::GetDialogOwningWindow() const {
  return dialog_owning_window;
}

std::filesystem::path DialogServiceImplViews::SelectOpenFile(
    base::StringPiece16 title) {
  std::filesystem::path result;

  base::RunLoop nested_loop;
  auto* selector = CreateFileSelector([&](const base::FilePath& path) {
    result = path.value();
    nested_loop.Quit();
  });

  base::WrapRefCounted(ui::SelectFileDialog::Create(selector, nullptr))
      ->SelectFile(ui::SelectFileDialog::SELECT_OPEN_FILE, title.as_string(),
                   base::FilePath(), nullptr, -1, std::wstring(),
                   dialog_owning_window, nullptr);

  // Run nested loop.
  nested_loop.Run();

  return result;
}

std::filesystem::path DialogServiceImplViews::SelectSaveFile(
    const SaveParams& params) {
  std::filesystem::path result;

  base::RunLoop nested_loop;
  auto* selector = CreateFileSelector([&](const base::FilePath& path) {
    result = path.value();
    nested_loop.Quit();
  });

  base::WrapRefCounted(ui::SelectFileDialog::Create(selector, nullptr))
      ->SelectFile(ui::SelectFileDialog::SELECT_SAVEAS_FILE,
                   params.title.as_string(),
                   base::FilePath{params.default_path.wstring()}, nullptr, -1,
                   std::wstring(), dialog_owning_window, nullptr);

  nested_loop.Run();

  return result;
}
