#include "components/favourites/favourites_view.h"

#include "common_resources.h"
#include "components/favourites/favourites_tree_model.h"
#include "controller_factory.h"
#include "controls/tree.h"

REGISTER_CONTROLLER(FavouritesView, ID_FAVOURITES_VIEW);

FavouritesView::FavouritesView(const ControllerContext& context)
    : Controller{context},
      favourites_tree_model_(new FavouritesTreeModel(favourites_)) {}

FavouritesView::~FavouritesView() {}

UiView* FavouritesView::Init(const WindowDefinition& definition) {
  tree_view_.reset(new Tree(*favourites_tree_model_));
  tree_view_->LoadIcons(IDB_WIN_TYPES, 16, UiColorRGB(255, 0, 255));
  tree_view_->SetDoubleClickHandler([this] { OpenSelection(); });

#if defined(UI_VIEWS)
  // TODO: controller_delegate_.ShowPopupMenu(IDR_FAVOR_POPUP, point, true);
  tree_view_->SetEditable(true);
#endif

  return tree_view_.get();
}

void FavouritesView::Save(WindowDefinition& definition) {
  // TODO: Save expanded nodes.
}

void FavouritesView::DeleteSelection() {
  FavouritesNode* node =
      reinterpret_cast<FavouritesNode*>(tree_view_->GetSelectedNode());
  if (node)
    node->Delete();
}

void FavouritesView::OpenSelection() {
  const FavouritesNode* node =
      reinterpret_cast<const FavouritesNode*>(tree_view_->GetSelectedNode());
  const FavouritesWindowNode* window_node = node ? node->AsWindowNode() : NULL;
  if (window_node)
    controller_delegate_.OpenView(window_node->window());
}

CommandHandler* FavouritesView::GetCommandHandler(unsigned command_id) {
  switch (command_id) {
    case ID_OPEN: {
      const FavouritesNode* node = reinterpret_cast<const FavouritesNode*>(
          tree_view_->GetSelectedNode());
      return node && node->AsWindowNode() ? this : NULL;
    }

    case ID_RENAME:
    case ID_DELETE:
      return tree_view_->GetSelectedNode() ? this : NULL;

    default:
      return __super::GetCommandHandler(command_id);
  }
}

void FavouritesView::ExecuteCommand(unsigned command) {
  switch (command) {
    case ID_OPEN:
      OpenSelection();
      break;

    case ID_RENAME:
      if (void* node = tree_view_->GetSelectedNode())
        tree_view_->StartEditing(node);
      break;

    case ID_DELETE:
      DeleteSelection();
      break;

    default:
      __super::ExecuteCommand(command);
      break;
  }
}
