#include "aui/drag_drop_types.h"

#include <oleidl.h>

namespace aui {

int ui::DragDropTypes::DropEffectToDragOperation(uint32_t effect) {
  int drag_operation = DRAG_NONE;
  if (effect & DROPEFFECT_LINK)
    drag_operation |= DRAG_LINK;
  if (effect & DROPEFFECT_COPY)
    drag_operation |= DRAG_COPY;
  if (effect & DROPEFFECT_MOVE)
    drag_operation |= DRAG_MOVE;
  return drag_operation;
}

uint32_t ui::DragDropTypes::DragOperationToDropEffect(int drag_operation) {
  uint32_t drop_effect = DROPEFFECT_NONE;
  if (drag_operation & DRAG_LINK)
    drop_effect |= DROPEFFECT_LINK;
  if (drag_operation & DRAG_COPY)
    drop_effect |= DROPEFFECT_COPY;
  if (drag_operation & DRAG_MOVE)
    drop_effect |= DROPEFFECT_MOVE;
  return drop_effect;
}

}  // namespace aui
