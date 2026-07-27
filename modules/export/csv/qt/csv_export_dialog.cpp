#include "ui_csv_export_dialog.h"

#include <QPushButton>

#include <QDialogButtonBox>

#include "aui/dialog_service.h"
#include "aui/qt/dialog_util.h"
#include "base/value_util.h"
#include "export/csv/csv_export.h"
#include "export/csv/csv_export_util.h"
#include "profile/profile.h"

#include <QMessageBox>

class CsvExportDialog : public QDialog {
  Q_OBJECT

 public:
  explicit CsvExportDialog(const CsvExportParams& params,
                           bool can_expand,
                           QWidget* parent = nullptr);

  CsvExportParams params_;

 public Q_SLOTS:
  virtual void accept() override;

 private:
  Ui::CsvExportDialog ui;

  static const int kTabIndex = 3;
  static const int kSpaceIndex = 4;
};

#include "csv_export_dialog.moc"

CsvExportDialog::CsvExportDialog(const CsvExportParams& params,
                                 bool can_expand,
                                 QWidget* parent)
    : QDialog{parent}, params_{params} {
  ui.setupUi(this);

  // Name the action rather than the assent (docs/ux/dialogs.md §3).
  ui.buttonBox->button(QDialogButtonBox::Ok)->setText(tr("Export"));

  // Only the views that group rows can expand them; for every other export the
  // choice would be meaningless, so it is not offered.
  ui.expandGroupsCheckBox->setVisible(can_expand);
  ui.expandGroupsCheckBox->setChecked(params_.expand_groups);

  ui.encodingComboBox->setCurrentIndex(params_.unicode ? 1 : 0);

  if (params_.delimiter == '\t')
    ui.delimiterComboBox->setCurrentIndex(kTabIndex);
  else if (params_.delimiter == ' ')
    ui.delimiterComboBox->setCurrentIndex(kSpaceIndex);
  else
    ui.delimiterComboBox->setCurrentText(QChar::fromLatin1(params_.delimiter));

  ui.quoteComboBox->setCurrentText(QChar::fromLatin1(params_.quote));
}

void CsvExportDialog::accept() {
  params_.unicode = ui.encodingComboBox->currentIndex() != 0;
  // Left at its stored value when the box is hidden, so a view without groups
  // never silently rewrites the preference.
  if (ui.expandGroupsCheckBox->isVisible())
    params_.expand_groups = ui.expandGroupsCheckBox->isChecked();

  auto delimiter_index = ui.delimiterComboBox->currentIndex();
  auto delimiter = ui.delimiterComboBox->currentText();
  if (delimiter_index == kTabIndex)
    params_.delimiter = '\t';
  else if (delimiter_index == kSpaceIndex)
    params_.delimiter = ' ';
  else if (delimiter.size() == 1)
    params_.delimiter = delimiter.front().toLatin1();
  else {
    ui.delimiterComboBox->setFocus();
    QMessageBox::critical(this,
                          tr("Please enter a symbol for the delimiter or "
                             "choose one from the drop-down list."),
                          windowTitle());
    return;
  }

  auto quote = ui.quoteComboBox->currentText();
  if (quote.size() == 1)
    params_.quote = quote.front().toLatin1();
  else {
    ui.quoteComboBox->setFocus();
    QMessageBox::critical(this,
                          tr("Please enter a symbol for the quote or choose "
                             "one from the drop-down list."),
                          windowTitle());
    return;
  }

  QDialog::accept();
}

Awaitable<CsvExportParams> ShowCsvExportDialog(DialogService& dialog_service,
                                               Profile& profile,
                                               bool can_expand) {
  auto csv_export_params =
      FromJson<CsvExportParams>(GetKey(profile.data(), "csv"))
          .value_or(CsvExportParams{});

  auto dialog = std::make_unique<CsvExportDialog>(
      csv_export_params, can_expand, dialog_service.GetParentWidget());

  return StartMappedModalDialog(
      std::move(dialog), [&profile](CsvExportDialog& dialog) {
        profile.data().as_object()["csv"] = ToJson(dialog.params_);
        return dialog.params_;
      });
}
