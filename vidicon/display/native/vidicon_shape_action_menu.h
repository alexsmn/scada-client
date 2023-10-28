#pragma once

#include "aui/models/simple_menu_model.h"
#include "vidicon/display/native/vidicon_display_lib.h"

#include <boost/locale/encoding_utf.hpp>

struct VidiconShapeActionMenu : private aui::SimpleMenuModel::Delegate {
  VidiconShapeActionMenu(const vidicon::shape& shape,
                         std::span<const vidicon::shape_action> actions)
      : shape{shape} {
    int action_index = 0;
    for (const auto& action : actions) {
      auto title = boost::locale::conv::utf_to_utf<char16_t>(action.title);
      if (action.checked) {
        menu_model.AddCheckItem(/*command_id*/ action_index, title);
      } else {
        menu_model.AddItem(/*command_id*/ action_index, title);
      }
      ++action_index;
    }
  }

  virtual bool IsCommandIdChecked(int command_id) const override {
    // TODO: Implement.
    return false;
  }

  virtual bool IsCommandIdEnabled(int command_id) const override {
    // TODO: Implement.
    return true;
  }

  virtual void ExecuteCommand(int command_id) override {
    // `command_id` represents action index.
    shape.exec_action(/*action_index*/ command_id);
  }

  const vidicon::shape& shape;
  aui::SimpleMenuModel menu_model{this};
};
