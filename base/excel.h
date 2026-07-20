#pragma once

#include <string>

#ifdef _WIN32
#include "base/win/scoped_variant.h"
#include <wrl/client.h>
#else
using HRESULT = int;
using DATE = double;
struct VARIANT {};
struct IDispatch {};
namespace scada::base::win {
class ScopedVariant {
 public:
  void Reset() {}
  template <class T>
  void Set(const T&) {}
};
}  // namespace scada::base::win
namespace Microsoft::WRL {
template <class T>
class ComPtr {
 public:
  T* Get() const { return nullptr; }
};
}  // namespace Microsoft::WRL
#endif

class ExcelSheetModel {
 public:
  ExcelSheetModel() {}
  ExcelSheetModel(int rows, int cols) { SetDataSize(rows, cols); }

  void SetDataSize(int rows, int cols);

  void SetData(int row, int col, scada::base::win::ScopedVariant&& val);
  void SetData(int row, int col, const VARIANT& val);
  void SetData(int row, int col, const std::wstring& val);

  scada::base::win::ScopedVariant data;
  int rows = 0;
  int cols = 0;
};

class Excel {
 public:
  Excel();

  void SetVisible(bool visible = true);

  void NewWorkbook();

  void NewSheet(const ExcelSheetModel& sheet);

  Microsoft::WRL::ComPtr<IDispatch> GetRange(const wchar_t* name);

  Microsoft::WRL::ComPtr<IDispatch> excel;
  Microsoft::WRL::ComPtr<IDispatch> workbook;
  Microsoft::WRL::ComPtr<IDispatch> sheet;
};
