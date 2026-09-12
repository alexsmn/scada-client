#include "bulk_create/qt/bulk_create_wizard.h"

#include "aui/dialog_service.h"
#include "aui/qt/dialog_util.h"
#include "aui/translation.h"
#include "bulk_create/bulk_create_plan.h"
#include "bulk_create/qt/bulk_create_preview_panel.h"
#include "model/data_items_node_ids.h"
#include "model/devices_node_ids.h"
#include "model/node_id_util.h"
#include "node_service/node_ref.h"
#include "node_service/node_service.h"
#include "node_service/node_util.h"
#include "scada/node_id.h"
#include "services/task_manager.h"

#include <QComboBox>
#include <QFormLayout>
#include <QLabel>
#include <QLineEdit>
#include <QVBoxLayout>
#include <QWizard>
#include <QWizardPage>

#include <memory>
#include <set>
#include <utility>

namespace {

QString Tr(std::string_view text) {
  return QString::fromStdU16String(Translate(text));
}

// Every device under `parent`, depth-first, into `combo` — display name shown,
// NodeId carried as the item's data. Devices nest (a device may organize
// further devices), which is why this recurses rather than listing one level.
void FillDevices(const NodeRef& parent, QComboBox& combo) {
  for (const NodeRef& node : parent.targets(scada::id::Organizes)) {
    if (!IsInstanceOf(node, scada::devices::id::DeviceType))
      continue;
    combo.addItem(QString::fromStdU16String(GetFullDisplayName(node)),
                  QString::fromStdString(node.node_id().ToString()));
    FillDevices(node, combo);
  }
}

// The NodeIds already under the target, for the preview's conflict column.
// Read once when the wizard opens: the operator is the only thing creating
// nodes here, so re-reading per keystroke would cost a browse per character
// and still race anything else.
std::set<std::u16string> ReadExistingNodeIds(NodeService& node_service,
                                             const scada::NodeId& parent_id) {
  std::set<std::u16string> existing;
  if (parent_id.is_null())
    return existing;
  for (const NodeRef& child :
       node_service.GetNode(parent_id).targets(scada::id::Organizes)) {
    const std::string id = child.node_id().ToString();
    existing.emplace(id.begin(), id.end());
  }
  return existing;
}

class BulkCreateWizard final : public QWizard {
 public:
  explicit BulkCreateWizard(BulkCreateWizardContext context, QWidget* parent);

  BulkCreateSubject subject() const;
  const BulkCreateWizardContext& context() const { return context_; }
  BulkCreatePreviewPanel& preview() { return *preview_; }

  // The nodes Finish would create, from the pattern the operator has edited.
  // The single source of both the Review page's count and what is posted, so
  // the two cannot disagree.
  std::vector<scada::NodeState> PlannedNodes() const;

  void accept() override;

 private:
  BulkCreateWizardContext context_;
  std::set<std::u16string> existing_node_ids_;

  QComboBox* subject_ = nullptr;
  QComboBox* device_ = nullptr;
  QComboBox* item_type_ = nullptr;
  QLineEdit* source_path_ = nullptr;
  BulkCreatePreviewPanel* preview_ = nullptr;
  QLabel* review_ = nullptr;

  friend class TargetAndTypePage;
  friend class PatternPage;
  friend class ReviewPage;
};

// Step 1. The subject lives here because it decides everything after it: which
// fields this page shows, whether the pattern step carries an address, and
// which node type is created.
class TargetAndTypePage final : public QWizardPage {
 public:
  explicit TargetAndTypePage(BulkCreateWizard& wizard) : wizard_{wizard} {
    setTitle(Tr("Target and type"));

    auto* layout = new QFormLayout{this};

    wizard_.subject_ = new QComboBox;
    wizard_.subject_->setObjectName(QStringLiteral("bulkCreateSubject"));
    wizard_.subject_->addItem(Tr("Data items"),
                              static_cast<int>(BulkCreateSubject::kDataItem));
    // Offered only when the caller handed over sources. A rule forwards an
    // existing node, so without a selection there is nothing for one to
    // forward and the subject would produce an empty run.
    if (!wizard_.context_.source_node_ids_.empty()) {
      wizard_.subject_->addItem(
          Tr("Transmission rules"),
          static_cast<int>(BulkCreateSubject::kTransmissionItem));
    }

    wizard_.item_type_ = new QComboBox;
    wizard_.item_type_->setObjectName(QStringLiteral("bulkCreateItemType"));
    wizard_.item_type_->addItem(Tr("Analog"), false);
    wizard_.item_type_->addItem(Tr("Discrete"), true);

    wizard_.device_ = new QComboBox;
    wizard_.device_->setObjectName(QStringLiteral("bulkCreateDevice"));

    wizard_.source_path_ = new QLineEdit(QStringLiteral("Signal{n}"));
    wizard_.source_path_->setObjectName(QStringLiteral("bulkCreateSourcePath"));

    layout->addRow(Tr("What to create"), wizard_.subject_);
    layout->addRow(Tr("Item type"), wizard_.item_type_);
    layout->addRow(Tr("Device"), wizard_.device_);
    layout->addRow(Tr("Source path template"), wizard_.source_path_);
    item_type_label_ = layout->labelForField(wizard_.item_type_);
    device_label_ = layout->labelForField(wizard_.device_);
    source_path_label_ = layout->labelForField(wizard_.source_path_);

    connect(wizard_.subject_, &QComboBox::currentIndexChanged, this,
            [this](int) { ApplySubject(); });
    ApplySubject();
  }

