#include "components/node_table/node_table_controller.h"

#include "base/win/clipboard.h"
#include "client_utils.h"
#include "common/node_id_util.h"
#include "common/node_service.h"
#include "common/node_state.h"
#include "common/scada_node_ids.h"
#include "common_resources.h"
#include "components/node_table/node_table_model.h"
#include "controller_factory.h"
#include "node_serialization.h"
#include "remote/protocol_utils.h"
#include "remote/session_proxy.h"
#include "services/profile.h"
#include "services/property_defs.h"
#include "services/task_manager.h"

#if defined(UI_QT)
#include "controls/qt/grid.h"
#elif defined(UI_VIEWS)
#include "ui/views/controls/textfield/combo_textfield.h"
#endif

namespace {

const UINT kNodeTreeFormat =
    ::RegisterClipboardFormat(L"EFCAD60E-2623-4eef-8DE9-9B030DCD3AFE");

std::string _GetClipboardData() {
  size_t size = 0;
  char* data = nullptr;
  if (!Clipboard().GetData(kNodeTreeFormat, data, size))
    return std::string();

  std::string v(data, data + size);
  delete[] data;
  return v;
}

bool PasteRecords(TaskManager& task_manager, const scada::NodeId& parent_id) {
  auto buffer = _GetClipboardData();
  if (buffer.empty())
    return false;

  protocol::NodeTree message;
  if (!message.ParseFromString(buffer))
    return false;

  // TODO: Correct references.

  for (auto& packed_node : message.node()) {
    auto node_state = FromProto(packed_node);
    task_manager.PostInsertTask(
        {}, parent_id, node_state.type_definition_id,
        std::move(node_state.attributes), std::move(node_state.properties),
        [&task_manager, references = std::move(node_state.references)](
            const scada::Status& status, const scada::NodeId& node_id) {
          if (!status)
            return;

          for (auto& reference : references) {
            if (reference.forward)
              task_manager.PostAddReference(reference.reference_type_id,
                                            node_id, reference.node_id);
          }
        });
  }

  return true;
}

scada::NodeId GetSortCommandPropertyId(unsigned command_id) {
  switch (command_id) {
    case ID_SORT_NONE:
      return {};
    case ID_SORT_ALIAS:
      return id::DataItemType_Alias;
    case ID_SORT_CHANNEL:
      return id::DataItemType_Input1;
    default:
      assert(false);
      return {};
  }
}

}  // namespace

// NodeTableControllerImpl

template <scada::NumericId kNodeId>
class NodeTableControllerImpl : public NodeTableController {
 public:
  explicit NodeTableControllerImpl(const ControllerContext& context)
      : NodeTableController(context,
                            context.node_service_.GetNode(scada::NodeId{
                                kNodeId, NamespaceIndexes::SCADA})) {}
};

REGISTER_CONTROLLER(NodeTableControllerImpl<0>, ID_TABLE_EDITOR);
REGISTER_CONTROLLER(NodeTableControllerImpl<numeric_id::TsFormats>,
                    ID_TS_FORMATS_VIEW);
REGISTER_CONTROLLER(NodeTableControllerImpl<numeric_id::Users>, ID_USERS_VIEW);
REGISTER_CONTROLLER(NodeTableControllerImpl<numeric_id::SimulationSignals>,
                    ID_SIMULATION_ITEMS_VIEW);
REGISTER_CONTROLLER(NodeTableControllerImpl<numeric_id::HistoricalDatabases>,
                    ID_HISTORICAL_DB_VIEW);

// NodeTableController

NodeTableController::NodeTableController(const ControllerContext& context,
                                         const NodeRef& parent_node)
    : Controller{context},
      model_{std::make_unique<NodeTableModel>(context.node_service_,
                                              context.task_manager_)} {
  model_->SetParentNode(parent_node);
}

NodeTableController::~NodeTableController() {}

UiView* NodeTableController::Init(const WindowDefinition& definition) {
  if (const WindowItem* item = definition.FindItem("Item")) {
    std::string path = item->GetString("path");
    auto node_id = NodeIdFromScadaString(path);
    model_->SetParentNode(node_service_.GetNode(node_id));
  }

  model_->SetSorting(profile_.node_table.default_sort_property_id);

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
  if (auto parent_node = model_->parent_node()) {
    auto path = NodeIdToScadaString(parent_node.id());
    definition.AddItem("Item").SetString("path", path);
  }
}

#if defined(UI_VIEWS)
void NodeTableController::ShowContextMenu(gfx::Point point) {
  controller_delegate_.ShowPopupMenu(IDR_GRID_POPUP, point, true);
}
#endif

