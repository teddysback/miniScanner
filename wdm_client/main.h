#pragma once

#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include <Windows.h>
#include <wchar.h>

#include "Fltuser.h"
#include "FltUserStructures.h"

#include "cmd_opts.h"
#include "..\wdmuk.h"

#define __FILENAME__ (strrchr(__FILE__, '\\') ? strrchr(__FILE__, '\\') + 1 : __FILE__) // file name from where it was printed

#define LOG_HELP(M, ...)                wprintf_s(L"[HELP] " M L"\n", __VA_ARGS__)

#ifdef _DEBUG
    #define LOG_INFO(M, ...)            wprintf_s(L"[INFO] " M L"\n", __VA_ARGS__)
    #define LOG_WARN(M, ...)            wprintf_s(L"[WARN] (%S:%d:%S) " M L"\n" , __FILENAME__, __LINE__, __func__, __VA_ARGS__)
    #define LOG_ERROR(err, M, ...)      wprintf_s(L"[ERROR:0x%08x] (%S:%d:%S) " M L"\n", err, __FILENAME__, __LINE__, __func__, __VA_ARGS__)
#else
    #define LOG_INFO(M, ...)            wprintf_s(L"[INFO] " M L"\n", __VA_ARGS__)
    #define LOG_WARN(M, ...)            wprintf_s(L"[WARN] " M L"\n", __VA_ARGS__)
    #define LOG_ERROR(err, M, ...)      wprintf_s(L"[ERROR:0x%08x] " M L"\n", err, __VA_ARGS__)
#endif


