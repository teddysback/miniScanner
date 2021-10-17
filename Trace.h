#pragma once

#define WPP_CONTROL_GUIDS                                                  \
    WPP_DEFINE_CONTROL_GUID(                                               \
        wdmTraceGuid, (6BE5F7C6, CE31, 42C8, 9B78, A223DA5F269D),          \
        WPP_DEFINE_BIT(WDM_ALL_INFO)             /* bit  0 = 0x00000001 */ \
        WPP_DEFINE_BIT(TRACE_DRIVER)             /* bit  1 = 0x00000002 */ \
        )


#define WPP_LEVEL_FLAGS_LOGGER(lvl,flags) \
           WPP_LEVEL_LOGGER(flags)

#define WPP_LEVEL_FLAGS_ENABLED(lvl, flags) \
           (WPP_LEVEL_ENABLED(flags) && WPP_CONTROL(WPP_BIT_ ## flags).Level >= lvl)



#define WPP_LEVEL_FLAGS_FUNCTION_STATUS_LOGGER(lvl, flags, function, status) \
            WPP_LEVEL_LOGGER(flags)

#define WPP_LEVEL_FLAGS_FUNCTION_STATUS_ENABLED(lvl, flags, function, status) \
            (WPP_LEVEL_ENABLED(flags) && WPP_CONTROL(WPP_BIT_ ## flags).Level >= lvl)




//
// This comment block is scanned by the trace preprocessor to define the trace function.
//
// begin_wpp config
//
//
// FUNC TraceEvents(LEVEL, FLAGS, MSG, ...);
// FUNC DbgPrint{LEVEL=TRACE_LEVEL_INFORMATION, FLAGS=TRACE_DRIVER} (MSG, ...);
//
// [!] un logdebug...
// FUNC LogTrace{LEVEL=TRACE_LEVEL_VERBOSE, FLAGS=TRACE_DRIVER}(MSG, ...);
// FUNC LogInfo{LEVEL=TRACE_LEVEL_INFORMATION, FLAGS=TRACE_DRIVER}(MSG, ...);
// FUNC LogWarning{LEVEL=TRACE_LEVEL_WARNING, FLAGS=TRACE_DRIVER}(MSG, ...);
// FUNC LogError{LEVEL=TRACE_LEVEL_ERROR, FLAGS=TRACE_DRIVER}(MSG, ...);
// FUNC LogCritical{LEVEL=TRACE_LEVEL_CRITICAL, FLAGS=TRACE_DRIVER}(MSG, ...);
//
// for NTSTATUS / ntstatus.h
// USEPREFIX (LogErrorNt, "%!STDPREFIX!");
// FUNC LogErrorNt{LEVEL=TRACE_LEVEL_ERROR, FLAGS=TRACE_DRIVER}(FUNCTION, STATUS);
// USESUFFIX (LogErrorNt, "%s failed with %!STATUS!", FUNCTION, STATUS);
//
// for HRESULT / winerror.h
// USEPREFIX (LogErrorHr, "%!STDPREFIX!");
// FUNC LogErrorHr{LEVEL=TRACE_LEVEL_ERROR, FLAGS=TRACE_DRIVER}(FUNCTION, STATUS);
// USESUFFIX (LogErrorHr, "%s failed with %!HRESULT!", FUNCTION, STATUS);
//
// for GetLastError / winerror.h
// USEPREFIX (LogErrorWin, "%!STDPREFIX!");
// FUNC LogErrorWin{LEVEL=TRACE_LEVEL_ERROR, FLAGS=TRACE_DRIVER}(FUNCTION, STATUS);
// USESUFFIX (LogErrorWin, "%s failed with %!WINERROR!", FUNCTION, STATUS);
//
// for other error types
// USEPREFIX (LogErrorHex, "%!STDPREFIX!");
// FUNC LogErrorHex{LEVEL=TRACE_LEVEL_ERROR, FLAGS=TRACE_DRIVER}(FUNCTION, STATUS);
// USESUFFIX (LogErrorHex, "%s failed with 0x%X", FUNCTION, STATUS);
//
//
// end_wpp
//