void NodeTableController::DeleteSelection() {
  if (!session_service_.HasPrivilege(scada::Privilege::Configure))
    return;

#if defined(UI_VIEWS)
  ui::GridRange range = table_->GetSelectionRange();
  if (range.empty())
    return;

  for (int i = range.row(); i <= range.last_row(); i++)
    DeleteTreeRecordsRecursive(task_manager_, model_->nodes()[i]);
#endif
}

void NodeTableController::CopyToClipboard() {
#if defined(UI_VIEWS)
  ui::GridRange range = table_->GetSelectionRange();
  if (range.empty())
    return;

  std::set<NodeRef> nodes;

  for (int i = range.row(); i <= range.last_row(); i++)
    nodes.emplace(model_->nodes()[i]);

  if (nodes.empty())
    return;

  auto parent_node = model_->parent_node();
  scada::NodeId parent_id =
      parent_node ? parent_node.id() : scada::id::RootFolder;

  std::vector<scada::NodeState> browse_nodes;

  for (auto& node : nodes) {
    browse_nodes.emplace_back();
    browse_nodes.back().parent_id = parent_id;
    browse_nodes.back().reference_type_id = scada::id::Organizes;
    NodeToData(node, browse_nodes.back());
  }

  protocol::NodeTree message;
  ContainerToProto(browse_nodes, *message.mutable_node());

  auto buffer = message.SerializePartialAsString();
  if (!Clipboard().SetData(kNodeTreeFormat, buffer.data(), buffer.size()))
    LOG(ERROR) << "Can't set clipboard data";
#endif
}

void NodeTableController::PasteFromClipboard() {
  if (!session_service_.HasPrivilege(scada::Privilege::Configure))
    return;

  auto parent_node = model_->parent_node();
  scada::NodeId parent_id =
      parent_node ? parent_node.id() : scada::id::RootFolder;
  if (!PasteRecords(task_manager_, parent_id))
    LOG(ERROR) << "Paste records error";
}

#if defined(UI_VIEWS)
bool NodeTableController::OnGridEditCellText(views::GridView& sender,
                                             int row,
                                             int column,
                                             const base::string16& text) {
  ui::GridRange range = table_->GetSelectionRange();
  if (range.empty())
    return true;

  for (int row = range.row(); row <= range.last_row(); row++)
    for (int col = range.column(); col <= range.last_column(); col++)
      model_->SetCellText(row, col, text);

  return true;
}

bool NodeTableController::CanEditCell(views::GridView& sender,
                                      int row,
                                      int column) {
  return true;
}

views::ComboTextfield* NodeTableController::OnGridCreateEditor(
    views::GridView& sender,
    int row,
    int column) {
  auto result = model_->GetCellEditor(row, column);
  if (result.type == PropertyEditor::NONE)
    return nullptr;

  views::ComboTextfield* editor =
      __super::OnGridCreateEditor(sender, row, column);
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
    case ID_RENAME:
    case ID_SORT_NONE:
    case ID_SORT_ALIAS:
    case ID_SORT_CHANNEL:
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

bool NodeTableController::IsCommandChecked(unsigned command_id) const {
  switch (command_id) {
    case ID_SORT_NONE:
    case ID_SORT_ALIAS:
    case ID_SORT_CHANNEL:
      return model_->sort_property_id() == GetSortCommandPropertyId(command_id);
    default:
      return false;
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
    case ID_RENAME:
#if defined(UI_VIEWS)
      if (auto selection = table_->GetSelectionRange(); !selection.empty())
        table_->OpenEditor(selection.row(), selection.column());
#endif
      break;
    case ID_SORT_NONE:
    case ID_SORT_ALIAS:
    case ID_SORT_CHANNEL:
      SetSorting(GetSortCommandPropertyId(command));
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
bool NodeTableController::OnKeyPressed(views::GridView& sender,
                                       ui::KeyboardCode key_code) {
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
  // TODO: Investigate why |row == 0| on empty table.
  // Left-click in table header to reproduce.
  if (row == -1 || row >= model_->nodes().size()) {
    selection().Clear();
    return;
  }

  auto node = model_->nodes()[row];
  assert(node);

  selection().SelectNode(node);
}

void NodeTableController::ShowHeaderContextMenu(gfx::Point point) {
  __super::ShowHeaderContextMenu(point);

  // TODO: Doesn't work.
}

void NodeTableController::OnGridColumnClicked(views::GridView& sender,
                                              int index) {
  __super::OnGridColumnClicked(sender, index);

  // TODO: Doesn't work.
}
#endif

void NodeTableController::SetSorting(const scada::NodeId& property_id) {
  profile_.node_table.default_sort_property_id = property_id;
  model_->SetSorting(property_id);
}
