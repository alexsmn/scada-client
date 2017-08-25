#include "components/login/views/login_dialog_views.h"

#include "ui/views/layout/grid_layout.h"
#include "ui/views/controls/label.h"
#include "ui/views/controls/textfield/combo_textfield.h"
#include "ui/views/window/dialog_delegate.h"
#include "ui/views/widget/widget.h"

LoginView::LoginView() {
  auto* layout = views::GridLayout::CreatePanel(this);
  auto* columns = layout->AddColumnSet(0);
  columns->AddColumn(views::GridLayout::LEADING, views::GridLayout::BASELINE, 0, views::GridLayout::FIXED, 50, 0);
  columns->AddPaddingColumn(0, 10);
  columns->AddColumn(views::GridLayout::LEADING, views::GridLayout::BASELINE, 0, views::GridLayout::FIXED, 100, 0);
  SetLayoutManager(layout);

  layout->StartRow(0, 0);
  layout->AddView(new views::Label(L"Имя:"));
  layout->AddView(new views::ComboTextfield);

  layout->StartRow(0, 0);
  layout->AddView(new views::Label(L"Пароль:"));
  layout->AddView(new views::ComboTextfield);

  layout->StartRow(0, 0);
  layout->AddView(new views::Label(L"Сервер:"));
  layout->AddView(new views::ComboTextfield);

//  layout->StartRow(0, 0);
//  layout->SkipColumns(1);
//  layout->AddView(new views::C)
}

class LoginDialogDelegate : public views::DialogDelegate {
 public:
  LoginDialogDelegate();

  // views::WidgetDelegate
  virtual views::View* GetContentsView() override;

 private:
  LoginView* contents_view_;
};

LoginDialogDelegate::LoginDialogDelegate()
    : contents_view_(new LoginView) {
}

views::View* LoginDialogDelegate::GetContentsView() {
  return contents_view_; 
}

void RunLoginDialog(gfx::NativeView parent) {
  LoginDialogDelegate delegate;
  std::unique_ptr<views::Widget> dialog(
      LoginDialogDelegate::CreateDialogWidgetWithBounds(&delegate, nullptr,  parent, gfx::Rect(100, 100, 500, 300)));
}
