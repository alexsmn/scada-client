#pragma once

#include "base/win/scoped_variant.h"

#include <wrl/client.h>

class ExcelSheetModel {
 public:
  ExcelSheetModel() : rows(0), cols(0) {}

  void SetDataSize(int rows, int cols);

  void SetData(int row, int col, base::win::ScopedVariant&& val);
  void SetData(int row, int col, const VARIANT& val);

  base::win::ScopedVariant data;
  int rows;
  int cols;
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
