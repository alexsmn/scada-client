#include "modules/write/write_dialog.h"

#include "aui/qt/dialog_service_impl_qt.h"
#include "aui/qt/dialog_util.h"
#include "modules/write/qt/control_review_dialog.h"
#include "modules/write/write_model.h"
#include "ui_write_dialog.h"

#include <QCheckBox>
#include <QComboBox>
#include <QDialogButtonBox>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>

class WriteDialog : public QDialog {
  Q_OBJECT

 public:
  explicit WriteDialog(std::shared_ptr<WriteModel> model,
                       QWidget* parent = nullptr);

 public Q_SLOTS:
  virtual void accept() override;

 private:
  void UpdateCurrent();
  void SeedValueControl();
  void UpdateCondition();
  void UpdateStatus();

  Ui::WriteDialog ui;

  const std::shared_ptr<WriteModel> model_;
  DialogServiceImplQt dialog_service_;
  // Set once the operator picks or types a value, after which the dialog stops
  // proposing one of its own.
  bool value_touched_ = false;
};

#include "write_dialog.moc"

WriteDialog::WriteDialog(std::shared_ptr<WriteModel> model, QWidget* parent)
    : QDialog{parent}, model_{std::move(model)} {
  ui.setupUi(this);

  // Name the action rather than the assent (docs/client/ux/dialogs.md §3). This
  // one matters most: for a control command the accept button is the second
  // half of a two-stage confirm (principles.md §7), and "OK" throws that away
  // by asking the operator to re-read the title to learn what they are agreeing
  // to. Manual entry writes a value into the point; a command is executed on
  // a device, so the two are labelled differently.
  QPushButton* const accept_button = ui.buttonBox->button(QDialogButtonBox::Ok);
  accept_button->setText(model_->manual() ? tr("Write") : tr("Execute"));

  // A field command is not sent on Return. The new-value box is pre-seeded
  // with the state opposite the present one, so with Execute as the default,
  // opening the dialog on a closed breaker and pressing Return started the
  // trip. The review that follows defaults to Cancel, but an administrator may
  // switch the review off, and then nothing stood between Return and the field
  // (UX audit 2026-11-25, item 3.3; docs/client/ux/dialogs.md §4). Manual input
  // keeps the ordinary default: it writes a value, not a command.
  if (!model_->manual()) {
    accept_button->setAutoDefault(false);
    accept_button->setDefault(false);
    if (QPushButton* cancel = ui.buttonBox->button(QDialogButtonBox::Cancel))
      cancel->setDefault(true);
  }

  dialog_service_.parent_widget = this;
  model_->set_dialog_service(&dialog_service_);
  // The second stage of the two-stage confirm: a review naming the object,
  // the values, the operator, the time and the stage (backlog 1118). A plain
  // lambda returning a coroutine that takes everything by value — the frame
  // outlives this call.
  model_->set_control_reviewer([this](ControlReview review) {
    return RunControlReviewDialog(this, std::move(review));
  });

  setWindowTitle(QString::fromStdU16String(model_->GetWindowTitle()));

  // The path, not the bare name: two substations' «Силовой выключатель» are
  // told apart only by what is above them (UX audit 2026-11-25, item 3.1).
  ui.descriptionLabel->setText(
      QString::fromStdU16String(model_->GetObjectPath()));
  ui.descriptionLabel->setWordWrap(true);

  // Manual input and a control command share this form, and differed only in
  // the accept button's word, so an operator could believe a breaker opened
  // when only the system's value changed, or the reverse (audit item 3.4).
  if (model_->manual()) {
    auto* note = new QLabel(
        tr("The value is entered into the system by hand; no command is sent "
           "to the equipment."),
        this);
    note->setObjectName(QStringLiteral("manualNoteLabel"));
    note->setWordWrap(true);
    ui.verticalLayout->insertWidget(
        ui.verticalLayout->indexOf(ui.descriptionLabel) + 1, note);
    // «Блокировка» alone does not say what it does: it makes the Server ignore
    // the device's telemetry for this object until it is unblocked.
    ui.lockCheckBox->setText(tr("Ignore values from the device"));
  }
  ui.currentValueLabel->setText(
      QString::fromStdU16String(model_->GetCurrentValue(true)));

  ui.valueComboBox->setEditable(!model_->discrete());

  ui.unitLabel->setText({});
  ui.conditionLabel->setText({});
  ui.statusLabel->setText({});
  ui.lockLabel->setVisible(model_->lock_allowed());
  ui.lockCheckBox->setVisible(model_->lock_allowed());
  ui.lockCheckBox->setChecked(model_->locked());
  ui.conditionHeaderLabel->setVisible(model_->has_condition());
  ui.conditionLabel->setVisible(model_->has_condition());
  // A blocked command names its interlock, which can be a long formula.
  ui.conditionLabel->setWordWrap(true);

  if (model_->discrete()) {
    for (const auto& state : model_->GetDiscreteStates())
      ui.valueComboBox->addItem(QString::fromStdU16String(state));
  } else {
    ui.unitLabel->setText(QString::fromStdU16String(model_->GetAnalogUnits()));
  }

  // Both signals fire only for operator action, which is what makes them
  // usable as the "leave it alone from now on" trigger: setCurrentIndex() and
  // setCurrentText() do not emit `activated`, and a programmatic text change
  // emits editTextChanged() rather than QLineEdit::textEdited().
  connect(ui.valueComboBox, &QComboBox::activated, this,
          [this] { value_touched_ = true; });
  if (auto* line_edit = ui.valueComboBox->lineEdit()) {
    connect(line_edit, &QLineEdit::textEdited, this,
            [this] { value_touched_ = true; });
  }

  model_->current_change_handler = [this] { UpdateCurrent(); };
  model_->condition_change_handler = [this] { UpdateCondition(); };
  model_->status_change_handler = [this] { UpdateStatus(); };

  model_->completion_handler = [this](bool ok) {
    ui.statusLabel->setText({});
    if (ok)
      QDialog::accept();
  };

  UpdateCurrent();
  UpdateCondition();
  UpdateStatus();
}

