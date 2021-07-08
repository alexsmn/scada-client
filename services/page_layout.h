#pragma once

#include "value_util.h"

#include <cassert>
#include <vector>

class PageLayoutBlock {
 public:
  enum Type { SPLIT, PANE };

  PageLayoutBlock() {}

  PageLayoutBlock(const PageLayoutBlock& src)
      : type(src.type),
        horz(src.horz),
        pos(src.pos),
        active_window(src.active_window),
        central(src.central) {
    if (type == SPLIT) {
      left = std::make_unique<PageLayoutBlock>(*src.left);
      right = std::make_unique<PageLayoutBlock>(*src.right);
    } else {
      wins = src.wins;
    }
  }

  bool empty() const { return type == PANE && wins.empty(); }

  PageLayoutBlock& top() {
    assert(type == SPLIT);
    assert(left);
    return *left;
  }

  PageLayoutBlock& bottom() {
    assert(type == SPLIT);
    assert(right);
    return *right;
  }

  void add(int window_id) {
    assert(type == PANE);
    wins.push_back(window_id);
  }

  void split(bool horizontally) {
    assert(wins.empty());
    if (type != SPLIT) {
      type = SPLIT;
      left = std::make_unique<PageLayoutBlock>();
      right = std::make_unique<PageLayoutBlock>();
    }
    horz = horizontally;
  }

  PageLayoutBlock& operator=(const PageLayoutBlock& src) {
    if (&src == this)
      return *this;

    if (src.type == SPLIT) {
      if (type == SPLIT) {
        *left = *src.left;
        *right = *src.right;
      } else {
        left = std::make_unique<PageLayoutBlock>(*src.left);
        right = std::make_unique<PageLayoutBlock>(*src.right);
      }
      horz = src.horz;
      pos = src.pos;

    } else {
      if (type == SPLIT) {
        left.reset();
        right.reset();
        left = 0;
        right = 0;
      }
      wins = src.wins;
      active_window = src.active_window;
    }

    type = src.type;
    central = src.central;
    return *this;
  }

  bool IsCentral() const {
    return central || (left && left->IsCentral()) ||
           (right && right->IsCentral());
  }

  bool operator==(const PageLayoutBlock& other) const {
    if (type != other.type)
      return false;
    if (central != other.central)
      return false;

    if (type == PANE) {
      if (wins != other.wins)
        return false;
      if (active_window != other.active_window)
        return false;
      return true;

    } else {
      if (horz != other.horz)
        return false;
      if (pos != other.pos)
        return false;
      return *left == *other.left && *right == *other.right;
    }
  }

  Type type = PANE;
  bool horz = false;
  std::unique_ptr<PageLayoutBlock> left;
  std::unique_ptr<PageLayoutBlock> right;
  int pos = -1;
  std::vector<int> wins;
  int active_window = -1;
  bool central = false;
};

class PageLayout {
 public:
  struct Dock : public PageLayoutBlock {
    int size;
    int place;
    Dock() : size(0), place(0) {}
  };

  // Warning: sequence corresponds to docking::DockSide.
  enum DockSide { BOTTOM, TOP, LEFT, RIGHT };

  PageLayoutBlock main;
  Dock dock[4];
  std::string blob;

  void Clear() {
    main = PageLayoutBlock();
    for (int i = 0; i < 4; i++)
      dock[i] = Dock();
  }

  bool operator==(const PageLayout& other) const noexcept {
    return main == other.main &&
           std::equal(std::begin(dock), std::end(dock),
                      std::begin(other.dock)) &&
           blob == other.blob;
  }
};

base::Value ToJson(const PageLayout& layout);

template <>
std::optional<PageLayout> FromJson(const base::Value& json);