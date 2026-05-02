// Created by Microsoft (R) C/C++ Compiler Version 10.00.40219.01 (f411267f).
//
// c:\work\workplace\trunk\build-vs10\debug\objs\client\telecontrolview.tlh
//
// C++ source equivalent of Win32 type library c:\Program Files\Telecontrol\Vidicon\Bin\\TelecontrolView.tlb
// compiler-generated file created 03/06/13 at 16:11:52 - DO NOT EDIT!

#pragma once
#pragma pack(push, 8)

#include <comdef.h>

namespace TelecontrolView {

//
// Forward references and typedefs
//

struct __declspec(uuid("7eca7e37-c0e0-4649-9db4-81d18bb137d5"))
/* LIBID */ __TelecontrolView;
struct __declspec(uuid("614bd075-1fd4-4cdb-b4d4-04f7121541b8"))
/* dual interface */ ITelecontrolView;

//
// Smart pointer typedef declarations
//

_COM_SMARTPTR_TYPEDEF(ITelecontrolView, __uuidof(ITelecontrolView));

//
// Type library items
//

struct __declspec(uuid("614bd075-1fd4-4cdb-b4d4-04f7121541b8"))
ITelecontrolView : IDispatch
{
    //
    // Raw methods provided by interface
    //

      virtual HRESULT __stdcall SetClient (
        /*[in]*/ IDispatch * Client ) = 0;
};

} // namespace TelecontrolView

#pragma pack(pop)
