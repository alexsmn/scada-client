// Created by Microsoft (R) C/C++ Compiler Version 14.35.32217.1 (a54d6eaf).
//
// C:\tc\scada\build-windows\client\qt\client_qt_lib.dir\Debug\htsde2.tlh
//
// C++ source equivalent of Win32 type library d:\Program Files (x86)\Modus 6.30\bin\htsde2.ocx 
// compiler-generated file - DO NOT EDIT!

//
// Cross-referenced type libraries:
//
//

#pragma once
#pragma pack(push, 8)

#include <comdef.h>

namespace htsde2 {

//
// Forward references and typedefs
//

struct __declspec(uuid("c556e264-5efd-4f48-8848-018f760c45b4"))
/* LIBID */ __htsde2;
struct __declspec(uuid("3616106f-e967-463f-bc23-04f523a5bf87"))
/* dual interface */ IHTSDEForm2;
struct __declspec(uuid("4677cfa4-78eb-43c0-84ae-b5ce30bf0f0d"))
/* dispinterface */ IHTSDEForm2Events;
struct __declspec(uuid("cc0e2703-07e4-4ad8-b83c-b371a2131efb"))
/* dual interface */ IHTSDEForm3;
struct __declspec(uuid("0351fd0a-91b9-410f-b651-80bb66e36417"))
/* dual interface */ INavigator;
struct /* coclass */ Navigator;
struct __declspec(uuid("0f766e0a-548e-4a32-ab3d-856e8b1d255f"))
/* dual interface */ IAuxiliary;
struct __declspec(uuid("47f17b73-916d-4be2-a2e2-d1cff64ab9fa"))
/* dual interface */ IProtection;
struct /* coclass */ Protection;
struct /* coclass */ HTSDEForm2;
struct __declspec(uuid("0618f0ef-20f2-412e-bc3f-a9d7f426ce16"))
/* dual interface */ IStringVarParam;
struct __declspec(uuid("d828c596-7b2f-472f-9923-7758b3d5d771"))
/* dual interface */ IBoolVarParam;
struct /* coclass */ StringVarParam;
struct /* coclass */ BoolVarParam;
enum TxActiveFormBorderStyle;
enum TxPrintScale;
enum TxMouseButton;

//
// Smart pointer typedef declarations
//

_COM_SMARTPTR_TYPEDEF(IHTSDEForm2Events, __uuidof(IHTSDEForm2Events));
_COM_SMARTPTR_TYPEDEF(INavigator, __uuidof(INavigator));
_COM_SMARTPTR_TYPEDEF(IProtection, __uuidof(IProtection));
_COM_SMARTPTR_TYPEDEF(IAuxiliary, __uuidof(IAuxiliary));
_COM_SMARTPTR_TYPEDEF(IStringVarParam, __uuidof(IStringVarParam));
_COM_SMARTPTR_TYPEDEF(IBoolVarParam, __uuidof(IBoolVarParam));
_COM_SMARTPTR_TYPEDEF(IHTSDEForm2, __uuidof(IHTSDEForm2));
_COM_SMARTPTR_TYPEDEF(IHTSDEForm3, __uuidof(IHTSDEForm3));

//
// Type library items
//

struct __declspec(uuid("4677cfa4-78eb-43c0-84ae-b5ce30bf0f0d"))
IHTSDEForm2Events : IDispatch
{};

struct __declspec(uuid("0351fd0a-91b9-410f-b651-80bb66e36417"))
INavigator : IDispatch
{
    //
    // Raw methods provided by interface
    //

      virtual HRESULT __stdcall get_Handle (
        /*[out,retval]*/ OLE_HANDLE * Value ) = 0;
      virtual HRESULT __stdcall get_Visible (
        /*[out,retval]*/ VARIANT_BOOL * Value ) = 0;
      virtual HRESULT __stdcall put_Visible (
        /*[in]*/ VARIANT_BOOL Value ) = 0;
      virtual HRESULT __stdcall ActivateFocus ( ) = 0;
      virtual HRESULT __stdcall get_Stretch (
        /*[out,retval]*/ VARIANT_BOOL * Value ) = 0;
      virtual HRESULT __stdcall put_Stretch (
        /*[in]*/ VARIANT_BOOL Value ) = 0;
};

struct __declspec(uuid("001f373c-29d3-630e-a000-a1fc803d82ee"))
Navigator;
    // [ default ] interface INavigator

struct __declspec(uuid("47f17b73-916d-4be2-a2e2-d1cff64ab9fa"))
IProtection : IDispatch
{
    //
    // Raw methods provided by interface
    //

      virtual HRESULT __stdcall DropNetKey ( ) = 0;
};

struct __declspec(uuid("0f766e0a-548e-4a32-ab3d-856e8b1d255f"))
IAuxiliary : IDispatch
{
    //
    // Raw methods provided by interface
    //

      virtual HRESULT __stdcall get_Protection (
        /*[out,retval]*/ struct IProtection * * Value ) = 0;
};

struct __declspec(uuid("75b66b10-2cf8-5bde-a3fa-1d416bd1134c"))
Protection;
    // [ default ] interface IProtection

struct __declspec(uuid("001f373c-29d3-630e-a0a0-a0fc803d82ee"))
HTSDEForm2;
    // interface IHTSDEForm2
    // [ default, source ] dispinterface IHTSDEForm2Events
    // [ default ] interface IHTSDEForm3

struct __declspec(uuid("0618f0ef-20f2-412e-bc3f-a9d7f426ce16"))
IStringVarParam : IDispatch
{
    //
    // Raw methods provided by interface
    //

      virtual HRESULT __stdcall get_Value (
        /*[out,retval]*/ BSTR * Value ) = 0;
      virtual HRESULT __stdcall put_Value (
        /*[in]*/ BSTR Value ) = 0;
};

struct __declspec(uuid("d828c596-7b2f-472f-9923-7758b3d5d771"))
IBoolVarParam : IDispatch
{
    //
    // Raw methods provided by interface
    //

      virtual HRESULT __stdcall get_Value (
        /*[out,retval]*/ VARIANT_BOOL * Value ) = 0;
      virtual HRESULT __stdcall put_Value (
        /*[in]*/ VARIANT_BOOL Value ) = 0;
};

struct __declspec(uuid("89b62fcc-f31e-4f60-9af9-3dfde3da3ed6"))
StringVarParam;
    // [ default ] interface IStringVarParam

struct __declspec(uuid("eb5e50a2-6d1a-40f8-9e44-17ec80dafbf1"))
BoolVarParam;
    // [ default ] interface IBoolVarParam

enum __declspec(uuid("91021e35-6222-11d1-a8df-000001325083"))
TxActiveFormBorderStyle
{
    afbNone = 0,
    afbSingle = 1,
    afbSunken = 2,
    afbRaised = 3
};

enum __declspec(uuid("91021e36-6222-11d1-a8df-000001325083"))
TxPrintScale
{
    poNone = 0,
    poProportional = 1,
    poPrintToFit = 2
};

struct __declspec(uuid("3616106f-e967-463f-bc23-04f523a5bf87"))
IHTSDEForm2 : IDispatch
{
    //
    // Raw methods provided by interface
    //

      virtual HRESULT __stdcall get_AutoScroll (
        /*[out,retval]*/ VARIANT_BOOL * AutoScroll ) = 0;
      virtual HRESULT __stdcall put_AutoScroll (
        /*[in]*/ VARIANT_BOOL AutoScroll ) = 0;
      virtual HRESULT __stdcall get_AxBorderStyle (
        /*[out,retval]*/ enum TxActiveFormBorderStyle * AxBorderStyle ) = 0;
      virtual HRESULT __stdcall put_AxBorderStyle (
        /*[in]*/ enum TxActiveFormBorderStyle AxBorderStyle ) = 0;
      virtual HRESULT __stdcall get_Caption (
        /*[out,retval]*/ BSTR * Caption ) = 0;
      virtual HRESULT __stdcall put_Caption (
        /*[in]*/ BSTR Caption ) = 0;
      virtual HRESULT __stdcall get_Color (
        /*[out,retval]*/ OLE_COLOR * Color ) = 0;
      virtual HRESULT __stdcall put_Color (
        /*[in]*/ OLE_COLOR Color ) = 0;
      virtual HRESULT __stdcall get_Font (
        /*[out,retval]*/ struct Font * * Font ) = 0;
      virtual HRESULT __stdcall put_Font (
        /*[in]*/ struct Font * Font ) = 0;
      virtual HRESULT __stdcall get_KeyPreview (
        /*[out,retval]*/ VARIANT_BOOL * KeyPreview ) = 0;
      virtual HRESULT __stdcall put_KeyPreview (
        /*[in]*/ VARIANT_BOOL KeyPreview ) = 0;
      virtual HRESULT __stdcall get_PixelsPerInch (
        /*[out,retval]*/ long * PixelsPerInch ) = 0;
      virtual HRESULT __stdcall put_PixelsPerInch (
        /*[in]*/ long PixelsPerInch ) = 0;
      virtual HRESULT __stdcall get_PrintScale (
        /*[out,retval]*/ enum TxPrintScale * PrintScale ) = 0;
      virtual HRESULT __stdcall put_PrintScale (
        /*[in]*/ enum TxPrintScale PrintScale ) = 0;
      virtual HRESULT __stdcall get_Scaled (
        /*[out,retval]*/ VARIANT_BOOL * Scaled ) = 0;
      virtual HRESULT __stdcall put_Scaled (
        /*[in]*/ VARIANT_BOOL Scaled ) = 0;
      virtual HRESULT __stdcall get_Active (
        /*[out,retval]*/ VARIANT_BOOL * Active ) = 0;
      virtual HRESULT __stdcall get_DropTarget (
        /*[out,retval]*/ VARIANT_BOOL * DropTarget ) = 0;
      virtual HRESULT __stdcall put_DropTarget (
        /*[in]*/ VARIANT_BOOL DropTarget ) = 0;
      virtual HRESULT __stdcall get_HelpFile (
        /*[out,retval]*/ BSTR * HelpFile ) = 0;
      virtual HRESULT __stdcall put_HelpFile (
        /*[in]*/ BSTR HelpFile ) = 0;
      virtual HRESULT __stdcall get_WindowState (
        /*[out,retval]*/ enum SDECore::TxWindowState * WindowState ) = 0;
      virtual HRESULT __stdcall put_WindowState (
        /*[in]*/ enum SDECore::TxWindowState WindowState ) = 0;
      virtual HRESULT __stdcall get_Visible (
        /*[out,retval]*/ VARIANT_BOOL * Visible ) = 0;
      virtual HRESULT __stdcall put_Visible (
        /*[in]*/ VARIANT_BOOL Visible ) = 0;
      virtual HRESULT __stdcall get_Enabled (
        /*[out,retval]*/ VARIANT_BOOL * Enabled ) = 0;
      virtual HRESULT __stdcall put_Enabled (
        /*[in]*/ VARIANT_BOOL Enabled ) = 0;
      virtual HRESULT __stdcall get_Cursor (
        /*[out,retval]*/ short * Cursor ) = 0;
      virtual HRESULT __stdcall put_Cursor (
        /*[in]*/ short Cursor ) = 0;
      virtual HRESULT __stdcall AboutBox ( ) = 0;
      virtual HRESULT __stdcall get_FileName (
        /*[out,retval]*/ BSTR * FileName ) = 0;
      virtual HRESULT __stdcall put_FileName (
        /*[in]*/ BSTR FileName ) = 0;
      virtual HRESULT __stdcall get_ToolbarVisible (
        /*[out,retval]*/ VARIANT_BOOL * Value ) = 0;
      virtual HRESULT __stdcall put_ToolbarVisible (
        /*[in]*/ VARIANT_BOOL Value ) = 0;
      virtual HRESULT __stdcall get_StatusVisible (
        /*[out,retval]*/ VARIANT_BOOL * Value ) = 0;
      virtual HRESULT __stdcall put_StatusVisible (
        /*[in]*/ VARIANT_BOOL Value ) = 0;
      virtual HRESULT __stdcall get_Scale (
        /*[out,retval]*/ long * Value ) = 0;
      virtual HRESULT __stdcall put_Scale (
        /*[in]*/ long Value ) = 0;
      virtual HRESULT __stdcall FitToWindow ( ) = 0;
      virtual HRESULT __stdcall FindView (
        /*[in]*/ BSTR ViewIdent,
        /*[out,retval]*/ long * Value ) = 0;
      virtual HRESULT __stdcall GotoView (
        /*[in]*/ BSTR ViewIdent,
        /*[out,retval]*/ long * Value ) = 0;
      virtual HRESULT __stdcall Print ( ) = 0;
      virtual HRESULT __stdcall get_NavigatorVisible (
        /*[out,retval]*/ VARIANT_BOOL * Value ) = 0;
      virtual HRESULT __stdcall put_NavigatorVisible (
        /*[in]*/ VARIANT_BOOL Value ) = 0;
      virtual HRESULT __stdcall get_PagesVisible (
        /*[out,retval]*/ enum SDECore::TxPagesMode * Value ) = 0;
      virtual HRESULT __stdcall put_PagesVisible (
        /*[in]*/ enum SDECore::TxPagesMode Value ) = 0;
      virtual HRESULT __stdcall ShowOptions ( ) = 0;
      virtual HRESULT __stdcall get_ScrollVisible (
        /*[out,retval]*/ VARIANT_BOOL * Value ) = 0;
      virtual HRESULT __stdcall put_ScrollVisible (
        /*[in]*/ VARIANT_BOOL Value ) = 0;
      virtual HRESULT __stdcall get_Document (
        /*[out,retval]*/ struct SDECore::ISDEDocument50 * * Value ) = 0;
      virtual HRESULT __stdcall Open (
        /*[in]*/ BSTR FileName ) = 0;
      virtual HRESULT __stdcall get_ElectricModel (
        /*[out,retval]*/ IDispatch * * Value ) = 0;
      virtual HRESULT __stdcall get_Loader (
        /*[out,retval]*/ struct SDECore::ISDELoader * * Value ) = 0;
      virtual HRESULT __stdcall get_AppHandle (
        /*[out,retval]*/ OLE_HANDLE * Value ) = 0;
      virtual HRESULT __stdcall put_AppHandle (
        /*[in]*/ OLE_HANDLE Value ) = 0;
      virtual HRESULT __stdcall get_Navigator (
        /*[out,retval]*/ struct INavigator * * Value ) = 0;
};

struct __declspec(uuid("cc0e2703-07e4-4ad8-b83c-b371a2131efb"))
IHTSDEForm3 : IHTSDEForm2
{
    //
    // Raw methods provided by interface
    //

      virtual HRESULT __stdcall get_DefaultPopups (
        /*[out,retval]*/ VARIANT_BOOL * Value ) = 0;
      virtual HRESULT __stdcall put_DefaultPopups (
        /*[in]*/ VARIANT_BOOL Value ) = 0;
      virtual HRESULT __stdcall get_ContextToolbar (
        /*[out,retval]*/ VARIANT_BOOL * Value ) = 0;
      virtual HRESULT __stdcall put_ContextToolbar (
        /*[in]*/ VARIANT_BOOL Value ) = 0;
      virtual HRESULT __stdcall get_GuardantID (
        /*[out,retval]*/ long * Value ) = 0;
};

enum __declspec(uuid("91021e37-6222-11d1-a8df-000001325083"))
TxMouseButton
{
    mbLeft = 0,
    mbRight = 1,
    mbMiddle = 2
};

} // namespace htsde2

#pragma pack(pop)
