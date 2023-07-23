#include "components/favourites/favourites_view.h"

#include "client_utils.h"
#include "common_resources.h"
#include "components/favourites/favourites_tree_model.h"
#include "components/prompt/prompt_dialog.h"
#include "controller_delegate.h"
#include "aui/tree.h"
#include "services/dialog_service.h"

#if !defined(UI_WT)
#include "components/web/web_component.h"
#endif

namespace {
const char16_t kAddUrl[] = u"Добавить Web-страницу";
}

FavouritesView::FavouritesView(const ControllerContext& context)
    : ControllerContext{context},
      favourites_tree_model_{new FavouritesTreeModel(favourites_)} {}

FavouritesView::~FavouritesView() {}

UiView* FavouritesView::Init(const WindowDefinition& definition) {
  tree_view_ = new aui::Tree{favourites_tree_model_};
  tree_view_->LoadIcons(IDB_WIN_TYPES, 16, aui::Rgba{255, 0, 255});
  tree_view_->SetDoubleClickHandler([this] { OpenSelection(); });
  tree_view_->SetContextMenuHandler([this](const aui::Point& point) {
    controller_delegate_.ShowPopupMenu(IDR_FAVOR_POPUP, point, true);
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

  return tree_view_;
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
  if (window_node)
    controller_delegate_.OpenView(window_node->window());
}

CommandHandler* FavouritesView::GetCommandHandler(unsigned command_id) {
  return command_registry_.GetCommandHandler(command_id);
}

#if !defined(UI_WT)
promise<> FavouritesView::AddUrl() {
  return RunPromptDialog(dialog_service_, u"URL-адрес:", kAddUrl)
      .then([this](const std::u16string& url) {
        if (!IsWebUrl(url)) {
          return ToRejectedPromise(dialog_service_.RunMessageBox(
              u"Допустимый URL-адрес должен начинаться с \"http://\" или "
              u"\"https://\".",
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
