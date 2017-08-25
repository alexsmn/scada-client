#pragma once

#include <memory>

namespace modus {
class MasterLibrary;
}

class ModusStyleLibrary2;

class ModusModule2 {
 public:
  ModusModule2();
  ~ModusModule2();

  ModusModule2(const ModusModule2&) = delete;
  ModusModule2& operator=(const ModusModule2&) = delete;

  modus::MasterLibrary& master_library() { return *master_library_; }
  ModusStyleLibrary2& style_library() { return *style_library_; }

  static ModusModule2* GetInstance() { return s_instance; }
  static void SetInstance(ModusModule2* instance) { s_instance = instance; }

 private:
  std::unique_ptr<modus::MasterLibrary> master_library_;
  std::unique_ptr<ModusStyleLibrary2> style_library_;

  static ModusModule2* s_instance;
};