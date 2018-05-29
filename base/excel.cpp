#include "base/excel.h"

#include <cassert>
#include <vector>

#include "base/strings/stringprintf.h"

// #import "C:\Program Files (x86)\Common Files\Microsoft
// Shared\OFFICE12\MSO.DLL" rename_namespace("A") #import "C:\Program Files
// (x86)\Microsoft Office\Office12\EXCEL.EXE" rename_namespace("A")

namespace {

HRESULT AutoWrap(int autoType,
                 VARIANT* pvResult,
                 IDispatch* pDisp,
                 LPOLESTR ptName,
                 std::vector<VARIANT> args = {}) {
  if (!pDisp)
    return E_POINTER;

  // Variables used...
  DISPPARAMS dp = {NULL, NULL, 0, 0};
  DISPID dispidNamed = DISPID_PROPERTYPUT;
  DISPID dispID;
  HRESULT hr;
  char szName[200];

  // Convert down to ANSI
  WideCharToMultiByte(CP_ACP, 0, ptName, -1, szName, 256, NULL, NULL);

  // Get DISPID for name passed...
  hr = pDisp->GetIDsOfNames(IID_NULL, &ptName, 1, LOCALE_USER_DEFAULT, &dispID);
  if (FAILED(hr)) {
    // sprintf(buf, "IDispatch::GetIDsOfNames(\"%s\") failed w/err 0x%08lx",
    // szName, hr); MessageBox(NULL, buf, "AutoWrap()", 0x10010); _exit(0);
    return hr;
  }

  // Build DISPPARAMS
  dp.cArgs = args.size();
  dp.rgvarg = args.data();

  // Handle special-case for property-puts!
  if (autoType & DISPATCH_PROPERTYPUT) {
    dp.cNamedArgs = 1;
    dp.rgdispidNamedArgs = &dispidNamed;
  }

  // Make the call!
  hr = pDisp->Invoke(dispID, IID_NULL, LOCALE_SYSTEM_DEFAULT, autoType, &dp,
                     pvResult, NULL, NULL);
  if (FAILED(hr)) {
    // sprintf(buf, "IDispatch::Invoke(\"%s\"=%08lx) failed w/err 0x%08lx",
    // szName, dispID, hr); MessageBox(NULL, buf, "AutoWrap()", 0x10010);
    //_exit(0);
    return hr;
  }

  return hr;
}

Microsoft::WRL::ComPtr<IDispatch> GetProperty(IDispatch& object,
                                              const wchar_t* property_name) {
  base::win::ScopedVariant variant;
  AutoWrap(DISPATCH_PROPERTYGET, variant.Receive(), &object,
           const_cast<LPOLESTR>(property_name));
  return Microsoft::WRL::ComPtr<IDispatch>(variant.AsInput()->pdispVal);
}

Microsoft::WRL::ComPtr<IDispatch> GetIndexedProperty(
    IDispatch& object,
    const wchar_t* property_name,
    const VARIANT& index) {
  base::win::ScopedVariant result;
  AutoWrap(DISPATCH_PROPERTYGET, result.Receive(), &object,
           const_cast<LPOLESTR>(property_name), {index});
  return Microsoft::WRL::ComPtr<IDispatch>(result.AsInput()->pdispVal);
}

}  // namespace

// ExcelSheetModel

void ExcelSheetModel::SetDataSize(int rows, int cols) {
  assert(rows >= 1);
  assert(cols >= 0);

  SAFEARRAYBOUND bounds[2];
  bounds[0].lLbound = 1;
  bounds[0].cElements = rows;
  bounds[1].lLbound = 1;
  bounds[1].cElements = cols;
  data.Set(SafeArrayCreate(VT_VARIANT, 2, bounds));

  this->cols = cols;
  this->rows = rows;
}

void ExcelSheetModel::SetData(int row, int col, const VARIANT& val) {
  LONG ixs[] = {row, col};
  SafeArrayPutElement(data.AsInput()->parray, ixs, const_cast<VARIANT*>(&val));
}

// Excel

Excel::Excel() {
  CLSID clsid;
  HRESULT hr = CLSIDFromProgID(L"Excel.Application", &clsid);
  if (FAILED(hr))
    throw hr;
  if (FAILED(hr = ::CoCreateInstance(clsid, nullptr, CLSCTX_LOCAL_SERVER,
                                     IID_PPV_ARGS(&excel))))
    throw hr;
}

void Excel::SetVisible(bool visible) {
  VARIANT x;
  x.vt = VT_I4;
  x.lVal = visible ? 1 : 0;
  AutoWrap(DISPATCH_PROPERTYPUT, NULL, excel.Get(), L"Visible", {x});
}

void Excel::NewWorkbook() {
  base::win::ScopedVariant result;
  AutoWrap(DISPATCH_PROPERTYGET, result.Receive(), excel.Get(), L"Workbooks");
  Microsoft::WRL::ComPtr<IDispatch> books(result.AsInput()->pdispVal);

  result.Reset();
  AutoWrap(DISPATCH_PROPERTYGET, result.Receive(), books.Get(), L"Add");
  workbook = result.AsInput()->pdispVal;
}

Microsoft::WRL::ComPtr<IDispatch> Excel::GetRange(const wchar_t* name) {
  base::win::ScopedVariant parm(name);
  return GetIndexedProperty(*sheet.Get(), L"Range", parm);
}

void Excel::NewSheet(const ExcelSheetModel& sheet) {
  // sheet = Application.ActiveSheet
  this->sheet = GetProperty(*excel.Get(), L"ActiveSheet");

  // range = sheet.Range[A1:XY]
  auto str = base::StringPrintf(L"A1:%lc%d", L'A' + sheet.cols - 1, sheet.rows);
  auto range = GetRange(str.c_str());

  // range.Value = data
  auto hr =
      AutoWrap(DISPATCH_PROPERTYPUT, NULL, range.Get(), L"Value", {sheet.data});
  if (FAILED(hr))
    throw hr;

  // auto validation = GetProperty(*GetProperty(*GetRange(L"D2"),
  // L"Validation"), L"Add");

  /*// Set .Saved property of workbook to TRUE so we aren't prompted
  // to save when we tell Excel to quit...
  {
    VARIANT x;
    x.vt = VT_I4;
    x.lVal = 1;
    AutoWrap(DISPATCH_PROPERTYPUT, NULL, pXlBook, L"Saved", x);
  }

  // Tell Excel to quit (i.e. App.Quit)
  AutoWrap(DISPATCH_METHOD, NULL, pXlApp, L"Quit");*/
}
