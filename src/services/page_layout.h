#pragma once

#include <cassert>
#include <vector>

class PageLayoutBlock
{
public:
  enum Type { SPLIT, PANE };

  PageLayoutBlock() : type(PANE), left(0), right(0), horz(false), pos(-1),
      active_window(-1), central(false) { }

  PageLayoutBlock(const PageLayoutBlock& src) : type(src.type), left(0), right(0),
      horz(src.horz), pos(src.pos), active_window(src.active_window),
      central(src.central)
  {
    if (type == SPLIT) {
      left = new PageLayoutBlock(*src.left);
      right = new PageLayoutBlock(*src.right);
    } else {
      wins = src.wins;
    }
  }

  ~PageLayoutBlock()
  {
    if (type == SPLIT) {
      delete left; 
      delete right;
    }
  }

  bool empty() const { return type == PANE && wins.empty(); }
  PageLayoutBlock& top() { assert(left); return *left; }
  PageLayoutBlock& bottom() { assert(right); return *right; }

  void add(int window_id)
  {
    assert(type == PANE);
    wins.push_back(window_id);
  }

  void split(bool horizontally)
  {
    assert(wins.empty());
    if (type != SPLIT) {
      type = SPLIT;
      left = new PageLayoutBlock();
      right = new PageLayoutBlock();
    }
    horz = horizontally;
  }

  PageLayoutBlock& operator=(const PageLayoutBlock& src) {
    if (src.type == SPLIT) {
      if (type == SPLIT) {
        *left = *src.left;
        *right = *src.right;
      } else {
        left = new PageLayoutBlock(*src.left);
        right = new PageLayoutBlock(*src.right);
      }
      horz = src.horz;
      pos = src.pos;
    } else {
      if (type == SPLIT) {
        delete left;
        delete right;
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

  Type				type;
  bool				horz;
  PageLayoutBlock*	left;
  PageLayoutBlock*	right;
  int					pos;
  std::vector<int>	wins;
  int					active_window;
  bool central;
};

class PageLayout
{
public:
  struct Dock : public PageLayoutBlock
  {
    int		size;
    int		place;
    Dock() : size(0), place(0) { }
  };

  // Warning: sequence corresponds to docking::DockSide.
  enum DockSide { BOTTOM, TOP, LEFT, RIGHT };

  PageLayoutBlock	main;
  Dock	dock[4];

  void Clear() {
    main = PageLayoutBlock();
    for (int i = 0; i < 4; i++)
      dock[i] = Dock();
  }
};
