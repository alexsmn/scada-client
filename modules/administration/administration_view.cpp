#include "administration/administration_view.h"

#include "administration/administration_sections.h"
#include "controller/command_handler.h"
#include "controller/controller_delegate.h"

#if !defined(UI_WT)
#include "administration/qt/administration_panel.h"
#endif

AdministrationView::AdministrationView(const ControllerContext& context)
    : ControllerContext{context} {}

AdministrationView::~AdministrationView() = default;

std::unique_ptr<UiView> AdministrationView::Init(
    const WindowDefinition& definition) {
#if !defined(UI_WT)
  auto panel = std::make_unique<AdministrationPanel>();

  // Availability is the shell's own answer, asked through the same resolution
  // the menu and the command palette use: a command with no handler is either
  // absent from this build or barred from this session (WIN_REQUIRES_ADMIN is
  // enforced there). Asking it here means the pane cannot drift from the
  // permission rule, and never offers a door that opens onto nothing.
  panel->ShowSections(ResolvableAdministrationSections(
      GetAdministrationSections(), [this](unsigned command_id) {
        return controller_delegate_.ResolveViewCommand(command_id) != nullptr;
      }));

  // Opening a section runs the shell's own command, so it lands in the
  // workspace exactly as the menu entry does.
  QObject::connect(panel.get(), &AdministrationPanel::SectionActivated,
                   panel.get(), [this](unsigned command_id) {
                     if (CommandHandler* handler =
                             controller_delegate_.ResolveViewCommand(
                                 command_id)) {
                       handler->ExecuteCommand(command_id);
                     }
                   });

  return std::unique_ptr<UiView>{panel.release()};
#else
  return nullptr;
#endif
}
