#pragma once

#include "base/win/scoped_comptr.h"
#include "base/win/scoped_variant.h"

class ExcelSheetModel {
 public:
  ExcelSheetModel() : rows(0), cols(0) {}

  void SetDataSize(int rows, int cols);

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

  base::win::ScopedComPtr<IDispatch> GetRange(const wchar_t* name);

  base::win::ScopedComPtr<IDispatch> excel;
  base::win::ScopedComPtr<IDispatch> workbook;
  base::win::ScopedComPtr<IDispatch> sheet;
};