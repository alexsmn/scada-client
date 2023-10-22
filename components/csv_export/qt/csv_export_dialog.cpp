#include "ui_csv_export_dialog.h"

#include "components/csv_export/csv_export.h"
#include "export_util.h"
#include "qt/dialog_util.h"
#include "services/dialog_service.h"
#include "profile/profile.h"

#include <QMessageBox>

class CsvExportDialog : public QDialog {
  Q_OBJECT

 public:
  explicit CsvExportDialog(const CsvExportParams& params,
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

CsvExportDialog::CsvExportDialog(const CsvExportParams& params, QWidget* parent)
    : QDialog{parent}, params_{params} {
  ui.setupUi(this);

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

promise<CsvExportParams> ShowCsvExportDialog(DialogService& dialog_service,
                                             Profile& profile) {
  auto dialog = std::make_unique<CsvExportDialog>(
      profile.csv_export_params, dialog_service.GetParentWidget());
  return StartModalDialog(std::move(dialog)).then([](CsvExportDialog* dialog) {
    return dialog->params_;
  });
}
