// Created by Microsoft (R) C/C++ Compiler Version 10.00.40219.01 (5b6dce9c).
//
// c:\work\workplace\trunk\build-vs10\debug\objs\client\teleclient.tlh
//
// C++ source equivalent of Win32 type library c:\Program Files\Telecontrol\Vidicon\Bin\\TeleClient.dll
// compiler-generated file created 03/06/13 at 15:59:14 - DO NOT EDIT!

#pragma once
#pragma pack(push, 8)

#include <comdef.h>

namespace TeleClientLib {

//
// Forward references and typedefs
//

struct __declspec(uuid("039f6def-60f4-47ef-8c30-893a6830f73e"))
/* LIBID */ __TeleClientLib;
struct __declspec(uuid("9a9362ac-790e-43fe-9b6a-e0e30ded9c0c"))
/* dispinterface */ _IClientEvents;
struct /* coclass */ Client;
struct __declspec(uuid("550feb52-c674-4ec3-9585-13565d879c38"))
/* dual interface */ IClient;
struct __declspec(uuid("17f796c8-9b9b-4268-aea3-b3bc5de84e76"))
/* dual interface */ IDataPoint;
struct __declspec(uuid("5ba438ee-1ff8-4532-afcd-2d9605191bd4"))
/* dispinterface */ _IDataPointEvents;
struct __declspec(uuid("04f3b184-510c-416e-915d-bba6461cc349"))
/* dispinterface */ _IDataPointEventsEx;
struct /* coclass */ DataPoint;
struct __declspec(uuid("1927d656-95ba-4c9a-aa5d-c0d919cbf6ac"))
/* dual interface */ IDataPointServer;

//
// Smart pointer typedef declarations
//

_COM_SMARTPTR_TYPEDEF(_IClientEvents, __uuidof(_IClientEvents));
_COM_SMARTPTR_TYPEDEF(IDataPoint, __uuidof(IDataPoint));
_COM_SMARTPTR_TYPEDEF(IClient, __uuidof(IClient));
_COM_SMARTPTR_TYPEDEF(_IDataPointEvents, __uuidof(_IDataPointEvents));
_COM_SMARTPTR_TYPEDEF(_IDataPointEventsEx, __uuidof(_IDataPointEventsEx));
_COM_SMARTPTR_TYPEDEF(IDataPointServer, __uuidof(IDataPointServer));

//
// Type library items
//

struct __declspec(uuid("9a9362ac-790e-43fe-9b6a-e0e30ded9c0c"))
_IClientEvents : IDispatch
{};

struct __declspec(uuid("cf94c1e4-54b7-4e76-b2fb-8d47f3ced2f8"))
Client;
    // [ default ] interface IClient
    // [ default, source ] dispinterface _IClientEvents

struct __declspec(uuid("17f796c8-9b9b-4268-aea3-b3bc5de84e76"))
IDataPoint : IDispatch
{
    //
    // Raw methods provided by interface
    //

      virtual HRESULT __stdcall get_Value (
        /*[out,retval]*/ VARIANT * pVal ) = 0;
      virtual HRESULT __stdcall put_Value (
        /*[in]*/ VARIANT pVal ) = 0;
      virtual HRESULT __stdcall get_Time (
        /*[out,retval]*/ DATE * pVal ) = 0;
      virtual HRESULT __stdcall get_Quality (
        /*[out,retval]*/ unsigned long * pVal ) = 0;
      virtual HRESULT __stdcall get_ValueStr (
        /*[out,retval]*/ BSTR * pVal ) = 0;
      virtual HRESULT __stdcall get_Attrib (
        /*[in]*/ BSTR Name,
        /*[out,retval]*/ VARIANT * pVal ) = 0;
      virtual HRESULT __stdcall get_Address (
        /*[out,retval]*/ BSTR * pVal ) = 0;
      virtual HRESULT __stdcall get_Prop (
        /*[in]*/ unsigned long ID,
        /*[out,retval]*/ VARIANT * pVal ) = 0;
      virtual HRESULT __stdcall put_Prop (
        /*[in]*/ unsigned long ID,
        /*[in]*/ VARIANT pVal ) = 0;
      virtual HRESULT __stdcall get_XmlNode (
        /*[out,retval]*/ IDispatch * * pVal ) = 0;
      virtual HRESULT __stdcall get_Connected (
        /*[out,retval]*/ unsigned long * pVal ) = 0;
      virtual HRESULT __stdcall get_ErrorStr (
        /*[in]*/ unsigned long Error,
        /*[out,retval]*/ BSTR * pVal ) = 0;
      virtual HRESULT __stdcall get_AccessRights (
        /*[out,retval]*/ unsigned long * pVal ) = 0;
      virtual HRESULT __stdcall Write (
        /*[in]*/ VARIANT Value ) = 0;
      virtual HRESULT __stdcall WriteAsync (
        /*[in]*/ VARIANT Value,
        /*[out,retval]*/ unsigned long * TransID ) = 0;
      virtual HRESULT __stdcall CancelAsync (
        /*[in]*/ unsigned long TransID ) = 0;
      virtual HRESULT __stdcall Ack (
        /*[in]*/ BSTR Acker,
        /*[in]*/ BSTR Comment ) = 0;
};

struct __declspec(uuid("550feb52-c674-4ec3-9585-13565d879c38"))
IClient : IDispatch
{
    //
    // Raw methods provided by interface
    //

      virtual HRESULT __stdcall RequestPoint (
        /*[in]*/ BSTR Address,
        /*[out,retval]*/ struct IDataPoint * * Point ) = 0;
      virtual HRESULT __stdcall get_GlobAttrib (
        /*[in]*/ BSTR Name,
        /*[out,retval]*/ VARIANT * pVal ) = 0;
      virtual HRESULT __stdcall get_XmlConfig (
        /*[in]*/ BSTR Name,
        /*[out,retval]*/ IDispatch * * pVal ) = 0;
      virtual HRESULT __stdcall get_XmlNode (
        /*[in]*/ BSTR Name,
        /*[in]*/ unsigned long ID,
        /*[out,retval]*/ IDispatch * * pVal ) = 0;
      virtual HRESULT __stdcall get_PointProp (
        /*[in]*/ BSTR Name,
        /*[in]*/ unsigned long ID,
        /*[out,retval]*/ VARIANT * pVal ) = 0;
      virtual HRESULT __stdcall Evalute (
        /*[in]*/ BSTR Text,
        /*[out,retval]*/ BSTR * pVal ) = 0;
};

struct __declspec(uuid("5ba438ee-1ff8-4532-afcd-2d9605191bd4"))
_IDataPointEvents : IDispatch
{};

struct __declspec(uuid("04f3b184-510c-416e-915d-bba6461cc349"))
_IDataPointEventsEx : IDispatch
{};

struct __declspec(uuid("be21d8f7-fae1-4024-ac00-a42ec4d02f00"))
DataPoint;
    // [ default ] interface IDataPoint
    // interface IDataPointServer
    // [ default, source ] dispinterface _IDataPointEvents
    // [ source ] dispinterface _IDataPointEventsEx

struct __declspec(uuid("1927d656-95ba-4c9a-aa5d-c0d919cbf6ac"))
IDataPointServer : IDispatch
{
    //
    // Raw methods provided by interface
    //

      virtual HRESULT __stdcall get_OPCServer (
        /*[out,retval]*/ IUnknown * * pVal ) = 0;
};

} // namespace TeleClientLib

#pragma pack(pop)
