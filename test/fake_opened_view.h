#pragma once

#include "base/awaitable.h"
#include "controller/window_info.h"
#include "main_window/opened_view/opened_view_interface.h"
#include "profile/window_definition.h"

#include <optional>
#include <string>
#include <string_view>

// A stand-in for an open workspace view, for tests that need the active view to
// exist without opening one.
//
// It replaces three near-identical fakes that each implemented the same seven
// `OpenedViewInterface` methods with slightly different stubs, so it carries
// the union of what they needed rather than the intersection: a title a test
// can set and read back, a `WindowInfo` that makes the view a pane, and a
// `GetOpenWindowDefinition` that can either echo the `WindowInfo` it is asked
// about — the default, and what a caller reopening a view sees — or report a
// definition the test supplied, while counting how often it was awaited.
//
// `Save()` reports an empty definition, which is all three callers wanted. A
// test that needs a real one adds the setter together with the assertion that
// reads it, rather than leaving unexercised surface here.
class FakeOpenedView : public OpenedViewInterface {
 public:
  FakeOpenedView() = default;

  explicit FakeOpenedView(WindowInfo window_info)
      : window_info_{std::move(window_info)} {}

  // `open_definition` is what `GetOpenWindowDefinition` reports, for a test
  // that cares what the view would reopen as rather than merely that it was
  // asked.
  FakeOpenedView(WindowDefinition open_definition, WindowInfo window_info)
      : window_info_{std::move(window_info)},
        open_definition_{std::move(open_definition)} {}

  const WindowInfo& GetWindowInfo() const override { return window_info_; }

  std::u16string GetWindowTitle() const override { return window_title_; }

  void SetWindowTitle(std::u16string_view title) override {
    window_title_ = title;
  }

  WindowDefinition Save() override { return {}; }

  ContentsModel* GetContents() override { return nullptr; }

  void Select(const scada::NodeId& /*node_id*/) override {}

  Awaitable<WindowDefinition> GetOpenWindowDefinition(
      const WindowInfo* window_info) const override {
    ++open_definition_await_count;
    if (open_definition_)
      co_return *open_definition_;
    co_return WindowDefinition{*window_info};
  }

  // Mutable so a const `GetOpenWindowDefinition` can record the await, which is
  // the point of counting it: the coroutine path this fake exists to cover was
  // once dropped on the floor, and a discarded awaitable never increments.
  mutable int open_definition_await_count = 0;

 private:
  WindowInfo window_info_;
  std::u16string window_title_;
  std::optional<WindowDefinition> open_definition_;
};
