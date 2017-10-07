#include "components/node_table/node_table_controller.h"

#include "base/win/clipboard.h"
#include "client_utils.h"
#include "common_resources.h"
#include "components/node_table/node_table_model.h"
#include "services/property_defs.h"
#include "services/task_manager.h"
#include "common/scada_node_ids.h"
#include "controller_factory.h"
#include "controls/grid.h"
#include "core/session_service.h"
#include "common/node_ref_service.h"

#if defined(UI_VIEWS)
#include "ui/views/controls/textfield/combo_textfield.h"
#endif

namespace {

std::string _GetClipboardData() {
  size_t size = 0;
  char* data = nullptr;
  if (!Clipboard().GetData(CF_TRECS, data, size))
    return std::string();

  std::string v(data, data + size);
  delete[] data;
  return v;
}

bool PasteRecords(TaskManager& task_manager, const scada::NodeId& parent_id) {
  auto buffer = _GetClipboardData();
  if (buffer.empty())
    return false;

  assert(false);

  return true;
}

} // namespace

// NodeTableControllerImpl

template<int kNodeId>
class NodeTableControllerImpl : public NodeTableController {
 public:
  explicit NodeTableControllerImpl(const ControllerContext& context)
      : NodeTableController(context, kNodeId) {
  }
};

REGISTER_CONTROLLER(NodeTableControllerImpl<0>, ID_TABLE_EDITOR);
REGISTER_CONTROLLER(NodeTableControllerImpl<numeric_id::TsFormats>, ID_TS_FORMATS_VIEW);
REGISTER_CONTROLLER(NodeTableControllerImpl<numeric_id::Users>, ID_USERS_VIEW);
REGISTER_CONTROLLER(NodeTableControllerImpl<numeric_id::SimulationSignals>, ID_SIMULATION_ITEMS_VIEW);
REGISTER_CONTROLLER(NodeTableControllerImpl<numeric_id::HistoricalDatabases>, ID_HISTORICAL_DB_VIEW);

// NodeTableController

NodeTableController::NodeTableController(const ControllerContext& context, const scada::NodeId& parent_id)
    : Controller(context),
      model_(std::make_unique<NodeTableModel>(PropertyContext{
          context.node_service_,
          context.task_manager_,
          context.node_management_service_,
      })) {
  model_->SetParentNodeId(parent_id);
}

NodeTableController::~NodeTableController() {
}

UiView* NodeTableController::Init(const WindowDefinition& definition) {
  if (const WindowItem* item = definition.FindItem("Item")) {
    std::string path = item->GetString("path");
    auto node_id = scada::NodeId::FromString(path);
    if (!node_id.is_null())
      model_->SetParentNodeId(node_id);
  }

  model_->Update();

#if defined(UI_QT)
  table_.reset(new Grid(*model_, model_->row_model(), model_->column_model()));
  return table_.get();

#elif defined(UI_VIEWS)
  table_.reset(new views::GridView);
  table_->SetColumnModel(&model_->column_model());
  table_->SetRowModel(&model_->row_model());
  table_->SetModel(model_.get());
  table_->set_controller(this);
  table_->SetRowHeadersVisible(true);
  table_->set_allow_column_select(true);
  table_->set_allow_expand(true);
  table_->SetTopHeaderHeight(19);
  table_->SetRowHeaderWidth(70);
  table_->SelectCell(0, 0, true);
  return table_->CreateParentIfNecessary();
#endif
}

void NodeTableController::Save(WindowDefinition& definition) {
  if (const auto& parent_node = model_->parent_node()) {
    auto path = parent_node.id().ToString();
    definition.AddItem("Item").SetString("path", path);
  }
}

#if defined(UI_VIEWS)
void NodeTableController::ShowContextMenu(gfx::Point point) {
  controller_delegate_.ShowPopupMenu(IDR_ITEM_POPUP, point, true);
}
#endif

void NodeTableController::DeleteSelection() {
  if (!session_service_.IsAdministrator())
    return;

#if defined(UI_VIEWS)
  ui::GridRange range = table_->GetSelectionRange();
  if (range.empty())
    return;

  for (int i = range.row(); i <= range.last_row(); i++)
    DeleteTreeRecordsRecursive(model_->nodes()[i], task_manager_);
#endif
}

