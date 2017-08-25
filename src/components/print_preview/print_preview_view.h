#pragma once

#include <winspool.h>
#include <atlbase.h>
#include <atlapp.h>
#include <atlprint.h>

class PrintPreviewView : public Controller,
                         public WTL::CPrintPreviewWindow
{
};