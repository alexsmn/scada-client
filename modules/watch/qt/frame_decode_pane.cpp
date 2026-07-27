#include "modules/watch/qt/frame_decode_pane.h"

#include "aui/qt/theme_qt.h"
#include "aui/qt/tree.h"
#include "aui/translation.h"
#include "modules/watch/frame_decode_tree_model.h"

#include <QLabel>
#include <QVBoxLayout>

#include <optional>

namespace {

QString Qt16(const std::u16string& text) {
  return QString::fromStdU16String(text);
}

}  // namespace

FrameDecodePane::FrameDecodePane(QWidget* parent)
    : QWidget{parent}, model_{std::make_shared<FrameDecodeTreeModel>()} {
  // Default layout margins on purpose: they come from the platform style, so
  // the pane keeps the host OS's spacing (client/docs/ux/principles.md §9).
  auto* layout = new QVBoxLayout{this};

  header_ = new QLabel;
  header_->setTextInteractionFlags(Qt::TextSelectableByMouse);

  // The raw octets, wrapped. Selectable because the first thing an engineer
  // does with a suspect frame is paste it somewhere else.
  octets_ = new QLabel;
  octets_->setWordWrap(true);
  octets_->setTextInteractionFlags(Qt::TextSelectableByMouse);
  if (std::optional<QFont> mono = scada::aui::MonoValueFont())
    octets_->setFont(*mono);

  tree_ = new scada::aui::Tree{model_};
  // No header: three columns whose meaning is evident from the rows ("@6" is
  // plainly an offset), and vertical space is what the pane is short of.
  tree_->SetHeaderVisible(false);

  layout->addWidget(header_);
  layout->addWidget(octets_);
  layout->addWidget(tree_, 1);

  Clear();
}

FrameDecodePane::~FrameDecodePane() = default;

void FrameDecodePane::ShowFrame(const std::u16string& header,
                                const scada::DeviceFrame& frame) {
  frame_ = frame;
  header_->setText(Qt16(header));

  FrameDecode decode = DecodeFrame(frame);
  // A frame the decoder could not read is precisely the one worth showing raw,
  // so the note replaces the octets only when there are none.
  octets_->setText(Qt16(decode.hex.empty() ? decode.note : decode.hex));
  if (address_map_loaded_)
    AppendMappedNodes(decode, mappings_);
  ShowDecode(decode);
}

void FrameDecodePane::SetAddressMap(std::vector<FrameObjectMapping> mappings) {
  mappings_ = std::move(mappings);
  address_map_loaded_ = true;
  // The map usually arrives after the operator has already selected a frame.
  if (frame_)
    ShowFrame(header_->text().toStdU16String(), *frame_);
}

void FrameDecodePane::Clear() {
  frame_.reset();
  header_->setText(Qt16(Translate("No frame selected")));
  octets_->clear();
  ShowDecode({});
}

void FrameDecodePane::ShowDecode(const FrameDecode& decode) {
  model_->SetDecode(decode);
  // Re-hiding the root is not redundant: Tree::SetRootVisible(false) works by
  // setting the view's root index, and a model reset drops it — without this
  // every decode after the first draws an empty root row above the tree.
  tree_->SetRootVisible(false);
  // The tree is short and the point is to read it at a glance; nothing is
  // served by making the operator open every group.
  tree_->expandAll();
}
