#include "favorites/favourites_view.h"

#include "aui/dialog_service.h"
#include "aui/translation.h"
#include "aui/prompt_dialog.h"
#include "aui/tree.h"
#include "common_resources.h"
#include "controller/controller_delegate.h"
#include "favorites/favourites.h"
#include "favorites/favourites_tree_model.h"
#include "web/web_util.h"

#if !defined(UI_WT)
#include "components/web/web_component.h"
#endif

namespace {
const char16_t kAddUrl[] = u"Add Web Page";
}

FavouritesView::FavouritesView(const ControllerContext& context,
                               Favourites& favorites)
    : ControllerContext{context},
      favourites_{favorites},
      favourites_tree_model_{std::make_shared<FavouritesTreeModel>(favorites)} {
}

FavouritesView::~FavouritesView() {}

std::unique_ptr<UiView> FavouritesView::Init(
    const WindowDefinition& definition) {
  tree_view_ = new aui::Tree{favourites_tree_model_};
  tree_view_->LoadIcons(IDB_WIN_TYPES, 16, aui::Rgba{255, 0, 255});
  tree_view_->SetDoubleClickHandler([this] { OpenSelection(); });
  tree_view_->SetContextMenuHandler([this](const aui::Point& point) {
    controller_delegate_.ShowPopupMenu(nullptr, IDR_FAVOR_POPUP, point, true);
  });

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

#if !defined(UI_WT)
  add_url_command_.execute_handler = [this] { AddUrl(); };
#endif

  return std::unique_ptr<UiView>{tree_view_};
}

void FavouritesView::Save(WindowDefinition& definition) {
  // TODO: Save expanded nodes.
}

void FavouritesView::DeleteSelection() {
  FavouritesNode* node =
      static_cast<FavouritesNode*>(tree_view_->GetSelectedNode());
  if (node)
    node->Delete();
}

void FavouritesView::OpenSelection() {
  const FavouritesNode* node =
      static_cast<const FavouritesNode*>(tree_view_->GetSelectedNode());
  const FavouritesWindowNode* window_node = node ? node->AsWindowNode() : NULL;
  if (window_node) {
    controller_delegate_.OpenView(window_node->window_def());
  }
}

CommandHandler* FavouritesView::GetCommandHandler(unsigned command_id) {
  return command_registry_.GetCommandHandler(command_id);
}

#if !defined(UI_WT)
promise<> FavouritesView::AddUrl() {
  return RunPromptDialog(dialog_service_, Translate("URL:"), kAddUrl)
      .then([this](const std::u16string& url) {
        if (!IsWebUrl(url)) {
          return ToRejectedPromise(dialog_service_.RunMessageBox(
              Translate("A valid URL must start with \"http://\" or "
              "\"https://\"."),
              kAddUrl, MessageBoxMode::Error));
        }

        const FavouritesNode* node =
            static_cast<const FavouritesNode*>(tree_view_->GetSelectedNode());
        if (node && !node->AsFolderNode())
          node = node->parent();

        const Page& folder = node && node->AsFolderNode()
                                 ? node->AsFolderNode()->folder()
                                 : favourites_.GetOrAddFolder();

        WindowDefinition win{kWebWindowInfo};
        win.path = url;
        favourites_.Add(win, folder);

        return make_resolved_promise();
      });
}
#endif
