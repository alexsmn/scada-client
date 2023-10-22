// Created by Microsoft (R) C/C++ Compiler Version 14.35.32217.1 (5ac98c27).
//
// C:\tc\scada\build-windows\client\qt\client_qt_lib.dir\Debug\sdecore.tlh
//
// C++ source equivalent of Win32 type library d:\Program Files (x86)\Modus 6.30\bin\sdecore.tlb 
// compiler-generated file - DO NOT EDIT!

#pragma once
#pragma pack(push, 8)

#include <comdef.h>

namespace SDECore {

//
// Forward references and typedefs
//

struct __declspec(uuid("30de86af-6933-4e53-954a-d866c97c1ae8"))
/* LIBID */ __SDECore;
struct __declspec(uuid("eef40d50-119e-4aa6-a1f3-60bf70da2186"))
/* interface */ IPersistScheme;
struct __declspec(uuid("973944ce-f917-4eeb-892e-7bf47047c502"))
/* dispinterface */ IDocumentEvents;
struct __declspec(uuid("2d0dc1b6-3f2b-4e0d-8a7b-5d1f9865aa9b"))
/* dual interface */ ISDEPage;
struct __declspec(uuid("d246e295-4c30-4bc5-8c31-01f0995eee2b"))
/* dual interface */ ISDEPage42;
struct __declspec(uuid("bb7bc19b-973b-4f70-a05f-a5e5326bd566"))
/* dual interface */ ISDEPages;
struct /* coclass */ SDEPages;
struct __declspec(uuid("2255f30d-2be8-4bfc-b16f-55ec4c5304ce"))
/* dual interface */ IParamInfo;
struct __declspec(uuid("f39f4490-37d1-4efc-a023-53b91ef20246"))
/* dual interface */ IRules;
struct /* coclass */ Rules;
struct __declspec(uuid("d4aa8b3c-5f8a-43f5-a8a4-38cd0d296547"))
/* dual interface */ IRule;
struct /* coclass */ Rule;
struct __declspec(uuid("aa30f080-ed90-4b41-9828-fa554623f2c0"))
/* dual interface */ ISDEDocument;
struct __declspec(uuid("f6485fde-55dc-47d8-8627-c597379cf402"))
/* dual interface */ ISDEDocument42;
struct __declspec(uuid("a01c704c-30f3-49e1-8fa5-38d319ef7718"))
/* dual interface */ INodes2;
struct __declspec(uuid("4d94cb1b-1d42-4ede-a822-6a1f567ff1ec"))
/* dual interface */ INodes42;
struct __declspec(uuid("5daf1789-f246-4885-a473-ac69705acefc"))
/* dual interface */ INode2;
struct __declspec(uuid("95a0cfb3-1e7d-488e-b53d-9ac3415aaa73"))
/* dual interface */ INode42;
struct __declspec(uuid("de4c1798-6123-47bf-b958-6fef306326db"))
/* dual interface */ ISDEObjects2;
struct __declspec(uuid("701b1666-26e2-4bc6-aaba-cd7caf577f15"))
/* dual interface */ ISDEObjects42;
struct __declspec(uuid("f6b1bd00-e87f-425a-ab9f-c13bffd996cd"))
/* dual interface */ ISDEObject2;
struct __declspec(uuid("7ffc4552-ff27-404d-bc71-d08b567fc4fa"))
/* dual interface */ ISDEObject42;
struct __declspec(uuid("834e0aba-ab7c-40a2-83ce-89a64a79f35a"))
/* dual interface */ IParams;
struct __declspec(uuid("95b4d131-88e6-4231-b9b7-69e72f33114c"))
/* dual interface */ IParams50;
struct __declspec(uuid("df135ed9-33ab-4e8e-8303-4309265c265c"))
/* dual interface */ IParam;
struct __declspec(uuid("b7416000-cfc1-404a-a692-91b145a49bf8"))
/* dual interface */ IParam42;
struct /* coclass */ ParamInfo;
struct __declspec(uuid("7c1d4b07-965c-4687-aa84-03388bcb64a0"))
/* dual interface */ IUserProperties;
struct /* coclass */ UserProperties;
struct __declspec(uuid("ceff8052-5065-48fa-9a04-7b2319784722"))
/* dual interface */ IDetailsLevel;
struct /* coclass */ DetailsLevel;
struct __declspec(uuid("1be6f60d-a363-4b63-b348-21b0e1187cd9"))
/* dual interface */ IDetailsLevels;
struct /* coclass */ DetailsLevels;
struct __declspec(uuid("25715b0a-c72f-4ff6-8660-61e4b8aed217"))
/* dual interface */ ITRect;
struct __declspec(uuid("2d4c6da5-9e80-4979-b47c-7e92bf692350"))
/* dual interface */ ITPoint;
struct /* coclass */ TMRect;
struct /* coclass */ TMPoint;
struct __declspec(uuid("6fc71e72-96aa-43ff-a9e4-c0c89b6c289f"))
/* dual interface */ IUIEventInfo;
struct /* coclass */ UIEventInfo;
struct __declspec(uuid("a8014b75-1133-46bd-b6ba-821e264e799b"))
/* dual interface */ IHighlights;
struct /* coclass */ HighLights;
struct __declspec(uuid("8101f8a7-ca7c-4146-9c6e-12079cf74092"))
/* dual interface */ ICPNotifyCustom;
struct /* coclass */ CPNotifyFilter;
struct __declspec(uuid("8092dc52-c505-40bd-9f8f-13885b819af2"))
/* dual interface */ INamedPB;
struct __declspec(uuid("c0db07c5-d6c2-45a3-8380-e05a219b3e98"))
/* dual interface */ INamedPB42;
struct __declspec(uuid("ac064aaf-2f87-4559-bb53-36e202a9a112"))
/* dual interface */ INamedPBs;
struct __declspec(uuid("b468e35c-947c-46dd-9730-69b57f3689fc"))
/* dual interface */ INamedPBs42;
struct __declspec(uuid("fdc2c328-378f-4160-8fb1-d3c5ed472210"))
/* dual interface */ ITechObject;
struct /* coclass */ TechObject;
struct __declspec(uuid("163b85fc-de71-43fe-88bb-4e40f11e6614"))
/* dual interface */ IParamsEx;
struct /* coclass */ ParamsEx;
struct __declspec(uuid("d575f411-f9a7-4e8a-b60a-59f258d4419f"))
/* dual interface */ ISDELoader;
struct /* coclass */ SDELoader;
struct __declspec(uuid("0c966278-8a18-47da-8a1e-dfd4e2286cc3"))
/* dual interface */ IParam50;
struct __declspec(uuid("e4d7fbd1-d54c-4419-bb1d-4be0f3657910"))
/* dual interface */ ISDEObject50;
struct __declspec(uuid("d1b1d336-e8a4-462e-a96a-f7766e250287"))
/* dual interface */ ISDEObjectInfo;
struct __declspec(uuid("d16e32bd-d1af-46df-b9d7-df3f53f8bd5e"))
/* dual interface */ ISDEElectricElement;
struct /* coclass */ SDEObjects2;
struct /* coclass */ Node2;
struct /* coclass */ Nodes2;
struct __declspec(uuid("974dd2c0-bb34-409a-abe0-df40bcc9a717"))
/* dual interface */ ISDEDocument50;
struct __declspec(uuid("997cae1b-b4cf-4192-94f6-e1c6afddded1"))
/* dual interface */ ISDEPage50;
struct /* coclass */ NamedPB;
struct __declspec(uuid("9d2bee17-4d3f-465c-ae1a-a5382eb49856"))
/* dual interface */ INamedPBs50;
struct /* coclass */ SDEPage;
struct /* coclass */ Param;
struct __declspec(uuid("4db6ca48-c505-4a1d-81ce-8d9545b2547d"))
/* dual interface */ IParams52;
struct /* coclass */ SDEObject2;
struct __declspec(uuid("abbc76cf-f1be-4be9-b02b-fca4d792a548"))
/* dual interface */ ISDEDocument51;
struct __declspec(uuid("3b99406f-56f7-488a-ab07-70ed8455e560"))
/* dual interface */ INamedPBs63;
struct /* coclass */ ObjParam;
struct __declspec(uuid("4b578465-7922-4711-9b37-649ae874556f"))
/* dual interface */ ISDEPage51;
struct /* coclass */ SDEDocument;
struct __declspec(uuid("7558e134-2968-4a92-a978-4b9678ef180d"))
/* dual interface */ IActionRec;
struct /* coclass */ ActionRec;
struct __declspec(uuid("a493e2a4-2946-4098-9c65-48e791e9102c"))
/* dual interface */ IHyperLinkInfo;
struct /* coclass */ HyperLinkInfo;
struct __declspec(uuid("f5adefc9-1a01-4d70-8649-9eeebd1b7067"))
/* dual interface */ IHyperLinkInfos;
struct /* coclass */ HyperLinkInfos;
struct __declspec(uuid("42b8c33f-75db-4dc0-a9de-c8d0ead1beb6"))
/* dual interface */ IMethodInfo;
struct /* coclass */ MethodInfo;
struct /* coclass */ Params;
struct __declspec(uuid("71d437b1-e1c2-4b1b-ac38-5e88d50e9b92"))
/* dual interface */ INamedPB50;
struct __declspec(uuid("e62af4d8-37ff-4a79-8e1d-ac629ae860e8"))
/* dual interface */ IRParams;
struct /* coclass */ RParams;
struct /* coclass */ NamedPBs;
enum TxActiveFormBorderStyle;
enum TxPrintScale;
enum TxMouseButton;
enum TxWindowState;
enum TxParamMode;
enum TxPagesMode;
enum TxShowMode;
enum TxRuleType;
enum TxStatQuery;
enum TxVisibilityStatus;
enum TxParamCategory;
enum TxAttributeType;
enum TxPropertySource;
enum TxCopyFlags;
enum TxPersister;
enum TAllowAction;
enum ParamSourceType;
enum MetaSourceType;
enum TxListType;
enum TxMaskChoice;
enum TxPChangeSource;
enum TxPChangeMode;
enum TxNagrState;
enum TxNodeState;
enum TxDamagedSet;
enum TxFindMask;
enum TxTypeFormat;
enum TxParamFlags;
enum TxExecType;
enum TxMethodParamOpt;
struct SDERect;
struct SDEPoint;
struct RParam;
union UKeyState;
typedef IUnknown * PiUnknown1;

//
// Smart pointer typedef declarations
//

_COM_SMARTPTR_TYPEDEF(IPersistScheme, __uuidof(IPersistScheme));
_COM_SMARTPTR_TYPEDEF(IDocumentEvents, __uuidof(IDocumentEvents));
_COM_SMARTPTR_TYPEDEF(IRule, __uuidof(IRule));
_COM_SMARTPTR_TYPEDEF(IRules, __uuidof(IRules));
_COM_SMARTPTR_TYPEDEF(IUserProperties, __uuidof(IUserProperties));
_COM_SMARTPTR_TYPEDEF(IDetailsLevel, __uuidof(IDetailsLevel));
_COM_SMARTPTR_TYPEDEF(IDetailsLevels, __uuidof(IDetailsLevels));
_COM_SMARTPTR_TYPEDEF(ITRect, __uuidof(ITRect));
_COM_SMARTPTR_TYPEDEF(ITPoint, __uuidof(ITPoint));
_COM_SMARTPTR_TYPEDEF(ICPNotifyCustom, __uuidof(ICPNotifyCustom));
_COM_SMARTPTR_TYPEDEF(IHighlights, __uuidof(IHighlights));
_COM_SMARTPTR_TYPEDEF(INodes2, __uuidof(INodes2));
_COM_SMARTPTR_TYPEDEF(INodes42, __uuidof(INodes42));
_COM_SMARTPTR_TYPEDEF(INamedPBs, __uuidof(INamedPBs));
_COM_SMARTPTR_TYPEDEF(INamedPBs42, __uuidof(INamedPBs42));
_COM_SMARTPTR_TYPEDEF(ISDEPages, __uuidof(ISDEPages));
_COM_SMARTPTR_TYPEDEF(IParams, __uuidof(IParams));
_COM_SMARTPTR_TYPEDEF(ISDEDocument, __uuidof(ISDEDocument));
_COM_SMARTPTR_TYPEDEF(INode2, __uuidof(INode2));
_COM_SMARTPTR_TYPEDEF(IUIEventInfo, __uuidof(IUIEventInfo));
_COM_SMARTPTR_TYPEDEF(ISDELoader, __uuidof(ISDELoader));
_COM_SMARTPTR_TYPEDEF(IHyperLinkInfo, __uuidof(IHyperLinkInfo));
_COM_SMARTPTR_TYPEDEF(IHyperLinkInfos, __uuidof(IHyperLinkInfos));
_COM_SMARTPTR_TYPEDEF(ISDEPage, __uuidof(ISDEPage));
_COM_SMARTPTR_TYPEDEF(INamedPB, __uuidof(INamedPB));
_COM_SMARTPTR_TYPEDEF(INamedPB42, __uuidof(INamedPB42));
_COM_SMARTPTR_TYPEDEF(INamedPB50, __uuidof(INamedPB50));
_COM_SMARTPTR_TYPEDEF(ITechObject, __uuidof(ITechObject));
_COM_SMARTPTR_TYPEDEF(INamedPBs50, __uuidof(INamedPBs50));
_COM_SMARTPTR_TYPEDEF(INamedPBs63, __uuidof(INamedPBs63));
_COM_SMARTPTR_TYPEDEF(ISDEObjects2, __uuidof(ISDEObjects2));
_COM_SMARTPTR_TYPEDEF(ISDEPage42, __uuidof(ISDEPage42));
_COM_SMARTPTR_TYPEDEF(ISDEPage50, __uuidof(ISDEPage50));
_COM_SMARTPTR_TYPEDEF(ISDEPage51, __uuidof(ISDEPage51));
_COM_SMARTPTR_TYPEDEF(IParamInfo, __uuidof(IParamInfo));
_COM_SMARTPTR_TYPEDEF(IParam, __uuidof(IParam));
_COM_SMARTPTR_TYPEDEF(IParamsEx, __uuidof(IParamsEx));
_COM_SMARTPTR_TYPEDEF(ISDEObject2, __uuidof(ISDEObject2));
_COM_SMARTPTR_TYPEDEF(ISDEObjects42, __uuidof(ISDEObjects42));
_COM_SMARTPTR_TYPEDEF(IParam42, __uuidof(IParam42));
_COM_SMARTPTR_TYPEDEF(ISDEElectricElement, __uuidof(ISDEElectricElement));
_COM_SMARTPTR_TYPEDEF(ISDEDocument42, __uuidof(ISDEDocument42));
_COM_SMARTPTR_TYPEDEF(ISDEDocument50, __uuidof(ISDEDocument50));
_COM_SMARTPTR_TYPEDEF(ISDEDocument51, __uuidof(ISDEDocument51));
_COM_SMARTPTR_TYPEDEF(IActionRec, __uuidof(IActionRec));
_COM_SMARTPTR_TYPEDEF(IMethodInfo, __uuidof(IMethodInfo));
_COM_SMARTPTR_TYPEDEF(INode42, __uuidof(INode42));
_COM_SMARTPTR_TYPEDEF(ISDEObject42, __uuidof(ISDEObject42));
_COM_SMARTPTR_TYPEDEF(ISDEObject50, __uuidof(ISDEObject50));
_COM_SMARTPTR_TYPEDEF(IParams50, __uuidof(IParams50));
_COM_SMARTPTR_TYPEDEF(IParam50, __uuidof(IParam50));
_COM_SMARTPTR_TYPEDEF(IParams52, __uuidof(IParams52));
_COM_SMARTPTR_TYPEDEF(ISDEObjectInfo, __uuidof(ISDEObjectInfo));
_COM_SMARTPTR_TYPEDEF(IRParams, __uuidof(IRParams));

//
// Type library items
//

struct __declspec(uuid("eef40d50-119e-4aa6-a1f3-60bf70da2186"))
IPersistScheme : IUnknown
{
    //
    // Raw methods provided by interface
    //

      virtual HRESULT __stdcall GetClassID (
        /*[out]*/ GUID * pClassID ) = 0;
      virtual HRESULT __stdcall IsDirty ( ) = 0;
      virtual HRESULT __stdcall InitNew (
        /*[in]*/ IUnknown * pStg ) = 0;
      virtual HRESULT __stdcall Load (
        /*[in]*/ IUnknown * pStg ) = 0;
      virtual HRESULT __stdcall Save (
        /*[in]*/ IUnknown * pStgSave,
        /*[in]*/ VARIANT_BOOL fSameAsLoad ) = 0;
      virtual HRESULT __stdcall SaveCompleted (
        /*[in]*/ IUnknown * pStgNew ) = 0;
      virtual HRESULT __stdcall HandsOffStorage ( ) = 0;
};

struct __declspec(uuid("973944ce-f917-4eeb-892e-7bf47047c502"))
IDocumentEvents : IDispatch
{};

struct __declspec(uuid("df1f9150-bd04-4177-b4f5-cb3ab1cffd67"))
SDEPages;
    // [ default ] interface ISDEPages

struct __declspec(uuid("53599508-50bc-4714-b04c-21306018c55b"))
Rules;
    // [ default ] interface IRules

struct __declspec(uuid("d4aa8b3c-5f8a-43f5-a8a4-38cd0d296547"))
IRule : IDispatch
{
    //
    // Raw methods provided by interface
    //

      virtual HRESULT __stdcall get_Name (
        /*[out,retval]*/ BSTR * Value ) = 0;
};

struct __declspec(uuid("f39f4490-37d1-4efc-a023-53b91ef20246"))
IRules : IDispatch
{
    //
    // Raw methods provided by interface
    //

      virtual HRESULT __stdcall get_Count (
        /*[out,retval]*/ long * Value ) = 0;
      virtual HRESULT __stdcall get_Item (
        /*[in]*/ VARIANT Index,
        /*[out,retval]*/ struct IRule * * Value ) = 0;
      virtual HRESULT __stdcall _NewEnum ( ) = 0;
      virtual HRESULT __stdcall LoadFromFile (
        /*[in]*/ BSTR FileName ) = 0;
      virtual HRESULT __stdcall Clear ( ) = 0;
};

struct __declspec(uuid("a5652ad6-f095-442a-98ba-d49d873782a8"))
Rule;
    // [ default ] interface IRule

struct __declspec(uuid("398c1a6f-733a-455a-a001-8aba9530845a"))
ParamInfo;
    // [ default ] interface IParamInfo

struct __declspec(uuid("7c1d4b07-965c-4687-aa84-03388bcb64a0"))
IUserProperties : IDispatch
{
    //
    // Raw methods provided by interface
    //

      virtual HRESULT __stdcall SaveToFile (
        /*[in]*/ BSTR FileName ) = 0;
      virtual HRESULT __stdcall get_Count (
        /*[out,retval]*/ long * Value ) = 0;
      virtual HRESULT __stdcall Add (
        /*[in]*/ BSTR Name,
        /*[out,retval]*/ struct IParamInfo * * Parametr ) = 0;
      virtual HRESULT __stdcall Delete (
        /*[in]*/ VARIANT Param ) = 0;
      virtual HRESULT __stdcall get_Item (
        /*[in]*/ VARIANT Index,
        /*[out,retval]*/ struct IParamInfo * * Value ) = 0;
      virtual HRESULT __stdcall get__NewEnum (
        /*[out,retval]*/ IUnknown * * Value ) = 0;
};

struct __declspec(uuid("e903f99c-7335-4abf-86fa-7d893bdaf466"))
UserProperties;
    // [ default ] interface IUserProperties

struct __declspec(uuid("ceff8052-5065-48fa-9a04-7b2319784722"))
IDetailsLevel : IDispatch
{
    //
    // Raw methods provided by interface
    //

      virtual HRESULT __stdcall get_Name (
        /*[out,retval]*/ BSTR * Value ) = 0;
      virtual HRESULT __stdcall get_PasswordShow (
        /*[out,retval]*/ BSTR * Value ) = 0;
      virtual HRESULT __stdcall put_PasswordShow (
        /*[in]*/ BSTR Value ) = 0;
      virtual HRESULT __stdcall get_PasswordEdit (
        /*[out,retval]*/ BSTR * Value ) = 0;
      virtual HRESULT __stdcall put_PasswordEdit (
        /*[in]*/ BSTR Value ) = 0;
      virtual HRESULT __stdcall get_Comment (
        /*[out,retval]*/ BSTR * Value ) = 0;
      virtual HRESULT __stdcall put_Comment (
        /*[in]*/ BSTR Value ) = 0;
      virtual HRESULT __stdcall get_Value (
        /*[out,retval]*/ long * Value ) = 0;
      virtual HRESULT __stdcall put_Value (
        /*[in]*/ long Value ) = 0;
      virtual HRESULT __stdcall get_Visible (
        /*[out,retval]*/ VARIANT_BOOL * Value ) = 0;
      virtual HRESULT __stdcall put_Visible (
        /*[in]*/ VARIANT_BOOL Value ) = 0;
      virtual HRESULT __stdcall get_Docked (
        /*[out,retval]*/ VARIANT_BOOL * Value ) = 0;
      virtual HRESULT __stdcall put_Docked (
        /*[in]*/ VARIANT_BOOL Value ) = 0;
      virtual HRESULT __stdcall get_PassOpened (
        /*[out,retval]*/ VARIANT_BOOL * Value ) = 0;
      virtual HRESULT __stdcall put_PassOpened (
        /*[in]*/ VARIANT_BOOL Value ) = 0;
};

struct __declspec(uuid("fcab445e-b77c-4385-9c03-e56fc1505f80"))
DetailsLevel;
    // [ default ] interface IDetailsLevel

struct __declspec(uuid("1be6f60d-a363-4b63-b348-21b0e1187cd9"))
IDetailsLevels : IDispatch
{
    //
    // Raw methods provided by interface
    //

      virtual HRESULT __stdcall get_Count (
        /*[out,retval]*/ long * Value ) = 0;
      virtual HRESULT __stdcall Add (
        /*[in]*/ BSTR Name,
        /*[out,retval]*/ struct IDetailsLevel * * Level ) = 0;
      virtual HRESULT __stdcall Delete (
        /*[in]*/ VARIANT Level ) = 0;
      virtual HRESULT __stdcall SaveToFile (
        /*[in]*/ BSTR FileName ) = 0;
      virtual HRESULT __stdcall get_Item (
        /*[in]*/ VARIANT Index,
        /*[out,retval]*/ struct IDetailsLevel * * Value ) = 0;
      virtual HRESULT __stdcall get__NewEnum (
        /*[out,retval]*/ IUnknown * * Value ) = 0;
};

struct __declspec(uuid("4e70fda4-8cda-4e1e-bddf-de857c542153"))
DetailsLevels;
    // [ default ] interface IDetailsLevels

struct __declspec(uuid("25715b0a-c72f-4ff6-8660-61e4b8aed217"))
ITRect : IDispatch
{
    //
    // Raw methods provided by interface
    //

      virtual HRESULT __stdcall get_Left (
        /*[out,retval]*/ double * pLeft ) = 0;
      virtual HRESULT __stdcall put_Left (
        /*[in]*/ double pLeft ) = 0;
      virtual HRESULT __stdcall get_Top (
        /*[out,retval]*/ double * pTop ) = 0;
      virtual HRESULT __stdcall put_Top (
        /*[in]*/ double pTop ) = 0;
      virtual HRESULT __stdcall get_Right (
        /*[out,retval]*/ double * pRight ) = 0;
      virtual HRESULT __stdcall put_Right (
        /*[in]*/ double pRight ) = 0;
      virtual HRESULT __stdcall get_Bottom (
        /*[out,retval]*/ double * pBottom ) = 0;
      virtual HRESULT __stdcall put_Bottom (
        /*[in]*/ double pBottom ) = 0;
      virtual HRESULT __stdcall Clone (
        /*[out,retval]*/ struct ITRect * * ppiRect ) = 0;
};

struct __declspec(uuid("2d4c6da5-9e80-4979-b47c-7e92bf692350"))
ITPoint : IDispatch
{
    //
    // Raw methods provided by interface
    //

      virtual HRESULT __stdcall get_X (
        /*[out,retval]*/ double * pX ) = 0;
      virtual HRESULT __stdcall put_X (
        /*[in]*/ double pX ) = 0;
      virtual HRESULT __stdcall get_Y (
        /*[out,retval]*/ double * pY ) = 0;
      virtual HRESULT __stdcall put_Y (
        /*[in]*/ double pY ) = 0;
      virtual HRESULT __stdcall Clone (
        /*[out,retval]*/ struct ITPoint * * ppiPoint ) = 0;
};

struct __declspec(uuid("721592ee-0ce2-423e-96be-486a20553de7"))
TMRect;
    // [ default ] interface ITRect

struct __declspec(uuid("90ddaeb8-8ea6-4571-8e3e-e0d0fb666277"))
TMPoint;
    // [ default ] interface ITPoint

struct __declspec(uuid("72f292a1-6293-41a1-8d66-cc8750ea0cf3"))
UIEventInfo;
    // [ default ] interface IUIEventInfo

struct __declspec(uuid("1a4e44eb-8ff4-409c-8b38-811cbc3949a0"))
HighLights;
    // [ default ] interface IHighlights

struct __declspec(uuid("99a4a327-af55-4953-a546-db86dc0134e7"))
CPNotifyFilter;
    // [ default ] interface ICPNotifyCustom

struct __declspec(uuid("8101f8a7-ca7c-4146-9c6e-12079cf74092"))
ICPNotifyCustom : IDispatch
{
    //
    // Raw methods provided by interface
    //

      virtual HRESULT __stdcall get_SDEObjTypeName (
        /*[out,retval]*/ BSTR * Value ) = 0;
      virtual HRESULT __stdcall put_SDEObjTypeName (
        /*[in]*/ BSTR Value ) = 0;
      virtual HRESULT __stdcall get_SDEObjParam (
        /*[out,retval]*/ BSTR * Value ) = 0;
      virtual HRESULT __stdcall put_SDEObjParam (
        /*[in]*/ BSTR Value ) = 0;
      virtual HRESULT __stdcall get_NotifyTime (
        /*[out,retval]*/ long * Value ) = 0;
      virtual HRESULT __stdcall put_NotifyTime (
        /*[in]*/ long Value ) = 0;
      virtual HRESULT __stdcall get_ParamByTypeName (
        /*[in]*/ BSTR Index,
        /*[out,retval]*/ BSTR * Value ) = 0;
      virtual HRESULT __stdcall put_ParamByTypeName (
        /*[in]*/ BSTR Index,
        /*[in]*/ BSTR Value ) = 0;
      virtual HRESULT __stdcall Clone (
        /*[out,retval]*/ struct ICPNotifyCustom * * Value ) = 0;
};

struct __declspec(uuid("7d7248a4-527a-473c-95a2-b0801f025cdc"))
TechObject;
    // [ default ] interface ITechObject

struct __declspec(uuid("c9229f0e-6ec0-4788-83b3-5c32ed2e8b4f"))
ParamsEx;
    // [ default ] interface IParamsEx

struct __declspec(uuid("781e89be-e203-4bc3-8b68-f40bc340c978"))
SDELoader;
    // [ default ] interface ISDELoader

struct __declspec(uuid("6be50079-f574-481b-aebe-05a53e086ff6"))
SDEObjects2;
    // [ default ] interface ISDEObjects2
    // interface ISDEObjects42

struct __declspec(uuid("a8014b75-1133-46bd-b6ba-821e264e799b"))
IHighlights : IDispatch
{
    //
    // Raw methods provided by interface
    //

      virtual HRESULT __stdcall get_Count (
        /*[out,retval]*/ long * Value ) = 0;
      virtual HRESULT __stdcall get_Item (
        /*[in]*/ VARIANT Index,
        /*[out,retval]*/ struct ISDEObjects2 * * Value ) = 0;
      virtual HRESULT __stdcall get__NewEnum (
        /*[out,retval]*/ IUnknown * * Value ) = 0;
      virtual HRESULT __stdcall Add (
        /*[in]*/ BSTR Name,
        /*[in]*/ BSTR StyleName,
        /*[out,retval]*/ struct ISDEObjects2 * * Value ) = 0;
      virtual HRESULT __stdcall Delete (
        /*[in]*/ VARIANT Index ) = 0;
      virtual HRESULT __stdcall get_Visible (
        /*[in]*/ VARIANT Index,
        /*[out,retval]*/ VARIANT_BOOL * Value ) = 0;
      virtual HRESULT __stdcall put_Visible (
        /*[in]*/ VARIANT Index,
        /*[in]*/ VARIANT_BOOL Value ) = 0;
      virtual HRESULT __stdcall Clear ( ) = 0;
      virtual HRESULT __stdcall get_StyleName (
        /*[in]*/ VARIANT Index,
        /*[out,retval]*/ BSTR * Value ) = 0;
      virtual HRESULT __stdcall get_Styles (
        /*[out,retval]*/ BSTR * Value ) = 0;
};

struct __declspec(uuid("fad0e168-1eba-4030-9adc-01d5cf89011e"))
Node2;
    // interface INode2
    // [ default ] interface INode42

struct __declspec(uuid("a01c704c-30f3-49e1-8fa5-38d319ef7718"))
INodes2 : IDispatch
{
    //
    // Raw methods provided by interface
    //

      virtual HRESULT __stdcall get_Count (
        /*[out,retval]*/ long * Value ) = 0;
      virtual HRESULT __stdcall get_Item (
        /*[in]*/ VARIANT Index,
        /*[out,retval]*/ struct INode42 * * Value ) = 0;
      virtual HRESULT __stdcall get__NewEnum (
        /*[out,retval]*/ IUnknown * * Value ) = 0;
};

struct __declspec(uuid("4d94cb1b-1d42-4ede-a822-6a1f567ff1ec"))
INodes42 : INodes2
{
    //
    // Raw methods provided by interface
    //

      virtual HRESULT __stdcall Connect ( ) = 0;
      virtual HRESULT __stdcall Disconnect ( ) = 0;
};

struct __declspec(uuid("894c3d25-76ce-485a-ac2e-846c835fffd8"))
Nodes2;
    // interface INodes2
    // [ default ] interface INodes42

struct __declspec(uuid("af046181-e383-42a6-b375-938809a0bbba"))
NamedPB;
    // [ default ] interface INamedPB
    // interface INamedPB42

struct __declspec(uuid("ac064aaf-2f87-4559-bb53-36e202a9a112"))
INamedPBs : IDispatch
{
    //
    // Raw methods provided by interface
    //

      virtual HRESULT __stdcall get_Count (
        /*[out,retval]*/ long * Value ) = 0;
      virtual HRESULT __stdcall get_Item (
        /*[in]*/ VARIANT Index,
        /*[out,retval]*/ struct INamedPB * * Value ) = 0;
      virtual HRESULT __stdcall get__NewEnum (
        /*[out,retval]*/ IUnknown * * Value ) = 0;
      virtual HRESULT __stdcall get_Name (
        /*[out,retval]*/ BSTR * Value ) = 0;
      virtual HRESULT __stdcall put_Name (
        /*[in]*/ BSTR Value ) = 0;
      virtual HRESULT __stdcall AddPB (
        /*[in]*/ struct INamedPB * Value ) = 0;
      virtual HRESULT __stdcall CreatePB (
        /*[in]*/ BSTR TypeName,
        /*[in]*/ BSTR KeyLink,
        /*[out,retval]*/ struct INamedPB * * PB ) = 0;
      virtual HRESULT __stdcall get_UserProp (
        /*[out,retval]*/ struct IUserProperties * * Value ) = 0;
      virtual HRESULT __stdcall RemovePB (
        /*[in]*/ VARIANT Index ) = 0;
};

struct __declspec(uuid("b468e35c-947c-46dd-9730-69b57f3689fc"))
INamedPBs42 : INamedPBs
{
    //
    // Raw methods provided by interface
    //

      virtual HRESULT __stdcall get_PBValue (
        /*[in]*/ BSTR PBID,
        /*[in]*/ BSTR ParamName,
        /*[out,retval]*/ BSTR * Value ) = 0;
      virtual HRESULT __stdcall put_PBValue (
        /*[in]*/ BSTR PBID,
        /*[in]*/ BSTR ParamName,
        /*[in]*/ BSTR Value ) = 0;
      virtual HRESULT __stdcall get_PBValueInd (
        /*[in]*/ BSTR PBID,
        /*[in]*/ BSTR ParamName,
        /*[in]*/ long Index,
        /*[out,retval]*/ BSTR * Value ) = 0;
      virtual HRESULT __stdcall put_PBValueInd (
        /*[in]*/ BSTR PBID,
        /*[in]*/ BSTR ParamName,
        /*[in]*/ long Index,
        /*[in]*/ BSTR Value ) = 0;
};

struct __declspec(uuid("6625f06f-94b9-4128-b23a-1493a5f89323"))
SDEPage;
    // interface ISDEPage
    // interface ISDEPage42
    // [ default ] interface ISDEPage50

struct __declspec(uuid("bb7bc19b-973b-4f70-a05f-a5e5326bd566"))
ISDEPages : IDispatch
{
    //
    // Raw methods provided by interface
    //

      virtual HRESULT __stdcall get_Count (
        /*[out,retval]*/ long * Value ) = 0;
      virtual HRESULT __stdcall get_Item (
        /*[in]*/ VARIANT Index,
        /*[out,retval]*/ struct ISDEPage50 * * Value ) = 0;
      virtual HRESULT __stdcall get__NewEnum (
        /*[out,retval]*/ IUnknown * * Value ) = 0;
};

struct __declspec(uuid("b34b54d9-5501-4968-a7bd-8875f4636122"))
Param;
    // [ default ] interface IParam
    // interface IParam42
    // interface IParam50

struct __declspec(uuid("834e0aba-ab7c-40a2-83ce-89a64a79f35a"))
IParams : IDispatch
{
    //
    // Raw methods provided by interface
    //

      virtual HRESULT __stdcall get_Count (
        /*[out,retval]*/ long * Value ) = 0;
      virtual HRESULT __stdcall get_Item (
        /*[in]*/ VARIANT Index,
        /*[out,retval]*/ struct IParam * * Value ) = 0;
      virtual HRESULT __stdcall get__NewEnum (
        /*[out,retval]*/ IUnknown * * Value ) = 0;
      virtual HRESULT __stdcall get_AsText (
        /*[out,retval]*/ BSTR * Value ) = 0;
};

struct __declspec(uuid("5b0222ab-3033-456b-adfd-1ac4dbc649d2"))
SDEObject2;
    // interface ISDEObject2
    // interface ISDEObject42
    // [ default ] interface ISDEObject50

struct __declspec(uuid("aa30f080-ed90-4b41-9828-fa554623f2c0"))
ISDEDocument : IDispatch
{
    //
    // Raw methods provided by interface
    //

      virtual HRESULT __stdcall get_Flat (
        /*[out,retval]*/ struct ISDEObjects2 * * Value ) = 0;
      virtual HRESULT __stdcall get_FlatIndex (
        /*[out,retval]*/ BSTR * Value ) = 0;
      virtual HRESULT __stdcall put_FlatIndex (
        /*[in]*/ BSTR Value ) = 0;
      virtual HRESULT __stdcall get_Application (
        /*[out,retval]*/ IDispatch * * Value ) = 0;
      virtual HRESULT __stdcall get_Author (
        /*[out,retval]*/ BSTR * Value ) = 0;
      virtual HRESULT __stdcall put_Author (
        /*[in]*/ BSTR Value ) = 0;
      virtual HRESULT __stdcall get_Comments (
        /*[out,retval]*/ BSTR * Value ) = 0;
      virtual HRESULT __stdcall put_Comments (
        /*[in]*/ BSTR Value ) = 0;
      virtual HRESULT __stdcall get_FullName (
        /*[out,retval]*/ BSTR * Value ) = 0;
      virtual HRESULT __stdcall get_KeyWords (
        /*[out,retval]*/ BSTR * Value ) = 0;
      virtual HRESULT __stdcall put_KeyWords (
        /*[in]*/ BSTR Value ) = 0;
      virtual HRESULT __stdcall get_Name (
        /*[out,retval]*/ BSTR * Value ) = 0;
      virtual HRESULT __stdcall get_Parent (
        /*[out,retval]*/ IDispatch * * Value ) = 0;
      virtual HRESULT __stdcall get_Path (
        /*[out,retval]*/ BSTR * Value ) = 0;
      virtual HRESULT __stdcall get_ReadOnly (
        /*[out,retval]*/ VARIANT_BOOL * Value ) = 0;
      virtual HRESULT __stdcall get_Saved (
        /*[out,retval]*/ VARIANT_BOOL * Value ) = 0;
      virtual HRESULT __stdcall get_Subject (
        /*[out,retval]*/ BSTR * Value ) = 0;
      virtual HRESULT __stdcall put_Subject (
        /*[in]*/ BSTR Value ) = 0;
      virtual HRESULT __stdcall get_Title (
        /*[out,retval]*/ BSTR * Value ) = 0;
      virtual HRESULT __stdcall put_Title (
        /*[in]*/ BSTR Value ) = 0;
      virtual HRESULT __stdcall Activate ( ) = 0;
      virtual HRESULT __stdcall Close ( ) = 0;
      virtual HRESULT __stdcall Print ( ) = 0;
      virtual HRESULT __stdcall PrintOut ( ) = 0;
      virtual HRESULT __stdcall Save (
        /*[in]*/ VARIANT FileName,
        /*[in]*/ VARIANT Version ) = 0;
      virtual HRESULT __stdcall get_Pages (
        /*[out,retval]*/ struct ISDEPages * * Value ) = 0;
      virtual HRESULT __stdcall DocGoToView (
        /*[in]*/ struct ISDEObject50 * Element ) = 0;
      virtual HRESULT __stdcall get_SourceFileName (
        /*[out,retval]*/ BSTR * Value ) = 0;
      virtual HRESULT __stdcall put_SourceFileName (
        /*[in]*/ BSTR Value ) = 0;
      virtual HRESULT __stdcall get_CurrentPage (
        /*[out,retval]*/ struct ISDEPage50 * * Value ) = 0;
      virtual HRESULT __stdcall get_CurrentPageIndex (
        /*[out,retval]*/ VARIANT * Value ) = 0;
      virtual HRESULT __stdcall put_CurrentPageIndex (
        /*[in]*/ VARIANT Value ) = 0;
      virtual HRESULT __stdcall get_SelectedList (
        /*[out,retval]*/ struct ISDEObjects2 * * Value ) = 0;
      virtual HRESULT __stdcall get_SelectedView (
        /*[out,retval]*/ struct ISDEObject50 * * Value ) = 0;
      virtual HRESULT __stdcall put_SelectedView (
        /*[in]*/ struct ISDEObject50 * Value ) = 0;
      virtual HRESULT __stdcall FindViewIndex (
        /*[in]*/ BSTR Index,
        /*[in]*/ BSTR Value,
        /*[out,retval]*/ struct ISDEObject50 * * Element ) = 0;
      virtual HRESULT __stdcall get_RTID (
        /*[out,retval]*/ long * Value ) = 0;
      virtual HRESULT __stdcall get_UserProperties (
        /*[out,retval]*/ struct IUserProperties * * Value ) = 0;
      virtual HRESULT __stdcall get_DetailLevels (
        /*[out,retval]*/ struct IDetailsLevels * * Value ) = 0;
      virtual HRESULT __stdcall Open (
        /*[in]*/ BSTR FileName,
        /*[out,retval]*/ VARIANT_BOOL * Result ) = 0;
      virtual HRESULT __stdcall get_Int (
        /*[out,retval]*/ long * Value ) = 0;
      virtual HRESULT __stdcall put_Int (
        /*[in]*/ long Value ) = 0;
      virtual HRESULT __stdcall get_Selections (
        /*[out,retval]*/ struct IHighlights * * Value ) = 0;
      virtual HRESULT __stdcall FindViewsIndex (
        /*[in]*/ BSTR Index,
        /*[in]*/ BSTR Value,
        /*[out,retval]*/ struct ISDEObjects2 * * Elements ) = 0;
      virtual HRESULT __stdcall get_MultiSelect (
        /*[out,retval]*/ VARIANT_BOOL * Value ) = 0;
      virtual HRESULT __stdcall put_MultiSelect (
        /*[in]*/ VARIANT_BOOL Value ) = 0;
      virtual HRESULT __stdcall get_ShowRules (
        /*[out,retval]*/ struct IRules * * Value ) = 0;
      virtual HRESULT __stdcall get_CurrentEntity (
        /*[out,retval]*/ IDispatch * * Value ) = 0;
      virtual HRESULT __stdcall put_CurrentEntity (
        /*[in]*/ IDispatch * Value ) = 0;
      virtual HRESULT __stdcall get_ParamBlock (
        /*[out,retval]*/ IDispatch * * Value ) = 0;
      virtual HRESULT __stdcall put_ParamBlock (
        /*[in]*/ IDispatch * Value ) = 0;
      virtual HRESULT __stdcall BeginUpdate ( ) = 0;
      virtual HRESULT __stdcall EndUpdate ( ) = 0;
      virtual HRESULT __stdcall DocHighLight (
        /*[in]*/ struct ISDEObject50 * Element,
        /*[in]*/ long Color,
        /*[in]*/ VARIANT_BOOL Restrict ) = 0;
};

struct __declspec(uuid("5daf1789-f246-4885-a473-ac69705acefc"))
INode2 : IDispatch
{
    //
    // Raw methods provided by interface
    //

      virtual HRESULT __stdcall get_Count (
        /*[out,retval]*/ long * Value ) = 0;
      virtual HRESULT __stdcall get__NewEnum (
        /*[out,retval]*/ IUnknown * * Value ) = 0;
      virtual HRESULT __stdcall get_NodeNum (
        /*[out,retval]*/ long * Value ) = 0;
      virtual HRESULT __stdcall get_Item (
        /*[in]*/ VARIANT Index,
        /*[out,retval]*/ struct ISDEObject50 * * Value ) = 0;
};

struct __declspec(uuid("6fc71e72-96aa-43ff-a9e4-c0c89b6c289f"))
IUIEventInfo : IDispatch
{
    //
    // Raw methods provided by interface
    //

      virtual HRESULT __stdcall get_X (
        /*[out,retval]*/ long * pX ) = 0;
      virtual HRESULT __stdcall get_Y (
        /*[out,retval]*/ long * pY ) = 0;
      virtual HRESULT __stdcall get_KeyCode (
        /*[out,retval]*/ long * pvk ) = 0;
      virtual HRESULT __stdcall get_KeyChar (
        /*[out,retval]*/ BSTR * Char ) = 0;
      virtual HRESULT __stdcall get_Button (
        /*[out,retval]*/ long * pbutton ) = 0;
      virtual HRESULT __stdcall get_AltKey (
        /*[out,retval]*/ VARIANT_BOOL * pvb ) = 0;
      virtual HRESULT __stdcall get_CtrlKey (
        /*[out,retval]*/ VARIANT_BOOL * pvb ) = 0;
      virtual HRESULT __stdcall get_ShiftKey (
        /*[out,retval]*/ VARIANT_BOOL * pvb ) = 0;
      virtual HRESULT __stdcall get_KeyState (
        /*[out,retval]*/ long * Value ) = 0;
      virtual HRESULT __stdcall get_Touched (
        /*[out,retval]*/ struct ISDEObject50 * * Value ) = 0;
};

struct __declspec(uuid("b34b54d9-5501-4968-a7bd-8875f4636123"))
ObjParam;
    // interface IParam
    // interface IParam42
    // [ default ] interface IParam50

struct __declspec(uuid("72c9ac6d-2f00-4db5-a63b-8c00bf969b6e"))
SDEDocument;
    // interface ISDEDocument
    // [ default, source ] dispinterface IDocumentEvents
    // interface ISDEDocument42
    // [ default ] interface ISDEDocument50
    // interface ISDEDocument51

struct __declspec(uuid("d575f411-f9a7-4e8a-b60a-59f258d4419f"))
ISDELoader : IDispatch
{
    //
    // Raw methods provided by interface
    //

      virtual HRESULT __stdcall get_PagesFilter (
        /*[out,retval]*/ BSTR * Value ) = 0;
      virtual HRESULT __stdcall put_PagesFilter (
        /*[in]*/ BSTR Value ) = 0;
      virtual HRESULT __stdcall OpenFile (
        /*[in]*/ BSTR FileName,
        /*[out,retval]*/ struct ISDEDocument50 * * Value ) = 0;
      virtual HRESULT __stdcall OpenStorage (
        /*[in]*/ IUnknown * Storage,
        /*[out,retval]*/ struct ISDEDocument50 * * Value ) = 0;
      virtual HRESULT __stdcall OpenStream (
        /*[in]*/ IUnknown * Stream,
        /*[out,retval]*/ struct ISDEDocument50 * * Value ) = 0;
};

struct __declspec(uuid("ca7fdc1c-9932-4312-b49a-36d7e77996bc"))
ActionRec;
    // [ default ] interface IActionRec

struct __declspec(uuid("a493e2a4-2946-4098-9c65-48e791e9102c"))
IHyperLinkInfo : IDispatch
{
    //
    // Raw methods provided by interface
    //

      virtual HRESULT __stdcall get_Caption (
        /*[out,retval]*/ struct IActionRec * * Value ) = 0;
      virtual HRESULT __stdcall put_Caption (
        /*[in]*/ struct IActionRec * Value ) = 0;
      virtual HRESULT __stdcall get_SDEStorageAlias (
        /*[out,retval]*/ struct IActionRec * * Value ) = 0;
      virtual HRESULT __stdcall put_SDEStorageAlias (
        /*[in]*/ struct IActionRec * Value ) = 0;
      virtual HRESULT __stdcall get_FileName (
        /*[out,retval]*/ struct IActionRec * * Value ) = 0;
      virtual HRESULT __stdcall put_FileName (
        /*[in]*/ struct IActionRec * Value ) = 0;
      virtual HRESULT __stdcall get_Page (
        /*[out,retval]*/ struct IActionRec * * Value ) = 0;
      virtual HRESULT __stdcall put_Page (
        /*[in]*/ struct IActionRec * Value ) = 0;
      virtual HRESULT __stdcall get_Element (
        /*[out,retval]*/ struct IActionRec * * Value ) = 0;
      virtual HRESULT __stdcall put_Element (
        /*[in]*/ struct IActionRec * Value ) = 0;
      virtual HRESULT __stdcall get_Hint (
        /*[out,retval]*/ BSTR * Value ) = 0;
      virtual HRESULT __stdcall get_Icon (
        /*[out,retval]*/ struct IPicture * * Value ) = 0;
      virtual HRESULT __stdcall get_AsString (
        /*[out,retval]*/ BSTR * Value ) = 0;
      virtual HRESULT __stdcall put_AsString (
        /*[in]*/ BSTR Value ) = 0;
};

struct __declspec(uuid("c42e090f-495c-4db1-ad74-6e5ba04e086d"))
HyperLinkInfo;
    // [ default ] interface IHyperLinkInfo

struct __declspec(uuid("f5adefc9-1a01-4d70-8649-9eeebd1b7067"))
IHyperLinkInfos : IDispatch
{
    //
    // Raw methods provided by interface
    //

      virtual HRESULT __stdcall get_Count (
        /*[out,retval]*/ long * Value ) = 0;
      virtual HRESULT __stdcall get_Item (
        /*[in]*/ VARIANT Index,
        /*[out,retval]*/ struct IHyperLinkInfo * * Value ) = 0;
      virtual HRESULT __stdcall get__NewEnum (
        /*[out,retval]*/ IUnknown * * Value ) = 0;
};

struct __declspec(uuid("15eb40b8-588c-41ed-af83-cb77952be901"))
HyperLinkInfos;
    // [ default ] interface IHyperLinkInfos

struct __declspec(uuid("47d7abd3-33d1-4118-80ab-28720408a4f6"))
MethodInfo;
    // [ default ] interface IMethodInfo

struct __declspec(uuid("ad7a32ad-91c6-43c9-946a-b098bf435370"))
Params;
    // interface IParams
    // interface IParams50
    // [ default ] interface IParams52

struct __declspec(uuid("2d0dc1b6-3f2b-4e0d-8a7b-5d1f9865aa9b"))
ISDEPage : IDispatch
{
    //
    // Raw methods provided by interface
    //

      virtual HRESULT __stdcall get_Name (
        /*[out,retval]*/ BSTR * Value ) = 0;
      virtual HRESULT __stdcall put_Name (
        /*[in]*/ BSTR Value ) = 0;
      virtual HRESULT __stdcall get_SDEObjects (
        /*[out,retval]*/ struct ISDEObjects2 * * Value ) = 0;
      virtual HRESULT __stdcall get_Params (
        /*[out,retval]*/ struct IParams52 * * Value ) = 0;
      virtual HRESULT __stdcall get_UseTopology (
        /*[out,retval]*/ VARIANT_BOOL * Value ) = 0;
      virtual HRESULT __stdcall put_UseTopology (
        /*[in]*/ VARIANT_BOOL Value ) = 0;
      virtual HRESULT __stdcall get_SizeX (
        /*[out,retval]*/ long * Value ) = 0;
      virtual HRESULT __stdcall put_SizeX (
        /*[in]*/ long Value ) = 0;
      virtual HRESULT __stdcall get_SizeY (
        /*[out,retval]*/ long * Value ) = 0;
      virtual HRESULT __stdcall put_SizeY (
        /*[in]*/ long Value ) = 0;
      virtual HRESULT __stdcall get_PosX (
        /*[out,retval]*/ long * Value ) = 0;
      virtual HRESULT __stdcall put_PosX (
        /*[in]*/ long Value ) = 0;
      virtual HRESULT __stdcall get_PosY (
        /*[out,retval]*/ long * Value ) = 0;
      virtual HRESULT __stdcall put_PosY (
        /*[in]*/ long Value ) = 0;
      virtual HRESULT __stdcall get_Scale (
        /*[out,retval]*/ double * Value ) = 0;
      virtual HRESULT __stdcall put_Scale (
        /*[in]*/ double Value ) = 0;
      virtual HRESULT __stdcall get_Color (
        /*[out,retval]*/ OLE_COLOR * Value ) = 0;
      virtual HRESULT __stdcall put_Color (
        /*[in]*/ OLE_COLOR Value ) = 0;
      virtual HRESULT __stdcall get_WinLeft (
        /*[out,retval]*/ long * Value ) = 0;
      virtual HRESULT __stdcall put_WinLeft (
        /*[in]*/ long Value ) = 0;
      virtual HRESULT __stdcall get_WinTop (
        /*[out,retval]*/ long * Value ) = 0;
      virtual HRESULT __stdcall put_WinTop (
        /*[in]*/ long Value ) = 0;
      virtual HRESULT __stdcall get_WinWidth (
        /*[out,retval]*/ long * Value ) = 0;
      virtual HRESULT __stdcall put_WinWidth (
        /*[in]*/ long Value ) = 0;
      virtual HRESULT __stdcall get_WinHeight (
        /*[out,retval]*/ long * Value ) = 0;
      virtual HRESULT __stdcall put_WinHeight (
        /*[in]*/ long Value ) = 0;
      virtual HRESULT __stdcall get_RTID (
        /*[out,retval]*/ long * Value ) = 0;
      virtual HRESULT __stdcall get_NavPicture (
        /*[out,retval]*/ VARIANT * Value ) = 0;
      virtual HRESULT __stdcall put_NavPicture (
        /*[in]*/ VARIANT Value ) = 0;
      virtual HRESULT __stdcall get_PosEndX (
        /*[out,retval]*/ long * Value ) = 0;
      virtual HRESULT __stdcall get_PosEndY (
        /*[out,retval]*/ long * Value ) = 0;
};

struct __declspec(uuid("8092dc52-c505-40bd-9f8f-13885b819af2"))
INamedPB : IDispatch
{
    //
    // Raw methods provided by interface
    //

      virtual HRESULT __stdcall get_TypeName (
        /*[out,retval]*/ BSTR * Value ) = 0;
      virtual HRESULT __stdcall put_TypeName (
        /*[in]*/ BSTR Value ) = 0;
      virtual HRESULT __stdcall get_KeyLink (
        /*[out,retval]*/ BSTR * Value ) = 0;
      virtual HRESULT __stdcall put_KeyLink (
        /*[in]*/ BSTR Value ) = 0;
      virtual HRESULT __stdcall get_Params (
        /*[out,retval]*/ struct IParams52 * * Value ) = 0;
      virtual HRESULT __stdcall Clone (
        /*[in]*/ BSTR NamedPBs,
        /*[out,retval]*/ struct INamedPB * * Value ) = 0;
};

struct __declspec(uuid("c0db07c5-d6c2-45a3-8380-e05a219b3e98"))
INamedPB42 : INamedPB
{
    //
    // Raw methods provided by interface
    //

      virtual HRESULT __stdcall get_Value (
        /*[in]*/ BSTR ParamName,
        /*[out,retval]*/ BSTR * Value ) = 0;
      virtual HRESULT __stdcall put_Value (
        /*[in]*/ BSTR ParamName,
        /*[in]*/ BSTR Value ) = 0;
      virtual HRESULT __stdcall get_ValueInd (
        /*[in]*/ BSTR ParamName,
        /*[in]*/ long Index,
        /*[out,retval]*/ BSTR * Value ) = 0;
      virtual HRESULT __stdcall put_ValueInd (
        /*[in]*/ BSTR ParamName,
        /*[in]*/ long Index,
        /*[in]*/ BSTR Value ) = 0;
};

struct __declspec(uuid("71d437b1-e1c2-4b1b-ac38-5e88d50e9b92"))
INamedPB50 : INamedPB42
{
    //
    // Raw methods provided by interface
    //

      virtual HRESULT __stdcall GetValues (
        /*[in]*/ BSTR Params,
        /*[out,retval]*/ VARIANT * Values ) = 0;
};

struct __declspec(uuid("3775ed24-5c3b-4eb1-81d5-148c67f1b204"))
RParams;
    // [ default ] interface IRParams

struct __declspec(uuid("c7fc2412-1c9e-45d7-9766-41deee114942"))
NamedPBs;
    // [ default ] interface INamedPBs
    // interface INamedPBs42
    // interface INamedPBs50
    // interface INamedPBs63

struct __declspec(uuid("fdc2c328-378f-4160-8fb1-d3c5ed472210"))
ITechObject : INamedPB
{
    //
    // Raw methods provided by interface
    //

      virtual HRESULT __stdcall get_Nodes (
        /*[out,retval]*/ struct INodes42 * * Value ) = 0;
      virtual HRESULT __stdcall get_Elements (
        /*[out,retval]*/ struct INamedPBs * * Value ) = 0;
      virtual HRESULT __stdcall get_RTID (
        /*[out,retval]*/ long * Value ) = 0;
      virtual HRESULT __stdcall put_RTID (
        /*[in]*/ long Value ) = 0;
      virtual HRESULT __stdcall get_CIMTypeName (
        /*[out,retval]*/ BSTR * Value ) = 0;
      virtual HRESULT __stdcall put_CIMTypeName (
        /*[in]*/ BSTR Value ) = 0;
      virtual HRESULT __stdcall get_AliasName (
        /*[out,retval]*/ BSTR * Value ) = 0;
      virtual HRESULT __stdcall put_AliasName (
        /*[in]*/ BSTR Value ) = 0;
      virtual HRESULT __stdcall get_Name (
        /*[out,retval]*/ BSTR * Value ) = 0;
      virtual HRESULT __stdcall put_Name (
        /*[in]*/ BSTR Value ) = 0;
      virtual HRESULT __stdcall get_PathName (
        /*[out,retval]*/ BSTR * Value ) = 0;
      virtual HRESULT __stdcall put_PathName (
        /*[in]*/ BSTR Value ) = 0;
      virtual HRESULT __stdcall get_Description (
        /*[out,retval]*/ BSTR * Value ) = 0;
      virtual HRESULT __stdcall put_Description (
        /*[in]*/ BSTR Value ) = 0;
      virtual HRESULT __stdcall get_Role (
        /*[out,retval]*/ BSTR * Value ) = 0;
      virtual HRESULT __stdcall put_Role (
        /*[in]*/ BSTR Value ) = 0;
};

struct __declspec(uuid("9d2bee17-4d3f-465c-ae1a-a5382eb49856"))
INamedPBs50 : INamedPBs42
{
    //
    // Raw methods provided by interface
    //

      virtual HRESULT __stdcall FindPBIndex (
        /*[in]*/ BSTR Index,
        /*[in]*/ BSTR Value,
        /*[out,retval]*/ struct INamedPB * * PB ) = 0;
      virtual HRESULT __stdcall FindPBsIndex (
        /*[in]*/ BSTR Index,
        /*[in]*/ BSTR Value,
        /*[out,retval]*/ struct INamedPBs * * PBs ) = 0;
};

struct __declspec(uuid("3b99406f-56f7-488a-ab07-70ed8455e560"))
INamedPBs63 : INamedPBs50
{
    //
    // Raw methods provided by interface
    //

      virtual HRESULT __stdcall GetParamsValues (
        /*[in]*/ __int64 CtxId,
        /*[in]*/ __int64 State,
        /*[in]*/ BSTR ObjsKey,
        /*[in]*/ BSTR Params,
        /*[out,retval]*/ BSTR * Result ) = 0;
      virtual HRESULT __stdcall GetCurrentState (
        /*[in]*/ __int64 CtxId,
        /*[out,retval]*/ BSTR * Result ) = 0;
};

enum __declspec(uuid("e98daf84-908e-4b76-8ce6-bd3c3807b07a"))
TxActiveFormBorderStyle
{
    afbNone = 0,
    afbSingle = 1,
    afbSunken = 2,
    afbRaised = 3
};

enum __declspec(uuid("e78f0e9a-87e8-478b-9ffb-b986186179ae"))
TxPrintScale
{
    poNone = 0,
    poProportional = 1,
    poPrintToFit = 2
};

enum __declspec(uuid("60edb8d4-915f-42a4-8629-f0c340476941"))
TxMouseButton
{
    mbLeft = 0,
    mbRight = 1,
    mbMiddle = 2
};

enum __declspec(uuid("fe4791c9-b8e3-423c-9069-9942ae9c5539"))
TxWindowState
{
    wsNormal = 0,
    wsMinimized = 1,
    wsMaximized = 2
};

enum __declspec(uuid("3a712f7e-bb75-4069-9b8c-01d8163e20eb"))
TxParamMode
{
    TxCanRead = 1,
    TxCanWrite = 2,
    TxIndexed = 4,
    TxSet = 8,
    TxObsolete = 16,
    TxTech = 32,
    TxCollection = 64,
    TxCanDisUse = 128,
    TxRunTime = 256
};

enum __declspec(uuid("0f339b8b-1329-4751-8f0a-e9fa16a42c1e"))
TxPagesMode
{
    txPagesAuto = 0,
    TxPagesVisible = 1,
    txPagesHidden = 2
};

enum __declspec(uuid("8ddd84f5-cd93-410e-bb74-1a2b1f56442c"))
TxShowMode
{
    txShowSxeme = 0,
    txShowTable = 2
};

enum __declspec(uuid("68157dd4-3721-46a1-828a-cf9f08d75f63"))
TxRuleType
{
    rtShow = 1,
    rtHyperLink = 2,
    rtUnknown = 0
};

enum __declspec(uuid("d6a2706e-1a40-4a41-8918-30c6a251b65c"))
TxStatQuery
{
    sqViewTypes = 0,
    sqViewParams = 1,
    sqObjTypes = 2,
    sqParamValues = 3
};

struct __declspec(uuid("de4c1798-6123-47bf-b958-6fef306326db"))
ISDEObjects2 : IDispatch
{
    //
    // Raw methods provided by interface
    //

      virtual HRESULT __stdcall get_Count (
        /*[out,retval]*/ long * Value ) = 0;
      virtual HRESULT __stdcall get__NewEnum (
        /*[out,retval]*/ IUnknown * * Value ) = 0;
      virtual HRESULT __stdcall get_Name (
        /*[out,retval]*/ BSTR * Value ) = 0;
      virtual HRESULT __stdcall get_Item (
        /*[in]*/ VARIANT Index,
        /*[out,retval]*/ struct ISDEObject50 * * Value ) = 0;
      virtual HRESULT __stdcall GetList (
        /*[in]*/ enum TxStatQuery QueryType,
        /*[in]*/ BSTR ElemType,
        /*[in]*/ BSTR Param,
        /*[out,retval]*/ BSTR * Value ) = 0;
      virtual HRESULT __stdcall CreateView (
        /*[in]*/ BSTR TypeName,
        /*[in]*/ BSTR Params,
        /*[out,retval]*/ struct ISDEObject50 * * Value ) = 0;
      virtual HRESULT __stdcall RemoveView (
        /*[in]*/ struct ISDEObject50 * Value ) = 0;
      virtual HRESULT __stdcall Clear ( ) = 0;
      virtual HRESULT __stdcall AddView (
        /*[in]*/ struct ISDEObject50 * Value ) = 0;
      virtual HRESULT __stdcall IsPresent (
        /*[in]*/ struct ISDEObject50 * Value,
        /*[out,retval]*/ VARIANT_BOOL * Result ) = 0;
};

struct __declspec(uuid("d246e295-4c30-4bc5-8c31-01f0995eee2b"))
ISDEPage42 : ISDEPage
{
    //
    // Raw methods provided by interface
    //

      virtual HRESULT __stdcall IntersectElements (
        /*[in]*/ double X1,
        /*[in]*/ double Y1,
        /*[in]*/ double X2,
        /*[in]*/ double X3,
        /*[out,retval]*/ struct ISDEObjects2 * * Result ) = 0;
      virtual HRESULT __stdcall ContainsElements (
        /*[in]*/ double X1,
        /*[in]*/ double Y1,
        /*[in]*/ double X2,
        /*[in]*/ double X3,
        /*[out,retval]*/ struct ISDEObjects2 * * Result ) = 0;
      virtual HRESULT __stdcall Reconnect ( ) = 0;
      virtual HRESULT __stdcall get_SizeXBase (
        /*[out,retval]*/ long * Value ) = 0;
      virtual HRESULT __stdcall put_SizeXBase (
        /*[in]*/ long Value ) = 0;
      virtual HRESULT __stdcall get_SizeYBase (
        /*[out,retval]*/ long * Value ) = 0;
      virtual HRESULT __stdcall put_SizeYBase (
        /*[in]*/ long Value ) = 0;
};

struct __declspec(uuid("997cae1b-b4cf-4192-94f6-e1c6afddded1"))
ISDEPage50 : ISDEPage42
{
    //
    // Raw methods provided by interface
    //

      virtual HRESULT __stdcall GetPictureByCoordinates (
        /*[in]*/ int X,
        /*[in]*/ int Y,
        /*[in]*/ int RigthX,
        /*[in]*/ int BottomY,
        /*[in]*/ BSTR FormatName,
        /*[in]*/ BSTR FileName,
        /*[out]*/ VARIANT * DataPicture ) = 0;
      virtual HRESULT __stdcall GetObjectByCoordinates (
        /*[in]*/ int X,
        /*[in]*/ int Y,
        /*[out,retval]*/ struct ISDEObject50 * * __Object ) = 0;
};

struct __declspec(uuid("4b578465-7922-4711-9b37-649ae874556f"))
ISDEPage51 : ISDEPage50
{
    //
    // Raw methods provided by interface
    //

      virtual HRESULT __stdcall get_Visible (
        /*[out,retval]*/ VARIANT_BOOL * Value ) = 0;
      virtual HRESULT __stdcall put_Visible (
        /*[in]*/ VARIANT_BOOL Value ) = 0;
};

enum __declspec(uuid("b68753cf-f9ea-4e5e-99df-bf93b5ac05eb"))
TxVisibilityStatus
{
    vsVisible = 0,
    vsFiltered = 1,
    vsInvisible = 2
};

enum __declspec(uuid("a80b763c-4f3e-4f57-98a8-8bc773858cee"))
TxParamCategory
{
    pcAnother = 0,
    pcLinking = 1,
    pcPersistent = 2,
    pcState = 3,
    pcDrawing = 4,
    pcRegim = 5
};

enum __declspec(uuid("c231265e-f6cd-4e0c-8922-fac3cd271bd7"))
TxAttributeType
{
    atNotDefined = 0,
    atInteger = 1,
    atRestrictedInteger = 2,
    atEnumerated = 3,
    atFloat = 4,
    atString = 5,
    atAggregate = 6,
    atObjRef = 7,
    atColor = 8
};

enum __declspec(uuid("40f998ff-95a6-4757-9a3b-7872150567b3"))
TxPropertySource
{
    psCore = 0,
    psScheme = 1,
    psCustomAggregate = 2
};

enum __declspec(uuid("5c01d593-0acf-47a2-b0a4-31efb3c5aec8"))
TxCopyFlags
{
    cfNonCopy = 1,
    cfAutoInc = 2,
    cfUnique = 4
};

struct __declspec(uuid("2255f30d-2be8-4bfc-b16f-55ec4c5304ce"))
IParamInfo : IDispatch
{
    //
    // Raw methods provided by interface
    //

      virtual HRESULT __stdcall get_Name (
        /*[out,retval]*/ BSTR * Value ) = 0;
      virtual HRESULT __stdcall get_ParMode (
        /*[out,retval]*/ enum TxParamMode * Value ) = 0;
      virtual HRESULT __stdcall put_ParMode (
        /*[in]*/ enum TxParamMode Value ) = 0;
      virtual HRESULT __stdcall get_Category (
        /*[out,retval]*/ enum TxParamCategory * Value ) = 0;
      virtual HRESULT __stdcall put_Category (
        /*[in]*/ enum TxParamCategory Value ) = 0;
      virtual HRESULT __stdcall get_ValuesNo (
        /*[out,retval]*/ long * Value ) = 0;
      virtual HRESULT __stdcall get_Values (
        /*[out,retval]*/ BSTR * Value ) = 0;
      virtual HRESULT __stdcall put_Values (
        /*[in]*/ BSTR Value ) = 0;
      virtual HRESULT __stdcall get_atype (
        /*[out,retval]*/ enum TxAttributeType * Value ) = 0;
      virtual HRESULT __stdcall put_atype (
        /*[in]*/ enum TxAttributeType Value ) = 0;
      virtual HRESULT __stdcall get_ValuesVerb (
        /*[out,retval]*/ BSTR * Value ) = 0;
      virtual HRESULT __stdcall put_ValuesVerb (
        /*[in]*/ BSTR Value ) = 0;
      virtual HRESULT __stdcall get_min (
        /*[out,retval]*/ long * Value ) = 0;
      virtual HRESULT __stdcall put_min (
        /*[in]*/ long Value ) = 0;
      virtual HRESULT __stdcall get_maxindex (
        /*[out,retval]*/ long * Value ) = 0;
      virtual HRESULT __stdcall put_maxindex (
        /*[in]*/ long Value ) = 0;
      virtual HRESULT __stdcall get_Hint (
        /*[out,retval]*/ BSTR * Value ) = 0;
      virtual HRESULT __stdcall put_Hint (
        /*[in]*/ BSTR Value ) = 0;
      virtual HRESULT __stdcall get_ElTypes (
        /*[out,retval]*/ BSTR * Value ) = 0;
      virtual HRESULT __stdcall put_ElTypes (
        /*[in]*/ BSTR Value ) = 0;
      virtual HRESULT __stdcall get_PropertySource (
        /*[out,retval]*/ enum TxPropertySource * Value ) = 0;
      virtual HRESULT __stdcall put_PropertySource (
        /*[in]*/ enum TxPropertySource Value ) = 0;
      virtual HRESULT __stdcall get_CopyFlags (
        /*[out,retval]*/ enum TxCopyFlags * Value ) = 0;
      virtual HRESULT __stdcall put_CopyFlags (
        /*[in]*/ enum TxCopyFlags Value ) = 0;
      virtual HRESULT __stdcall get_AutoIncVal (
        /*[out,retval]*/ long * Value ) = 0;
      virtual HRESULT __stdcall put_AutoIncVal (
        /*[in]*/ long Value ) = 0;
      virtual HRESULT __stdcall get_Help (
        /*[out,retval]*/ long * Value ) = 0;
      virtual HRESULT __stdcall put_Help (
        /*[in]*/ long Value ) = 0;
      virtual HRESULT __stdcall get_IsDefault (
        /*[out,retval]*/ long * Value ) = 0;
      virtual HRESULT __stdcall put_IsDefault (
        /*[in]*/ long Value ) = 0;
      virtual HRESULT __stdcall get_max (
        /*[out,retval]*/ long * Value ) = 0;
      virtual HRESULT __stdcall put_max (
        /*[in]*/ long Value ) = 0;
      virtual HRESULT __stdcall get_DefaultValue (
        /*[out,retval]*/ BSTR * Value ) = 0;
      virtual HRESULT __stdcall put_DefaultValue (
        /*[in]*/ BSTR Value ) = 0;
};

struct __declspec(uuid("df135ed9-33ab-4e8e-8303-4309265c265c"))
IParam : IDispatch
{
    //
    // Raw methods provided by interface
    //

      virtual HRESULT __stdcall get_Name (
        /*[out,retval]*/ BSTR * Value ) = 0;
      virtual HRESULT __stdcall get_ValuesCount (
        /*[out,retval]*/ long * Value ) = 0;
      virtual HRESULT __stdcall get_IndexedValue (
        /*[in]*/ long Index,
        /*[out,retval]*/ BSTR * Value ) = 0;
      virtual HRESULT __stdcall put_IndexedValue (
        /*[in]*/ long Index,
        /*[in]*/ BSTR Value ) = 0;
      virtual HRESULT __stdcall get_Value (
        /*[out,retval]*/ BSTR * Value ) = 0;
      virtual HRESULT __stdcall put_Value (
        /*[in]*/ BSTR Value ) = 0;
      virtual HRESULT __stdcall get_Category (
        /*[out,retval]*/ enum TxParamCategory * Value ) = 0;
      virtual HRESULT __stdcall put_Category (
        /*[in]*/ enum TxParamCategory Value ) = 0;
      virtual HRESULT __stdcall get_Values (
        /*[out,retval]*/ BSTR * Value ) = 0;
      virtual HRESULT __stdcall put_Values (
        /*[in]*/ BSTR Value ) = 0;
      virtual HRESULT __stdcall Info (
        /*[out,retval]*/ struct IParamInfo * * Info ) = 0;
      virtual HRESULT __stdcall get_Mode (
        /*[out,retval]*/ long * Value ) = 0;
      virtual HRESULT __stdcall put_Mode (
        /*[in]*/ long Value ) = 0;
      virtual HRESULT __stdcall get_Dim (
        /*[out,retval]*/ long * Value ) = 0;
      virtual HRESULT __stdcall put_Dim (
        /*[in]*/ long Value ) = 0;
};

struct __declspec(uuid("163b85fc-de71-43fe-88bb-4e40f11e6614"))
IParamsEx : IDispatch
{
    //
    // Raw methods provided by interface
    //

      virtual HRESULT __stdcall Add (
        /*[in]*/ struct IParamInfo * Meta,
        /*[out,retval]*/ struct IParam * * Param ) = 0;
      virtual HRESULT __stdcall Remove (
        /*[in]*/ struct IParam * Param ) = 0;
};

enum __declspec(uuid("e3031fc6-2b33-4818-99a3-5cba6d1355ad"))
TxPersister
{
    psXML = 1,
    psBinary = 2,
    psINI = 3,
    psAuto = 0
};

struct __declspec(uuid("f6b1bd00-e87f-425a-ab9f-c13bffd996cd"))
ISDEObject2 : IDispatch
{
    //
    // Raw methods provided by interface
    //

      virtual HRESULT __stdcall get_SNIdent (
        /*[out,retval]*/ BSTR * Value ) = 0;
      virtual HRESULT __stdcall get_TypeName (
        /*[out,retval]*/ BSTR * Value ) = 0;
      virtual HRESULT __stdcall get_Nodes (
        /*[out,retval]*/ struct INodes42 * * Value ) = 0;
      virtual HRESULT __stdcall get_Elements (
        /*[out,retval]*/ struct ISDEObjects2 * * Value ) = 0;
      virtual HRESULT __stdcall get_Owner (
        /*[out,retval]*/ struct ISDEObject50 * * Value ) = 0;
      virtual HRESULT __stdcall get_Params (
        /*[out,retval]*/ struct IParams52 * * Value ) = 0;
      virtual HRESULT __stdcall get_VisibleExt (
        /*[out,retval]*/ enum TxVisibilityStatus * Value ) = 0;
      virtual HRESULT __stdcall get_ShortPath (
        /*[out,retval]*/ BSTR * Value ) = 0;
      virtual HRESULT __stdcall get_Tag (
        /*[out,retval]*/ long * Value ) = 0;
      virtual HRESULT __stdcall get_OwnerPage (
        /*[out,retval]*/ struct ISDEPage50 * * Value ) = 0;
      virtual HRESULT __stdcall get_OwnerDoc (
        /*[out,retval]*/ struct ISDEDocument50 * * Value ) = 0;
      virtual HRESULT __stdcall get_X (
        /*[in]*/ long Index,
        /*[out,retval]*/ double * Value ) = 0;
      virtual HRESULT __stdcall put_X (
        /*[in]*/ long Index,
        /*[in]*/ double Value ) = 0;
      virtual HRESULT __stdcall get_Y (
        /*[in]*/ long Index,
        /*[out,retval]*/ double * Value ) = 0;
      virtual HRESULT __stdcall put_Y (
        /*[in]*/ long Index,
        /*[in]*/ double Value ) = 0;
      virtual HRESULT __stdcall get_NoOfNodes (
        /*[out,retval]*/ long * Value ) = 0;
      virtual HRESULT __stdcall get_Radius (
        /*[out,retval]*/ double * Value ) = 0;
      virtual HRESULT __stdcall put_Radius (
        /*[in]*/ double Value ) = 0;
      virtual HRESULT __stdcall get_Orient90 (
        /*[out,retval]*/ long * Value ) = 0;
      virtual HRESULT __stdcall put_Orient90 (
        /*[in]*/ long Value ) = 0;
      virtual HRESULT __stdcall get_Document (
        /*[out,retval]*/ struct ISDEDocument50 * * Value ) = 0;
      virtual HRESULT __stdcall get_Page (
        /*[out,retval]*/ struct ISDEPage50 * * Value ) = 0;
      virtual HRESULT __stdcall get_Container (
        /*[out,retval]*/ struct ISDEObject50 * * Value ) = 0;
      virtual HRESULT __stdcall SaveStream (
        /*[in]*/ VARIANT Destination,
        /*[in]*/ enum TxPersister Format ) = 0;
      virtual HRESULT __stdcall LoadStream (
        /*[in]*/ VARIANT Source,
        /*[in]*/ enum TxPersister Format ) = 0;
      virtual HRESULT __stdcall get_RTID (
        /*[out,retval]*/ long * Value ) = 0;
      virtual HRESULT __stdcall get_Tech (
        /*[out,retval]*/ struct ITechObject * * Value ) = 0;
      virtual HRESULT __stdcall get_Techs (
        /*[out,retval]*/ struct INamedPBs * * Value ) = 0;
      virtual HRESULT __stdcall get_ViewParts (
        /*[out,retval]*/ struct INamedPBs * * Value ) = 0;
};

enum __declspec(uuid("5fb4f652-6df8-47d8-98b3-0da2609e4a03"))
TAllowAction
{
    aaAllow = 0,
    aaNotAllow = 1
};

enum __declspec(uuid("932e4d2b-a142-4e0e-b335-9c4cac1f3490"))
ParamSourceType
{
    psSDEDocument = 0,
    psSnapShot = 1,
    psTechSpace = 2
};

enum __declspec(uuid("f27073d6-7a35-4c84-9bab-24cb587337ad"))
MetaSourceType
{
    msSDEDocument = 0,
    msSnapShot = 1,
    msTechSpace = 2
};

enum __declspec(uuid("6fe9fee1-7085-4899-925c-54f7f0dc5ef3"))
TxListType
{
    ltUnknown = 0,
    ltScheme = 1,
    ltContainer = 2,
    ltQueryResult = 3
};

struct __declspec(uuid("701b1666-26e2-4bc6-aaba-cd7caf577f15"))
ISDEObjects42 : ISDEObjects2
{
    //
    // Raw methods provided by interface
    //

      virtual HRESULT __stdcall get_UseTopology (
        /*[out,retval]*/ VARIANT_BOOL * Value ) = 0;
      virtual HRESULT __stdcall put_UseTopology (
        /*[in]*/ VARIANT_BOOL Value ) = 0;
      virtual HRESULT __stdcall get_NeedRecreateElectricModel (
        /*[out,retval]*/ VARIANT_BOOL * Value ) = 0;
      virtual HRESULT __stdcall put_NeedRecreateElectricModel (
        /*[in]*/ VARIANT_BOOL Value ) = 0;
      virtual HRESULT __stdcall get_NodeListCount (
        /*[out,retval]*/ long * Value ) = 0;
      virtual HRESULT __stdcall GetVListByTyp (
        /*[in]*/ long Typ,
        /*[out,retval]*/ struct ISDEObjects2 * * Result ) = 0;
      virtual HRESULT __stdcall get_ListType (
        /*[out,retval]*/ enum TxListType * Value ) = 0;
      virtual HRESULT __stdcall get_RTID (
        /*[out,retval]*/ long * Value ) = 0;
      virtual HRESULT __stdcall get_OwnerView (
        /*[out,retval]*/ struct ISDEObject50 * * Value ) = 0;
      virtual HRESULT __stdcall get_OwnerPage (
        /*[out,retval]*/ struct ISDEPage50 * * Value ) = 0;
      virtual HRESULT __stdcall get_OwnerDoc (
        /*[out,retval]*/ struct ISDEDocument50 * * Value ) = 0;
};

enum __declspec(uuid("1e9b4461-8e7a-41cb-a84a-5ed03a708a7c"))
TxMaskChoice
{
    mcAll = 0,
    mcIntersect = 1,
    mcMatching = 2
};

enum __declspec(uuid("00942634-9342-45f0-a908-2df2b7608513"))
TxPChangeSource
{
    csUnknown = 0,
    csHandle = 1,
    csTele = 2,
    csComm = 3,
    csSensor = 4,
    csBlank = 5,
    csAutoPilot = 6,
    csInstructor = 7,
    csRelay = 8,
    csSwitching = 9,
    csShowRules = 16,
    csParamBlock = 17
};

enum __declspec(uuid("557be551-3159-4e22-9fd6-deb3e475617b"))
TxPChangeMode
{
    csrUnknown = 0,
    csrChange = 256,
    csrInit = 512,
    csrRollback = 768,
    csrSinchro = 1024
};

struct __declspec(uuid("b7416000-cfc1-404a-a692-91b145a49bf8"))
IParam42 : IParam
{
    //
    // Raw methods provided by interface
    //

      virtual HRESULT __stdcall get_PChangeSource (
        /*[out,retval]*/ enum TxPChangeSource * Value ) = 0;
      virtual HRESULT __stdcall put_PChangeSource (
        /*[in]*/ enum TxPChangeSource Value ) = 0;
      virtual HRESULT __stdcall get_PChangeMode (
        /*[out,retval]*/ enum TxPChangeMode * Value ) = 0;
      virtual HRESULT __stdcall put_PChangeMode (
        /*[in]*/ enum TxPChangeMode Value ) = 0;
      virtual HRESULT __stdcall get_SDEObjectRef (
        /*[out,retval]*/ struct ISDEObject50 * * Value ) = 0;
      virtual HRESULT __stdcall put_SDEObjectRef (
        /*[in]*/ struct ISDEObject50 * Value ) = 0;
      virtual HRESULT __stdcall putref_SDEObjectRef (
        /*[in]*/ struct ISDEObject50 * * Value ) = 0;
};

enum __declspec(uuid("5737b366-1efb-440d-98d4-5802a989222c"))
TxNagrState
{
    nsNone = 0,
    nsNagr = 1,
    nsGener = 2,
    nsNagrGener = 3,
    nsSelfNagr = 4,
    nsSecondCircuit = 5
};

enum __declspec(uuid("5b5151e4-07a5-4bcc-8b05-34753ecf4010"))
TxNodeState
{
    nsConnected = 0,
    nsDisconnected = 1,
    nsCoupled = 2,
    nsGrounded = 3
};

enum __declspec(uuid("0fdc5a14-8fed-4b39-93c4-4f66258304f8"))
TxDamagedSet
{
    dsEmpty = 0
};

struct __declspec(uuid("d16e32bd-d1af-46df-b9d7-df3f53f8bd5e"))
ISDEElectricElement : IDispatch
{
    //
    // Raw methods provided by interface
    //

      virtual HRESULT __stdcall get_NagrState (
        /*[out,retval]*/ enum TxNagrState * Value ) = 0;
      virtual HRESULT __stdcall put_NagrState (
        /*[in]*/ enum TxNagrState Value ) = 0;
      virtual HRESULT __stdcall get_NoOfExternalConnector (
        /*[out,retval]*/ long * Value ) = 0;
      virtual HRESULT __stdcall get_LineGround (
        /*[out,retval]*/ VARIANT_BOOL * Value ) = 0;
      virtual HRESULT __stdcall put_LineGround (
        /*[in]*/ VARIANT_BOOL Value ) = 0;
      virtual HRESULT __stdcall get_UnderVoltage (
        /*[out,retval]*/ VARIANT_BOOL * Value ) = 0;
      virtual HRESULT __stdcall put_UnderVoltage (
        /*[in]*/ VARIANT_BOOL Value ) = 0;
      virtual HRESULT __stdcall get_NodeState (
        /*[in]*/ long Index,
        /*[out,retval]*/ enum TxNodeState * Value ) = 0;
      virtual HRESULT __stdcall put_NodeState (
        /*[in]*/ long Index,
        /*[in]*/ enum TxNodeState Value ) = 0;
      virtual HRESULT __stdcall get_DamagedSet (
        /*[out,retval]*/ enum TxDamagedSet * Value ) = 0;
      virtual HRESULT __stdcall put_DamagedSet (
        /*[in]*/ enum TxDamagedSet Value ) = 0;
      virtual HRESULT __stdcall get_Closed (
        /*[out,retval]*/ VARIANT_BOOL * Value ) = 0;
      virtual HRESULT __stdcall put_Closed (
        /*[in]*/ VARIANT_BOOL Value ) = 0;
      virtual HRESULT __stdcall get_VoltageLevel (
        /*[in]*/ long Index,
        /*[out,retval]*/ long * Value ) = 0;
      virtual HRESULT __stdcall put_VoltageLevel (
        /*[in]*/ long Index,
        /*[in]*/ long Value ) = 0;
      virtual HRESULT __stdcall get_Locked (
        /*[out,retval]*/ VARIANT_BOOL * Value ) = 0;
      virtual HRESULT __stdcall put_Locked (
        /*[in]*/ VARIANT_BOOL Value ) = 0;
      virtual HRESULT __stdcall get_OperServo (
        /*[out,retval]*/ VARIANT_BOOL * Value ) = 0;
      virtual HRESULT __stdcall put_OperServo (
        /*[in]*/ VARIANT_BOOL Value ) = 0;
      virtual HRESULT __stdcall get_OperTok (
        /*[out,retval]*/ VARIANT_BOOL * Value ) = 0;
      virtual HRESULT __stdcall put_OperTok (
        /*[in]*/ VARIANT_BOOL Value ) = 0;
      virtual HRESULT __stdcall get_Blinking (
        /*[out,retval]*/ VARIANT_BOOL * Value ) = 0;
      virtual HRESULT __stdcall put_Blinking (
        /*[in]*/ VARIANT_BOOL Value ) = 0;
      virtual HRESULT __stdcall GetViewNodeNum (
        /*[in]*/ long Index,
        /*[out,retval]*/ long * Value ) = 0;
};

enum __declspec(uuid("c3121b65-2fc3-4f45-8209-347d8cdaf2dc"))
TxFindMask
{
    fmEMpty = 0
};

enum __declspec(uuid("76c025e3-6aed-4eef-ab77-7d2a46db2df8"))
TxTypeFormat
{
    tfEmpty = 0
};

struct __declspec(uuid("f6485fde-55dc-47d8-8627-c597379cf402"))
ISDEDocument42 : ISDEDocument
{
    //
    // Raw methods provided by interface
    //

      virtual HRESULT __stdcall get_LightChanging (
        /*[out,retval]*/ VARIANT_BOOL * Value ) = 0;
      virtual HRESULT __stdcall put_LightChanging (
        /*[in]*/ VARIANT_BOOL Value ) = 0;
      virtual HRESULT __stdcall UpdateIndexesObj (
        /*[in]*/ struct ISDEObject50 * Obj ) = 0;
      virtual HRESULT __stdcall ResetIndexes ( ) = 0;
      virtual HRESULT __stdcall get_NeedRecreateElectricModel (
        /*[out,retval]*/ VARIANT_BOOL * Value ) = 0;
      virtual HRESULT __stdcall put_NeedRecreateElectricModel (
        /*[in]*/ VARIANT_BOOL Value ) = 0;
      virtual HRESULT __stdcall FindDamaged (
        /*[in]*/ enum TxFindMask Mask,
        /*[in]*/ enum TxMaskChoice Mode,
        /*[out,retval]*/ struct ISDEObjects2 * * Result ) = 0;
      virtual HRESULT __stdcall get_ProtectionModel (
        /*[out,retval]*/ IUnknown * * Value ) = 0;
      virtual HRESULT __stdcall RefreshIndex (
        /*[in]*/ BSTR Index ) = 0;
      virtual HRESULT __stdcall DeleteIndex (
        /*[in]*/ BSTR Index ) = 0;
      virtual HRESULT __stdcall get_Graphics (
        /*[in]*/ enum TxTypeFormat TypeFormat,
        /*[in]*/ long ObjectRTID,
        /*[in]*/ VARIANT_BOOL DefaultOrient,
        /*[in]*/ VARIANT_BOOL DefaultSize,
        /*[out,retval]*/ VARIANT * Result ) = 0;
};

struct __declspec(uuid("974dd2c0-bb34-409a-abe0-df40bcc9a717"))
ISDEDocument50 : ISDEDocument42
{
    //
    // Raw methods provided by interface
    //

      virtual HRESULT __stdcall get_Techs (
        /*[out,retval]*/ struct INamedPBs * * Value ) = 0;
};

struct __declspec(uuid("abbc76cf-f1be-4be9-b02b-fca4d792a548"))
ISDEDocument51 : ISDEDocument50
{
    //
    // Raw methods provided by interface
    //

      virtual HRESULT __stdcall Clone (
        /*[in]*/ struct ISDEDocument * DestDoc ) = 0;
      virtual HRESULT __stdcall CloneFromFile (
        /*[in]*/ BSTR FileName ) = 0;
      virtual HRESULT __stdcall FindTechsIndex (
        /*[in]*/ BSTR Index,
        /*[in]*/ BSTR Value,
        /*[out,retval]*/ struct ISDEObjects2 * * Elements ) = 0;
};

enum __declspec(uuid("b17306db-48f3-4d41-9599-3ebce155c28b"))
TxParamFlags
{
    pfDisUsed = 1,
    piNotDefined = 2
};

enum __declspec(uuid("30609fab-c61f-4606-abe7-fe9e4e15f0b2"))
TxExecType
{
    etConst = 0,
    etVar = 1,
    etSimpleExpr = 2,
    etTable = 3,
    etVBS = 4,
    etJS = 5,
    etPerl = 6
};

struct __declspec(uuid("7558e134-2968-4a92-a978-4b9678ef180d"))
IActionRec : IDispatch
{
    //
    // Raw methods provided by interface
    //

      virtual HRESULT __stdcall get_Kind (
        /*[out,retval]*/ enum TxExecType * Value ) = 0;
      virtual HRESULT __stdcall put_Kind (
        /*[in]*/ enum TxExecType Value ) = 0;
      virtual HRESULT __stdcall get_Command (
        /*[out,retval]*/ BSTR * Value ) = 0;
      virtual HRESULT __stdcall put_Command (
        /*[in]*/ BSTR Value ) = 0;
      virtual HRESULT __stdcall get_Value (
        /*[out,retval]*/ BSTR * Value ) = 0;
      virtual HRESULT __stdcall put_Value (
        /*[in]*/ BSTR Value ) = 0;
};

enum __declspec(uuid("28ca93ba-2549-4da8-b1e5-665a7bfb0282"))
TxMethodParamOpt
{
    pdIn = 1,
    pdOut = 2,
    pdRetValue = 4,
    pdDefValue = 8,
    pdOptional = 16,
    pdLCID = 32
};

struct __declspec(uuid("42b8c33f-75db-4dc0-a9de-c8d0ead1beb6"))
IMethodInfo : IDispatch
{
    //
    // Raw methods provided by interface
    //

      virtual HRESULT __stdcall get_Name (
        /*[out,retval]*/ BSTR * Value ) = 0;
      virtual HRESULT __stdcall get_ParamsCount (
        /*[out,retval]*/ long * Value ) = 0;
      virtual HRESULT __stdcall get_ParamInfo (
        /*[in]*/ VARIANT Index,
        /*[out]*/ enum TxMethodParamOpt * ParamsOpt,
        /*[out,retval]*/ struct IParamInfo * * Info ) = 0;
};

#pragma pack(push, 8)

struct __declspec(uuid("8b27ab81-5bf6-49d5-a849-82476b4ddd60"))
SDERect
{
    double Left;
    double Top;
    double Right;
    double Bottom;
};

#pragma pack(pop)

struct __declspec(uuid("95a0cfb3-1e7d-488e-b53d-9ac3415aaa73"))
INode42 : INode2
{
    //
    // Raw methods provided by interface
    //

      virtual HRESULT __stdcall get_NodeState (
        /*[out,retval]*/ enum TxNodeState * Value ) = 0;
      virtual HRESULT __stdcall put_NodeState (
        /*[in]*/ enum TxNodeState Value ) = 0;
      virtual HRESULT __stdcall GetConnector (
        /*[out,retval]*/ struct SDERect * Result ) = 0;
};

struct __declspec(uuid("7ffc4552-ff27-404d-bc71-d08b567fc4fa"))
ISDEObject42 : ISDEObject2
{
    //
    // Raw methods provided by interface
    //

      virtual HRESULT __stdcall Clone (
        /*[in]*/ struct ISDEDocument * DestDoc,
        /*[in]*/ struct ISDEPage * DestPage,
        /*[in]*/ double ShiftX,
        /*[in]*/ double ShiftY,
        /*[in]*/ VARIANT_BOOL internal,
        /*[out,retval]*/ struct ISDEObject42 * * Result ) = 0;
      virtual HRESULT __stdcall Shift (
        /*[in]*/ double X,
        /*[in]*/ double Y ) = 0;
      virtual HRESULT __stdcall ChangeType (
        /*[in]*/ VARIANT NewType ) = 0;
      virtual HRESULT __stdcall MinimalDistance (
        /*[in]*/ struct ISDEObject42 * V,
        /*[out,retval]*/ long * Result ) = 0;
      virtual HRESULT __stdcall get_Typ (
        /*[out,retval]*/ long * Value ) = 0;
      virtual HRESULT __stdcall VinheritsFrom (
        /*[in]*/ BSTR AClassName,
        /*[out,retval]*/ VARIANT_BOOL * Result ) = 0;
      virtual HRESULT __stdcall Connect (
        /*[in]*/ VARIANT_BOOL act ) = 0;
      virtual HRESULT __stdcall get_OwnerList (
        /*[out,retval]*/ struct ISDEObjects2 * * Value ) = 0;
      virtual HRESULT __stdcall get_KeyRoleLink (
        /*[out,retval]*/ BSTR * Value ) = 0;
      virtual HRESULT __stdcall get_KeyLink (
        /*[out,retval]*/ BSTR * Value ) = 0;
      virtual HRESULT __stdcall get_RoleLink (
        /*[out,retval]*/ BSTR * Value ) = 0;
      virtual HRESULT __stdcall get_DispName (
        /*[out,retval]*/ BSTR * Value ) = 0;
      virtual HRESULT __stdcall get_EquipType (
        /*[out,retval]*/ BSTR * Value ) = 0;
      virtual HRESULT __stdcall get_DOCRTID (
        /*[out,retval]*/ long * Value ) = 0;
      virtual HRESULT __stdcall get_Bounds (
        /*[out,retval]*/ struct SDERect * Value ) = 0;
      virtual HRESULT __stdcall put_Bounds (
        /*[in]*/ struct SDERect Value ) = 0;
};

struct __declspec(uuid("e4d7fbd1-d54c-4419-bb1d-4be0f3657910"))
ISDEObject50 : ISDEObject42
{
    //
    // Raw methods provided by interface
    //

      virtual HRESULT __stdcall get_SValue (
        /*[in]*/ BSTR ParamNm,
        /*[out,retval]*/ BSTR * Value ) = 0;
      virtual HRESULT __stdcall put_SValue (
        /*[in]*/ BSTR ParamNm,
        /*[in]*/ BSTR Value ) = 0;
      virtual HRESULT __stdcall get_IsContainer (
        /*[out,retval]*/ VARIANT_BOOL * Value ) = 0;
};

struct __declspec(uuid("95b4d131-88e6-4231-b9b7-69e72f33114c"))
IParams50 : IParams
{
    //
    // Raw methods provided by interface
    //

      virtual HRESULT __stdcall SParamInfo (
        /*[in]*/ BSTR ParamNm,
        /*[out,retval]*/ struct IParamInfo * * Info ) = 0;
      virtual HRESULT __stdcall get_SValue (
        /*[in]*/ BSTR ParamNm,
        /*[out,retval]*/ BSTR * Value ) = 0;
      virtual HRESULT __stdcall put_SValue (
        /*[in]*/ BSTR ParamNm,
        /*[in]*/ BSTR Value ) = 0;
      virtual HRESULT __stdcall get_SView (
        /*[out,retval]*/ struct ISDEObject50 * * Value ) = 0;
      virtual HRESULT __stdcall put_SView (
        /*[in]*/ struct ISDEObject50 * Value ) = 0;
      virtual HRESULT __stdcall SAddView (
        /*[in]*/ BSTR ParamNm,
        /*[in]*/ struct ISDEObject50 * SDEObject ) = 0;
      virtual HRESULT __stdcall SDeleteParam (
        /*[in]*/ BSTR ParamNm ) = 0;
      virtual HRESULT __stdcall get_SSubObject (
        /*[in]*/ BSTR ParamNm,
        /*[out,retval]*/ struct IParams50 * * Value ) = 0;
      virtual HRESULT __stdcall SCreateSubObject (
        /*[in]*/ BSTR ParamNm,
        /*[out,retval]*/ struct IParams50 * * Value ) = 0;
      virtual HRESULT __stdcall get_ParamsList (
        /*[in]*/ BSTR Prefix,
        /*[in]*/ long RecursionLevel,
        /*[out,retval]*/ BSTR * Value ) = 0;
      virtual HRESULT __stdcall get_SDisUsed (
        /*[in]*/ BSTR ParamNm,
        /*[out,retval]*/ VARIANT_BOOL * Value ) = 0;
      virtual HRESULT __stdcall put_SDisUsed (
        /*[in]*/ BSTR ParamNm,
        /*[in]*/ VARIANT_BOOL Value ) = 0;
      virtual HRESULT __stdcall FindParam (
        /*[in]*/ BSTR ParamNm,
        /*[out,retval]*/ long * Index ) = 0;
};

struct __declspec(uuid("0c966278-8a18-47da-8a1e-dfd4e2286cc3"))
IParam50 : IParam42
{
    //
    // Raw methods provided by interface
    //

      virtual HRESULT __stdcall get_SubObject (
        /*[out,retval]*/ struct IParams50 * * Value ) = 0;
      virtual HRESULT __stdcall get_Index2Str (
        /*[in]*/ long Index,
        /*[out,retval]*/ BSTR * Value ) = 0;
      virtual HRESULT __stdcall get_Str2Index (
        /*[in]*/ BSTR IndexS,
        /*[out,retval]*/ long * Value ) = 0;
      virtual HRESULT __stdcall get_DisUsed (
        /*[in]*/ VARIANT Index,
        /*[out,retval]*/ VARIANT_BOOL * Value ) = 0;
      virtual HRESULT __stdcall put_DisUsed (
        /*[in]*/ VARIANT Index,
        /*[in]*/ VARIANT_BOOL Value ) = 0;
      virtual HRESULT __stdcall get_Flags (
        /*[in]*/ enum TxParamFlags Index,
        /*[out,retval]*/ VARIANT_BOOL * Value ) = 0;
      virtual HRESULT __stdcall put_Flags (
        /*[in]*/ enum TxParamFlags Index,
        /*[in]*/ VARIANT_BOOL Value ) = 0;
};

struct __declspec(uuid("4db6ca48-c505-4a1d-81ce-8d9545b2547d"))
IParams52 : IParams50
{
    //
    // Raw methods provided by interface
    //

      virtual HRESULT __stdcall get_MethodCount (
        /*[out,retval]*/ long * Value ) = 0;
      virtual HRESULT __stdcall get_MethodInfo (
        /*[in]*/ VARIANT Index,
        /*[out,retval]*/ struct IMethodInfo * * Value ) = 0;
      virtual HRESULT __stdcall MethodRun (
        /*[in]*/ VARIANT Index,
        /*[in,out]*/ VARIANT * Params,
        /*[out,retval]*/ VARIANT * Result ) = 0;
};

#pragma pack(push, 8)

struct __declspec(uuid("fd9dc110-87b7-4290-9e3f-563e29d4038d"))
SDEPoint
{
    double X;
    double Y;
};

#pragma pack(pop)

struct __declspec(uuid("d1b1d336-e8a4-462e-a96a-f7766e250287"))
ISDEObjectInfo : ISDEObject2
{
    //
    // Raw methods provided by interface
    //

      virtual HRESULT __stdcall GetConnectorPoint (
        /*[in]*/ long Index,
        /*[out,retval]*/ struct SDEPoint * Result ) = 0;
      virtual HRESULT __stdcall get_InternalBounds (
        /*[out,retval]*/ struct SDERect * Value ) = 0;
};

#pragma pack(push, 8)

struct __declspec(uuid("c127281e-f58a-4458-be2f-e940a32f538b"))
RParam
{
    __int64 UID;
    BSTR KeyLink;
    BSTR Param;
    BSTR Value;
    DATE DateTime;
    long ActuatorID;
};

#pragma pack(pop)

struct __declspec(uuid("e62af4d8-37ff-4a79-8e1d-ac629ae860e8"))
IRParams : IDispatch
{
    //
    // Raw methods provided by interface
    //

      virtual HRESULT __stdcall get_Count (
        /*[out,retval]*/ long * Value ) = 0;
      virtual HRESULT __stdcall get_Item (
        /*[in]*/ __int64 Index,
        /*[out,retval]*/ struct RParam * Value ) = 0;
      virtual HRESULT __stdcall get__NewEnum (
        /*[out,retval]*/ IUnknown * * Value ) = 0;
};

#pragma pack(push, 4)

union __declspec(uuid("83ce6ffd-7cf4-4199-80fb-a3bd1bdcc79a"))
UKeyState
{
    long ksAltKey;
    long ksShiftKey;
    long ksCtrlKey;
};

#pragma pack(pop)

} // namespace SDECore

#pragma pack(pop)