void NodeTableController::CopyToClipboard() {
#if defined(UI_VIEWS)
/*  ui::GridRange range = table_->GetSelectionRange();
  if (range.empty())
    return;
    
  std::vector<NodeRef> nodes;

  for (int i = range.row(); i <= range.last_row(); i++)
    nodes.emplace_back(model_->nodes()[i]);

  if (nodes.empty())
    return;

  std::vector<scada::BrowseNode> browse_nodes;
  std::vector<scada::BrowseReference> browse_references;

  for (auto& node : nodes) {
    browse_nodes.emplace_back();
    NodeToBrowse(node.node(), browse_nodes.back(), browse_references);
  }

  protocol::BrowseResult message;
  ToProto(browse_nodes, *message.mutable_nodes());
  ToProto(browse_references, *message.mutable_references());

  auto buffer = message.SerializePartialAsString();
  if (!Clipboard().SetData(CF_TRECS, buffer.data(), buffer.size()))
    LOG(ERROR) << "Can't set clipboard data";*/
#endif
}

void NodeTableController::PasteFromClipboard() {
  const auto& parent_node = model_->parent_node();
  scada::NodeId parent_id = parent_node ? parent_node.id() : scada::id::RootFolder;
  if (!session_service_.IsAdministrator() ||
      !PasteRecords(task_manager_, parent_id)) {
    LOG(ERROR) << "Paste records error";
  }
}

#if defined(UI_VIEWS)
bool NodeTableController::OnGridEditCellText(views::GridView& sender,
                                         int row, int column,
                                         const base::string16& text) {
  ui::GridRange range = table_->GetSelectionRange();
  if (range.empty())
    return true;
  
  for (int row = range.row(); row <= range.last_row(); row++)
    for (int col = range.column(); col <= range.last_column(); col++)
      model_->SetCellText(row, col, text);

  return true;
}

bool NodeTableController::CanEditCell(views::GridView& sender, int row, int column) {
  return true;
}

views::ComboTextfield* NodeTableController::OnGridCreateEditor(views::GridView& sender, int row, int column) {
  auto result = model_->GetCellEditor(row, column);
  if (result.type == PropertyEditor::NONE)
    return nullptr;

  views::ComboTextfield* editor = __super::OnGridCreateEditor(sender, row, column);
  assert(editor);

  if (result.type == PropertyEditor::DROPDOWN) {
    editor->set_type(views::ComboTextfield::DROP_LIST);
    for (auto& choice : result.choices)
      editor->AddChoice(choice);

  } else if (result.type == PropertyEditor::BUTTON) {
    editor->set_type(views::ComboTextfield::BUTTON);
  }

  return editor;
}
#endif

CommandHandler* NodeTableController::GetCommandHandler(unsigned command_id) {
  switch (command_id) {
    case ID_DELETE:
    case ID_COPY:
    case ID_PASTE:
      return this;
  }

  return __super::GetCommandHandler(command_id);
}

bool NodeTableController::IsCommandEnabled(unsigned command_id) const {
  switch (command_id) {
    case ID_DELETE:
#if defined(UI_VIEWS)
      return table_->selected_row() != -1;
#else
      return false;
#endif
    default:
      return __super::IsCommandEnabled(command_id);
  }
}

void NodeTableController::ExecuteCommand(unsigned command) {
  switch (command) {
    case ID_DELETE:
      DeleteSelection();
      break;
    case ID_COPY:
      CopyToClipboard();
      break;
    case ID_PASTE:
      PasteFromClipboard();
      break;
    default:
      __super::ExecuteCommand(command);
      break;
  }
}

NodeRef NodeTableController::GetRootNode() const {
  return model_->parent_node();
}

#if defined(UI_VIEWS)
bool NodeTableController::OnKeyPressed(views::GridView& sender, ui::KeyboardCode key_code) {
  // Clear selection when Delete is pressed.
  if (key_code == ui::VKEY_DELETE && !table_->editing()) {
    ui::GridRange range = table_->GetSelectionRange();
    if (!range.empty()) {
      for (int row = range.row(); row <= range.last_row(); row++)
        for (int col = range.column(); col <= range.last_column(); col++)
          model_->SetCellText(row, col, base::string16());
      return true;
    }
  }

  return false;
}

void NodeTableController::OnGridSelectionChanged(views::GridView& sender) {
  __super::OnGridSelectionChanged(sender);

  int row = table_->selected_row();
  if (row == -1) {
    selection().Clear();
    return;
  }
  
  const auto& node = model_->nodes()[row];
  assert(node);

  selection().SelectNode(node);
}
#endif