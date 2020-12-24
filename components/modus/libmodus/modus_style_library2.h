#pragma once

#include <memory>
#include <vector>

class BlinkerManager;
class ModusStyle2;

enum class StyleId {
  INVAL,
  INACT,
  BADQ,
  ALERT,
  COUNT
};

class ModusStyleLibrary2 {
 public:
  explicit ModusStyleLibrary2(BlinkerManager& blinker_manager);
  ~ModusStyleLibrary2();

  ModusStyle2* GetStyle(StyleId id);

 private:
  void set_style(StyleId id, std::unique_ptr<ModusStyle2> style);

  std::vector<std::unique_ptr<ModusStyle2>> styles_;
};