 private:
  // The data item's binding fields have no meaning for a rule, whose source is
  // selected rather than named. Hidden with their labels, for the reason the
  // preview panel hides its IOA row: a QFormLayout label outlives the field it
  // captions.
  void ApplySubject() {
    const bool data_item = wizard_.subject() == BulkCreateSubject::kDataItem;
    for (QWidget* widget :
         {static_cast<QWidget*>(wizard_.item_type_),
          static_cast<QWidget*>(wizard_.device_),
          static_cast<QWidget*>(wizard_.source_path_), item_type_label_,
          device_label_, source_path_label_}) {
      if (widget)
        widget->setVisible(data_item);
    }
  }

  BulkCreateWizard& wizard_;
  QWidget* item_type_label_ = nullptr;
  QWidget* device_label_ = nullptr;
  QWidget* source_path_label_ = nullptr;
};

// Step 2. The existing preview panel, unchanged — this page only tells it the
// subject and the conflict set, which is the whole point of having built it as
// a widget rather than as part of a dialog.
class PatternPage final : public QWizardPage {
 public:
  explicit PatternPage(BulkCreateWizard& wizard) : wizard_{wizard} {
    setTitle(Tr("Pattern"));
    setSubTitle(Tr("Naming and addressing"));
    auto* layout = new QVBoxLayout{this};
    layout->setContentsMargins(0, 0, 0, 0);
    layout->addWidget(wizard_.preview_);
  }

  void initializePage() override {
    wizard_.preview_->SetSubject(wizard_.subject());
    wizard_.preview_->SetExistingNodeIds(wizard_.existing_node_ids_);
  }

 private:
  BulkCreateWizard& wizard_;
};

// Step 3. Counts what Finish will actually do, from the same planner that does
// it.
class ReviewPage final : public QWizardPage {
 public:
  explicit ReviewPage(BulkCreateWizard& wizard) : wizard_{wizard} {
    setTitle(Tr("Review and create"));
    auto* layout = new QVBoxLayout{this};
    wizard_.review_ = new QLabel;
    wizard_.review_->setObjectName(QStringLiteral("bulkCreateReview"));
    wizard_.review_->setWordWrap(true);
    layout->addWidget(wizard_.review_);
    layout->addStretch();
  }

  void initializePage() override {
    const std::size_t planned = wizard_.PlannedNodes().size();
    const std::size_t rows = wizard_.preview_->rows().size();
    wizard_.review_->setText(Tr("Will create %1 of %2").arg(planned).arg(rows));
  }

 private:
  BulkCreateWizard& wizard_;
};

BulkCreateWizard::BulkCreateWizard(BulkCreateWizardContext context,
                                   QWidget* parent)
    : QWizard{parent}, context_{std::move(context)} {
  setWindowTitle(Tr("Bulk create"));
  setObjectName(QStringLiteral("bulkCreateWizard"));
  // The platform's own wizard chrome; the stepper the screen draws is this
  // control's business, not ours (client/CLAUDE.md: prefer stock Qt widgets in
  // their conventional roles).
  setOption(QWizard::NoBackButtonOnStartPage);

  existing_node_ids_ =
      ReadExistingNodeIds(context_.node_service_, context_.parent_id_);
  preview_ = new BulkCreatePreviewPanel;

  addPage(new TargetAndTypePage{*this});
  addPage(new PatternPage{*this});
  addPage(new ReviewPage{*this});

  setButtonText(QWizard::FinishButton, Tr("Create"));

  // Populate the device list after the pages exist, since the combo is built
  // by the first of them. Same walk as the dialog this replaces: devices nest,
  // so a flat pass over the Devices folder would miss every child device.
  FillDevices(context_.node_service_.GetNode(scada::devices::id::Devices),
              *device_);
}

BulkCreateSubject BulkCreateWizard::subject() const {
  if (!subject_)
    return BulkCreateSubject::kDataItem;
  return static_cast<BulkCreateSubject>(subject_->currentData().toInt());
}

std::vector<scada::NodeState> BulkCreateWizard::PlannedNodes() const {
  BulkCreatePlan plan;
  plan.subject = subject();
  plan.parent_id = context_.parent_id_;
  if (plan.subject == BulkCreateSubject::kDataItem) {
    plan.type_definition_id = item_type_->currentData().toBool()
                                  ? scada::data_items::id::DiscreteItemType
                                  : scada::data_items::id::AnalogItemType;
    plan.source_device_id = scada::NodeId::FromString(
        device_->currentData().toString().toStdString());
    plan.source_path_template = source_path_->text().toStdU16String();
  } else {
    plan.source_node_ids = context_.source_node_ids_;
  }
  return PlanBulkCreate(plan, preview_->rows());
}

void BulkCreateWizard::accept() {
  for (const scada::NodeState& node : PlannedNodes())
    context_.task_manager_.PostInsertTask(node);
  QWizard::accept();
}

}  // namespace

QWizard* MakeBulkCreateWizard(BulkCreateWizardContext&& context) {
  return new BulkCreateWizard{std::move(context), /*parent=*/nullptr};
}

void ShowBulkCreateWizard(DialogService& dialog_service,
                          BulkCreateWizardContext&& context) {
  auto wizard = std::make_unique<BulkCreateWizard>(
      std::move(context), dialog_service.GetParentWidget());
  ShowSelfOwnedModalDialog(std::move(wizard));
}
