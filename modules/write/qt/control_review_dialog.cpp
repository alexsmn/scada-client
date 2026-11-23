#include "modules/write/qt/control_review_dialog.h"

#include "aui/qt/dialog_util.h"

#include <QDialogButtonBox>
#include <QFormLayout>
#include <QLabel>
#include <QPushButton>
#include <QVBoxLayout>

namespace {

// A value cell: selectable, so the operator can copy what they reviewed into
// a shift log, and wrapping, so a long object name or interlock does not
// widen the dialog past the screen.
QLabel* MakeValueLabel(const std::u16string& text, QWidget* parent) {
  auto* label = new QLabel{QString::fromStdU16String(text), parent};
  label->setTextInteractionFlags(Qt::TextSelectableByMouse);
  label->setWordWrap(true);
  return label;
}

}  // namespace

ControlReviewDialog::ControlReviewDialog(const ControlReview& review,
                                         QWidget* parent)
    : QDialog{parent} {
  // The title bar carries «Control»; the content does not repeat it
  // (docs/client/ux/dialogs.md §1).
  setWindowTitle(QString::fromStdU16String(review.title));

  auto* layout = new QVBoxLayout{this};

  auto* form = new QFormLayout;
  form->addRow(tr("Object:"), MakeValueLabel(review.object, this));
  form->addRow(tr("Present:"), MakeValueLabel(review.present, this));
  // The commanded value is what the operator is agreeing to, so it is the one
  // emphasised cell — through the font, not a colour (principle §5).
  QLabel* command = MakeValueLabel(review.command, this);
  QFont command_font = command->font();
  command_font.setBold(true);
  command->setFont(command_font);
  form->addRow(tr("Command:"), command);
  form->addRow(tr("Operator:"), MakeValueLabel(review.operator_name, this));
  form->addRow(tr("Time:"), MakeValueLabel(review.time, this));
  form->addRow(tr("Stage:"), MakeValueLabel(review.stage, this));
  layout->addLayout(form);

  // The consequence, in body text (dialogs.md §4).
  auto* warning = new QLabel{QString::fromStdU16String(review.warning), this};
  warning->setWordWrap(true);
  layout->addWidget(warning);

  button_box_ = new QDialogButtonBox{
      QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this};
  QPushButton* send = button_box_->button(QDialogButtonBox::Ok);
  send->setText(tr("Send command"));
  QPushButton* cancel = button_box_->button(QDialogButtonBox::Cancel);
  // Cancel is the default and has focus, so neither Return nor Space sends an
  // irreversible command by reflex (dialogs.md §4). QDialog makes the first
  // auto-default button the default on show, so «Send command» must stop
  // being one.
  send->setAutoDefault(false);
  send->setDefault(false);
  cancel->setAutoDefault(true);
  cancel->setDefault(true);
  cancel->setFocus(Qt::OtherFocusReason);
  connect(button_box_, &QDialogButtonBox::accepted, this, &QDialog::accept);
  connect(button_box_, &QDialogButtonBox::rejected, this, &QDialog::reject);
  layout->addWidget(button_box_);

  // Wide enough that the warning wraps into a paragraph rather than a column;
  // measured in the dialog's own font, not pixels.
  setMinimumWidth(fontMetrics().averageCharWidth() * 48);
}

Awaitable<bool> RunControlReviewDialog(QWidget* parent, ControlReview review) {
  auto dialog = std::make_unique<ControlReviewDialog>(review, parent);
  co_return co_await StartFinishedModalDialog(
      std::move(dialog), [](ControlReviewDialog&, int result) {
        return result == QDialog::Accepted;
      });
}
