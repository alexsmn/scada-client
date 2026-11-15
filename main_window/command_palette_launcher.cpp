#include "main_window/command_palette_launcher.h"

#include "aui/translation.h"
#include "main_window/command_palette_qt.h"
#include "main_window/tag_search_index.h"
#include "scada/session_service.h"
#include "scada/standard_node_ids.h"

#include <QString>

#include <utility>
#include <vector>

CommandPaletteLauncher::CommandPaletteLauncher(
    CommandPaletteLauncherContext&& context)
    : executor_{context.executor_},
      command_manager_{context.command_manager_},
      resolve_command_{std::move(context.resolve_command_)},
      open_tag_{std::move(context.open_tag_)},
      parent_{context.parent_},
      session_service_{context.session_service_} {
  if (!context.node_service_)
    return;

  // Built here but deliberately NOT started here; see StartTagSearchBrowse().
  tag_search_index_ = std::make_unique<TagSearchIndex>(
      executor_, *context.node_service_, scada::id::ObjectsFolder);

  // The index invalidates itself on model changes, but a re-login is not one:
  // OnLoginCompleted swaps the services under the same window, so without
  // this the palette kept listing the previous session's tags.
  if (context.session_service_) {
    session_state_connection_ =
        context.session_service_->SubscribeSessionStateChanged(
            [this](bool connected, const scada::Status&) {
              if (connected)
                tag_search_index_->Reset();
            });
  }
}

CommandPaletteLauncher::~CommandPaletteLauncher() = default;

bool CommandPaletteLauncher::has_tag_search() const {
  // Asked each time rather than at construction: the window outlives a
  // re-login, which can land on a different kind of server. On a foreign one
  // the walk would browse its whole Organizes tree and find nothing.
  return tag_search_index_ &&
         (!session_service_ || session_service_->IsScada());
}

void CommandPaletteLauncher::StartTagSearchBrowse() {
  if (has_tag_search())
    tag_search_index_->EnsurePopulated();
}

CommandPalette* CommandPaletteLauncher::Show(const QString& initial_text) {
  // Address-space tags as extra palette entries; activating one opens it. The
  // browse normally started when the first page opened, so tags() is usually
  // already populated here.
  std::vector<CommandPalette::ExtraItem> extras;
  if (has_tag_search()) {
    StartTagSearchBrowse();
    const std::u16string tag_detail = Translate("tag");
    for (const TagSearchIndex::Tag& tag : tag_search_index_->tags()) {
      extras.push_back({tag.name, tag_detail,
                        [open_tag = open_tag_, node_id = tag.node_id,
                         title = tag.name] { open_tag(node_id, title); }});
    }
  }

  auto* palette = new CommandPalette(&parent_, command_manager_,
                                     resolve_command_, std::move(extras));
  palette->setAttribute(Qt::WA_DeleteOnClose);
  if (!initial_text.isEmpty())
    palette->PresetFilter(initial_text);
  palette->show();
  palette->raise();
  palette->activateWindow();
  return palette;
}
