#include "modules/watch/qt/watch_filter_bar.h"

#include "aui/translation.h"

#include <QCheckBox>
#include <QComboBox>
#include <QHBoxLayout>
#include <QLineEdit>
#include <QStyle>
#include <QWidget>

namespace {

QString Qt16(const std::u16string& text) {
  return QString::fromStdU16String(text);
}

}  // namespace

QWidget* CreateWatchFilterBar(std::function<void(WatchFilter)> on_change) {
  auto* bar = new QWidget;
  auto* layout = new QHBoxLayout{bar};
  const int margin =
      bar->style()->pixelMetric(QStyle::PM_LayoutLeftMargin, nullptr, bar);
  layout->setContentsMargins(margin, margin / 2, margin, margin / 2);

  auto* kind = new QComboBox;
  kind->addItem(Qt16(Translate("All frames")),
                static_cast<int>(WatchFilter::Kind::kAny));
  kind->addItem(Qt16(Translate("I-format")),
                static_cast<int>(WatchFilter::Kind::kInformation));
  // S and U together: neither carries data, and both answer the same question
  // about a link that is not moving.
  kind->addItem(Qt16(Translate("S/U-format")),
                static_cast<int>(WatchFilter::Kind::kSupervisoryAndUnnumbered));
  layout->addWidget(kind);

  auto* errors = new QCheckBox{Qt16(Translate("Errors only"))};
  layout->addWidget(errors);

  auto* text = new QLineEdit;
  text->setPlaceholderText(Qt16(Translate("Filter by IOA, type or cause")));
  text->setClearButtonEnabled(true);
  layout->addWidget(text, 1);

  const auto apply = [kind, errors, text,
                      on_change = std::move(on_change)] {
    on_change(
        {.kind = static_cast<WatchFilter::Kind>(kind->currentData().toInt()),
         .errors_only = errors->isChecked(),
         .text = text->text().toStdU16String()});
  };
  QObject::connect(kind, &QComboBox::currentIndexChanged, bar, apply);
  QObject::connect(errors, &QCheckBox::toggled, bar, apply);
  QObject::connect(text, &QLineEdit::textChanged, bar, apply);

  return bar;
}