void WriteDialog::UpdateCurrent() {
  ui.currentValueLabel->setText(
      QString::fromStdU16String(model_->GetCurrentValue(true)));
  SeedValueControl();
}

// Proposes a new value from the item's present reading, and keeps proposing it
// until the operator picks or types one of their own.
//
// The reading arrives over TimedDataService a turn or more after the dialog is
// built, so seeding only in the constructor left the operator looking at an
// empty analog box, or — worse on the discrete side — at
// GetCurrentDiscreteState()'s `get_or(true)` default, which proposes the state
// the item is already in while the "Current value:" label corrects itself to
// the other one. Re-seeding on every change closes that window; stopping once
// `value_touched_` is set keeps a value that ticks in from silently replacing a
// deliberate selection, which would be the worse failure of the two.
void WriteDialog::SeedValueControl() {
  if (value_touched_)
    return;

  if (model_->discrete()) {
    ui.valueComboBox->setCurrentIndex(model_->GetCurrentDiscreteState());
  } else {
    ui.valueComboBox->setCurrentText(
        QString::fromStdU16String(model_->GetCurrentValue(false)));
  }
}

void WriteDialog::UpdateCondition() {
  ui.conditionLabel->setText(
      QString::fromStdU16String(model_->GetConditionText()));
  ui.buttonBox->button(QDialogButtonBox::Ok)
      ->setEnabled(model_->IsConditionOk());
}

void WriteDialog::UpdateStatus() {
  ui.statusLabel->setText(QString::fromStdU16String(model_->GetStatusText()));
}

void WriteDialog::accept() {
  double value = 0.0;
  if (model_->discrete()) {
    auto index = ui.valueComboBox->currentIndex();
    value = index ? 1.0 : 0.0;

  } else {
    // Not QString::toDouble, which is C-locale and refused «12,5» (backlog
    // 912).
    const std::optional<double> parsed =
        ParseOperatorNumber(ui.valueComboBox->currentText().toStdU16String());
    if (parsed)
      value = *parsed;
    if (!parsed) {
      // Through the model, which owns the executor the message box has to be
      // spawned on: RunMessageBox is lazy, and calling it here discarded the
      // awaitable, so the box never appeared and the dialog just refused to
      // close.
      model_->ReportInputError(
          tr("Incorrect floating point value.").toStdU16String());
      return;
    }
  }

  bool lock = false;
  if (model_->lock_allowed())
    lock = ui.lockCheckBox->isChecked();

  model_->Write(value, lock);
}

Awaitable<void> ExecuteWriteDialog(DialogService& dialog_service,
                                   WriteContext context) {
  auto model = std::make_shared<WriteModel>(std::move(context));
  auto dialog =
      std::make_unique<WriteDialog>(model, dialog_service.GetParentWidget());
  return StartOwnedModalDialog(std::move(dialog));
}
