#include "components/favourites/favourites_view.h"

#include "common_resources.h"
#include "components/favourites/favourites_tree_model.h"
#include "controller_factory.h"
#include "controls/tree.h"

const WindowInfo kWindowInfo = {ID_FAVOURITES_VIEW, "Favorites", L"Избранное",
                                WIN_SING,           200,         400};

REGISTER_CONTROLLER(FavouritesView, kWindowInfo);

FavouritesView::FavouritesView(const ControllerContext& context)
    : Controller{context},
      favourites_tree_model_(new FavouritesTreeModel(favourites_)) {}

FavouritesView::~FavouritesView() {}

UiView* FavouritesView::Init(const WindowDefinition& definition) {
  tree_view_.reset(new Tree(*favourites_tree_model_));
  tree_view_->LoadIcons(IDB_WIN_TYPES, 16, UiColorRGB(255, 0, 255));
  tree_view_->SetDoubleClickHandler([this] { OpenSelection(); });
  tree_view_->SetContextMenuHandler([this](const UiPoint& point) {
    controller_delegate_.ShowPopupMenu(IDR_FAVOR_POPUP, point, true);
  });

#if defined(UI_VIEWS)
  tree_view_->SetEditable(true);
#endif

  open_command_.enabled_handler = [this] {
    auto* node =
        static_cast<const FavouritesNode*>(tree_view_->GetSelectedNode());
    return node && node->AsWindowNode();
  };
  open_command_.execute_handler = [this] { OpenSelection(); };

  Command::EnabledHandler selection_enabled_handler = [this] {
    return tree_view_->GetSelectedNode() != nullptr;
  };

  rename_command_.enabled_handler = selection_enabled_handler;
  rename_command_.execute_handler = [this] {
    if (void* node = tree_view_->GetSelectedNode())
      tree_view_->StartEditing(node);
  };

  delete_command_.enabled_handler = selection_enabled_handler;
  delete_command_.execute_handler = [this] { DeleteSelection(); };

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
  return command_handler_.GetCommandHandler(command_id);
}
