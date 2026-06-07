#include "graph/graph_setup_dialog.h"

#include "aui/dialog_service.h"

#include <QComboBox>
#include <QDialog>
#include <QDialogButtonBox>
#include <QFormLayout>
#include <QIcon>
#include <QPixmap>
#include <QSpinBox>
#include <QString>
#include <QVBoxLayout>

namespace {

QString ToQString(std::u16string_view text) {
  return QString::fromUtf16(text.data(), text.size());
}

QIcon MakeColorIcon(const QColor& color) {
  QPixmap pixmap{16, 16};
  pixmap.fill(color);
  return QIcon{pixmap};
}

class GraphSetupDialogQt final : public QDialog {
 public:
  GraphSetupDialogQt(const GraphSetupDialog& setup, QWidget* parent)
      : QDialog{parent} {
    setWindowTitle("Graph Setup");

    color_combo_ = new QComboBox{this};
    for (int i = 0; i < static_cast<int>(aui::GetColorCount()); ++i) {
      color_combo_->addItem(MakeColorIcon(aui::GetColor(i).qcolor()),
                            ToQString(aui::GetColorName(i)), i);
    }
    int color_index = aui::FindColor(setup.color);
    color_combo_->setCurrentIndex(color_index >= 0 ? color_index : 0);

    line_weight_spin_ = new QSpinBox{this};
    line_weight_spin_->setRange(1, 10);
    line_weight_spin_->setValue(setup.line_weight_);

    auto* form_layout = new QFormLayout;
    form_layout->addRow("Color:", color_combo_);
    form_layout->addRow("Line weight:", line_weight_spin_);

    auto* button_box = new QDialogButtonBox{
        QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this};
    connect(button_box, &QDialogButtonBox::accepted, this, &QDialog::accept);
    connect(button_box, &QDialogButtonBox::rejected, this, &QDialog::reject);

    auto* layout = new QVBoxLayout{this};
    layout->addLayout(form_layout);
    layout->addWidget(button_box);
  }

  void Save(GraphSetupDialog& setup) const {
    setup.color = aui::GetColor(color_combo_->currentData().toInt());
    setup.line_weight_ = line_weight_spin_->value();
  }

 private:
  QComboBox* color_combo_ = nullptr;
  QSpinBox* line_weight_spin_ = nullptr;
};

}  // namespace

bool RunGraphSetupDialog(DialogService& dialog_service,
                         GraphSetupDialog& setup) {
  GraphSetupDialogQt dialog{setup, dialog_service.GetParentWidget()};
  if (dialog.exec() != QDialog::Accepted) {
    return false;
  }

  dialog.Save(setup);
  return true;
}
