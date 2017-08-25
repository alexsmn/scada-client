// Created by Microsoft (R) C/C++ Compiler Version 10.00.40219.01 (b6b374a1).
//
// c:\work\workplace\trunk\build-vs10\debug\objs\client\displayviewerx.tlh
//
// C++ source equivalent of Win32 type library c:\Program Files\Telecontrol\Vidicon\Bin\\DisplayViewerX.ocx
// compiler-generated file created 03/02/13 at 21:16:03 - DO NOT EDIT!

#pragma once
#pragma pack(push, 8)

#include <comdef.h>

namespace ViewerX {

//
// Forward references and typedefs
//

struct __declspec(uuid("272eaacf-bbdd-47b3-8bea-928c47622398"))
/* LIBID */ __ViewerX;
struct __declspec(uuid("9dfb14a8-9b76-4c60-8d7c-e50b4796a64e"))
/* dual interface */ IViewerForm;
struct __declspec(uuid("19ceaad6-2670-4bea-92a3-947828494782"))
/* dispinterface */ IViewerFormEvents;
struct /* coclass */ ViewerForm;
enum TxActiveFormBorderStyle;
enum TxPrintScale;
enum TxMouseButton;
enum TxPopupMode;

//
// Smart pointer typedef declarations
//

_COM_SMARTPTR_TYPEDEF(IViewerFormEvents, __uuidof(IViewerFormEvents));
_COM_SMARTPTR_TYPEDEF(IViewerForm, __uuidof(IViewerForm));

//
// Type library items
//

struct __declspec(uuid("19ceaad6-2670-4bea-92a3-947828494782"))
IViewerFormEvents : IDispatch
{};

struct __declspec(uuid("80b66966-8aa1-4e09-adce-aaf48d6eedcc"))
ViewerForm;
    // [ default ] interface IViewerForm
    // [ default, source ] dispinterface IViewerFormEvents
    // interface ITelecontrolView

enum __declspec(uuid("30253bf4-82e8-459d-b080-623c56cd2f25"))
TxActiveFormBorderStyle
{
    afbNone = 0,
    afbSingle = 1,
    afbSunken = 2,
    afbRaised = 3
};

enum __declspec(uuid("58cf9465-dec8-4adc-97c8-687621043bb4"))
TxPrintScale
{
    poNone = 0,
    poProportional = 1,
    poPrintToFit = 2
};

enum __declspec(uuid("2ff7c904-a781-4682-b3fb-418d776e765c"))
TxMouseButton
{
    mbLeft = 0,
    mbRight = 1,
    mbMiddle = 2
};

enum __declspec(uuid("c30ad7b4-9cd5-470f-bcd2-06c92ab7ce84"))
TxPopupMode
{
    pmNone = 0,
    pmAuto = 1,
    pmExplicit = 2
};

struct __declspec(uuid("9dfb14a8-9b76-4c60-8d7c-e50b4796a64e"))
IViewerForm : IDispatch
{
    //
    // Raw methods provided by interface
    //

      virtual HRESULT __stdcall get_Visible (
        /*[out,retval]*/ VARIANT_BOOL * Value ) = 0;
      virtual HRESULT __stdcall put_Visible (
        /*[in]*/ VARIANT_BOOL Value ) = 0;
      virtual HRESULT __stdcall get_AutoScroll (
        /*[out,retval]*/ VARIANT_BOOL * Value ) = 0;
      virtual HRESULT __stdcall put_AutoScroll (
        /*[in]*/ VARIANT_BOOL Value ) = 0;
      virtual HRESULT __stdcall get_AutoSize (
        /*[out,retval]*/ VARIANT_BOOL * Value ) = 0;
      virtual HRESULT __stdcall put_AutoSize (
        /*[in]*/ VARIANT_BOOL Value ) = 0;
      virtual HRESULT __stdcall get_AxBorderStyle (
        /*[out,retval]*/ enum TxActiveFormBorderStyle * Value ) = 0;
      virtual HRESULT __stdcall put_AxBorderStyle (
        /*[in]*/ enum TxActiveFormBorderStyle Value ) = 0;
      virtual HRESULT __stdcall get_Caption (
        /*[out,retval]*/ BSTR * Value ) = 0;
      virtual HRESULT __stdcall put_Caption (
        /*[in]*/ BSTR Value ) = 0;
      virtual HRESULT __stdcall get_Color (
        /*[out,retval]*/ OLE_COLOR * Value ) = 0;
      virtual HRESULT __stdcall put_Color (
        /*[in]*/ OLE_COLOR Value ) = 0;
      virtual HRESULT __stdcall get_Font (
        /*[out,retval]*/ IFontDisp * * Value ) = 0;
      virtual HRESULT __stdcall put_Font (
        /*[in]*/ IFontDisp * Value ) = 0;
      virtual HRESULT __stdcall putref_Font (
        /*[in]*/ IFontDisp * * Value ) = 0;
      virtual HRESULT __stdcall get_KeyPreview (
        /*[out,retval]*/ VARIANT_BOOL * Value ) = 0;
      virtual HRESULT __stdcall put_KeyPreview (
        /*[in]*/ VARIANT_BOOL Value ) = 0;
      virtual HRESULT __stdcall get_PixelsPerInch (
        /*[out,retval]*/ long * Value ) = 0;
      virtual HRESULT __stdcall put_PixelsPerInch (
        /*[in]*/ long Value ) = 0;
      virtual HRESULT __stdcall get_PrintScale (
        /*[out,retval]*/ enum TxPrintScale * Value ) = 0;
      virtual HRESULT __stdcall put_PrintScale (
        /*[in]*/ enum TxPrintScale Value ) = 0;
      virtual HRESULT __stdcall get_Scaled (
        /*[out,retval]*/ VARIANT_BOOL * Value ) = 0;
      virtual HRESULT __stdcall put_Scaled (
        /*[in]*/ VARIANT_BOOL Value ) = 0;
      virtual HRESULT __stdcall get_Active (
        /*[out,retval]*/ VARIANT_BOOL * Value ) = 0;
      virtual HRESULT __stdcall get_DropTarget (
        /*[out,retval]*/ VARIANT_BOOL * Value ) = 0;
      virtual HRESULT __stdcall put_DropTarget (
        /*[in]*/ VARIANT_BOOL Value ) = 0;
      virtual HRESULT __stdcall get_HelpFile (
        /*[out,retval]*/ BSTR * Value ) = 0;
      virtual HRESULT __stdcall put_HelpFile (
        /*[in]*/ BSTR Value ) = 0;
      virtual HRESULT __stdcall get_PopupMode (
        /*[out,retval]*/ enum TxPopupMode * Value ) = 0;
      virtual HRESULT __stdcall put_PopupMode (
        /*[in]*/ enum TxPopupMode Value ) = 0;
      virtual HRESULT __stdcall get_ScreenSnap (
        /*[out,retval]*/ VARIANT_BOOL * Value ) = 0;
      virtual HRESULT __stdcall put_ScreenSnap (
        /*[in]*/ VARIANT_BOOL Value ) = 0;
      virtual HRESULT __stdcall get_SnapBuffer (
        /*[out,retval]*/ long * Value ) = 0;
      virtual HRESULT __stdcall put_SnapBuffer (
        /*[in]*/ long Value ) = 0;
      virtual HRESULT __stdcall get_DockSite (
        /*[out,retval]*/ VARIANT_BOOL * Value ) = 0;
      virtual HRESULT __stdcall put_DockSite (
        /*[in]*/ VARIANT_BOOL Value ) = 0;
      virtual HRESULT __stdcall get_DoubleBuffered (
        /*[out,retval]*/ VARIANT_BOOL * Value ) = 0;
      virtual HRESULT __stdcall put_DoubleBuffered (
        /*[in]*/ VARIANT_BOOL Value ) = 0;
      virtual HRESULT __stdcall get_AlignDisabled (
        /*[out,retval]*/ VARIANT_BOOL * Value ) = 0;
      virtual HRESULT __stdcall get_VisibleDockClientCount (
        /*[out,retval]*/ long * Value ) = 0;
      virtual HRESULT __stdcall get_UseDockManager (
        /*[out,retval]*/ VARIANT_BOOL * Value ) = 0;
      virtual HRESULT __stdcall put_UseDockManager (
        /*[in]*/ VARIANT_BOOL Value ) = 0;
      virtual HRESULT __stdcall get_Enabled (
        /*[out,retval]*/ VARIANT_BOOL * Value ) = 0;
      virtual HRESULT __stdcall put_Enabled (
        /*[in]*/ VARIANT_BOOL Value ) = 0;
      virtual HRESULT __stdcall get_AutoStartRuntime (
        /*[out,retval]*/ VARIANT_BOOL * Value ) = 0;
      virtual HRESULT __stdcall put_AutoStartRuntime (
        /*[in]*/ VARIANT_BOOL Value ) = 0;
      virtual HRESULT __stdcall get_FileName (
        /*[out,retval]*/ BSTR * Value ) = 0;
      virtual HRESULT __stdcall put_FileName (
        /*[in]*/ BSTR Value ) = 0;
      virtual HRESULT __stdcall get_FileProps (
        /*[out,retval]*/ BSTR * Value ) = 0;
      virtual HRESULT __stdcall put_FileProps (
        /*[in]*/ BSTR Value ) = 0;
      virtual HRESULT __stdcall get_ShowDockPanel (
        /*[out,retval]*/ VARIANT_BOOL * Value ) = 0;
      virtual HRESULT __stdcall put_ShowDockPanel (
        /*[in]*/ VARIANT_BOOL Value ) = 0;
      virtual HRESULT __stdcall get_ParentWin (
        /*[out,retval]*/ long * Value ) = 0;
      virtual HRESULT __stdcall put_ParentWin (
        /*[in]*/ long Value ) = 0;
};

} // namespace ViewerX

#pragma pack(pop)
