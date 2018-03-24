#pragma once

#include <atlbase.h>

#include <atlapp.h>
#include <atlprint.h>
#include <winspool.h>

class PrintPreviewView : public Controller, public WTL::CPrintPreviewWindow {};
