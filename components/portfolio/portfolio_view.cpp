#include "components/portfolio/portfolio_view.h"

#include "commands/select_item_dialog.h"
#include "common_resources.h"
#include "components/portfolio/portfolio_tree_model.h"
#include "controller_factory.h"
#include "controls/tree.h"
#include "selection_model.h"

REGISTER_CONTROLLER(PortfolioView, ID_PORTFOLIO_VIEW);

// PortfolioView

PortfolioView::PortfolioView(const ControllerContext& context)
    : Controller(context),
      model_(std::make_unique<PortfolioTreeModel>(context.portfolio_manager_,
                                                  context.node_service_)) {
  selection().multiple_handler_ = [this] { return GetSelectedNodeIdList(); };
}

PortfolioView::~PortfolioView() {}

UiView* PortfolioView::Init(const WindowDefinition& definition) {
  tree_.reset(new Tree(*model_));
  tree_->SetRootVisible(false);
  tree_->SetEditable(true);
  tree_->LoadIcons(IDB_ITEMS, 16, UiColorRGB(255, 0, 255));

  tree_->SetDoubleClickHandler([this] {
    PortfolioTreeNode* node = model_->AsNode(tree_->GetSelectedNode());
    if (node) {
      tree_->SelectNode(node);
      // TODO: Implement double-click processing in common way.
    }
  });

  tree_->SetSelectionChangedHandler([this] {
    PortfolioTreeNode* node = model_->AsNode(tree_->GetSelectedNode());
    // |node| may be either portfolio or item. For portfolio |item_id()| has
    // value of |TRID_INVAL|.
    if (!node)
      selection().Clear();
    else if (node->is_portfolio())
      selection().SelectMultiple();
    else
      selection().SelectNode(node->node);
  });

  tree_->SetEditHandler(
      [this](void* node) { return model_->AsNode(node)->is_portfolio(); });

  return tree_.get();
}

/*void PortfolioView::OnShowContextMenu(views::TreeView& sender,
                                      const gfx::Point& point) {
  controller_delegate_.ShowPopupMenu(IDR_PFOLIO_POPUP, point, true);
}*/

void PortfolioView::DeleteSelection() {
  PortfolioTreeNode* node = model_->AsNode(tree_->GetSelectedNode());
  if (node) {
    if (node->is_portfolio())
      portfolio_manager_.Delete(node->portfolio());
    else
      portfolio_manager_.DeleteItem(node->portfolio(), node->node.id());
  }
}

const Portfolio* PortfolioView::GetSelectedPortfolio() const {
  PortfolioTreeNode* node = model_->AsNode(tree_->GetSelectedNode());
  if (!node)
    return NULL;
  if (!node->is_portfolio())
    node = node->parent();
  return &node->portfolio();
}

void PortfolioView::AddItemsToPortfolio() {
  const Portfolio* portfolio = GetSelectedPortfolio();
  if (!portfolio)
    return;

  auto items = RunSelectItemsDialog(dialog_service_, node_service_);
  for (auto& trid : items)
    portfolio_manager_.AddItem(*portfolio, trid);
}

void PortfolioView::AddContainedItem(const scada::NodeId& node_id,
                                     unsigned flags) {
  const Portfolio* portfolio = GetSelectedPortfolio();
  if (!portfolio)
    return;

  portfolio_manager_.AddItem(*portfolio, node_id);

  PortfolioTreeNode* portfolio_node = model_->FindPortfolioNode(*portfolio);
  assert(portfolio_node);

  PortfolioTreeNode* item_node = model_->FindItemNode(*portfolio_node, node_id);
  assert(item_node);

  // TODO: Expand portfolio node.
  // TODO: Select and make item node visible.

  tree_->SelectNode(item_node);
}

base::string16 PortfolioView::GetSelectionTitle() {
  PortfolioTreeNode* node = model_->AsNode(tree_->GetSelectedNode());
  return node ? node->GetText(0) : base::string16();
}

NodeIdSet PortfolioView::GetSelectedNodeIdList() {
  NodeIdSet node_ids;

  PortfolioTreeNode* node = model_->AsNode(tree_->GetSelectedNode());
  if (!node)
    return node_ids;

  if (node->is_portfolio())
    node_ids = node->portfolio().items;
  else
    node_ids.insert(node->node.id());

  return node_ids;
}

void PortfolioView::NewPortfolio() {
  Portfolio& portfolio = portfolio_manager_.New();

  PortfolioTreeNode* node = model_->FindPortfolioNode(portfolio);
  assert(node);

  tree_->SelectNode(node);
  tree_->StartEditing(node);
}

CommandHandler* PortfolioView::GetCommandHandler(unsigned command_id) {
  switch (command_id) {
    case ID_RENAME:
    case ID_DELETE:
    case ID_NEW_PORTFOLIO:
    case ID_ADD_ITEMS:
      return this;
  }

  return __super::GetCommandHandler(command_id);
}

bool PortfolioView::IsCommandEnabled(unsigned command_id) const {
  switch (command_id) {
    case ID_NEW_PORTFOLIO:
      return true;
    case ID_RENAME: {
      PortfolioTreeNode* node = model_->AsNode(tree_->GetSelectedNode());
      return node && node->is_portfolio();
    }
    case ID_DELETE:
      return tree_->GetSelectedNode() != NULL;
    case ID_ADD_ITEMS:
      return GetSelectedPortfolio() != NULL;
    default:
      return __super::IsCommandEnabled(command_id);
  }
}

void PortfolioView::ExecuteCommand(unsigned command) {
  switch (command) {
    case ID_RENAME: {
      PortfolioTreeNode* node = model_->AsNode(tree_->GetSelectedNode());
      if (node->is_portfolio())
        tree_->StartEditing(node);
      break;
    }
    case ID_DELETE:
      DeleteSelection();
      break;
    case ID_NEW_PORTFOLIO:
      NewPortfolio();
      break;
    case ID_ADD_ITEMS:
      AddItemsToPortfolio();
      break;
    default:
      __super::ExecuteCommand(command);
      break;
  }
}
