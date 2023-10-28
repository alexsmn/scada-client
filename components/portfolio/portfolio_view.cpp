#include "components/portfolio/portfolio_view.h"

#include "aui/tree.h"
#include "common_resources.h"
#include "components/portfolio/portfolio_tree_model.h"
#include "components/select_item/select_item_dialog.h"
#include "controller/controller_delegate.h"
#include "node_service/node_service.h"
#include "controller/selection_model.h"

// PortfolioView

PortfolioView::PortfolioView(const ControllerContext& context)
    : ControllerContext{context},
      model_{std::make_shared<PortfolioTreeModel>(context.node_service_,
                                                  context.portfolio_manager_)} {
  selection_.multiple_handler = [this] { return GetSelectedNodeIdList(); };
}

PortfolioView::~PortfolioView() {}

UiView* PortfolioView::Init(const WindowDefinition& definition) {
  tree_ = new aui::Tree{model_};
  tree_->SetRootVisible(false);
  tree_->LoadIcons(IDB_ITEMS, 16, aui::Rgba{255, 0, 255});

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
      selection_.Clear();
    else if (node->is_portfolio())
      selection_.SelectMultiple();
    else
      selection_.SelectNode(node_service_.GetNode(node->item_id()));
  });

  tree_->SetContextMenuHandler([this](const aui::Point& point) {
    controller_delegate_.ShowPopupMenu(nullptr, IDR_PFOLIO_POPUP, point, true);
  });

  command_registry_.AddCommand(
      Command{ID_RENAME}
          .set_execute_handler([this] {
            PortfolioTreeNode* node = model_->AsNode(tree_->GetSelectedNode());
            if (node->is_portfolio())
              tree_->StartEditing(node);
          })
          .set_enabled_handler([this] {
            PortfolioTreeNode* node = model_->AsNode(tree_->GetSelectedNode());
            return node && node->is_portfolio();
          }));

  command_registry_.AddCommand(
      Command{ID_DELETE}
          .set_execute_handler([this] { DeleteSelection(); })
          .set_enabled_handler([this] { return !!tree_->GetSelectedNode(); }));

  command_registry_.AddCommand(Command{ID_NEW_PORTFOLIO}.set_execute_handler(
      [this] { NewPortfolio(); }));

  command_registry_.AddCommand(
      Command{ID_ADD_ITEMS}
          .set_execute_handler([this] { AddItemsToPortfolio(); })
          .set_enabled_handler([this] { return !!GetSelectedPortfolio(); }));

  return tree_;
}

void PortfolioView::DeleteSelection() {
  PortfolioTreeNode* node = model_->AsNode(tree_->GetSelectedNode());
  if (node) {
    if (node->is_portfolio())
      portfolio_manager_.Delete(node->portfolio());
    else
      portfolio_manager_.DeleteItem(node->portfolio(), node->item_id());
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

  for (auto& trid : RunSelectItemsDialog(dialog_service_, node_service_))
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

std::u16string PortfolioView::GetSelectionTitle() {
  PortfolioTreeNode* node = model_->AsNode(tree_->GetSelectedNode());
  return node ? node->GetText(0) : std::u16string();
}

NodeIdSet PortfolioView::GetSelectedNodeIdList() {
  NodeIdSet node_ids;

  PortfolioTreeNode* node = model_->AsNode(tree_->GetSelectedNode());
  if (!node)
    return node_ids;

  if (node->is_portfolio())
    node_ids = node->portfolio().items;
  else
    node_ids.insert(node->item_id());

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
  return command_registry_.GetCommandHandler(command_id);
}
