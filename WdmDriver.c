/*++

Module Name:

    WdmDriver.c

Abstract:

    This is the main module of the WdmDriver miniFilter driver.

Environment:

    Kernel mode

--*/


#include "WdmDriver.h"
#include "Comm.h"
#include "wdmuk.h"
#include "Utility.h"

#include "Trace.h"
#include "WdmDriver.tmh"

/************************
    Prototypes
*************************/

NTSTATUS
DriverEntry(
    _In_ PDRIVER_OBJECT     DriverObject,
    _In_ PUNICODE_STRING    RegistryPath
);

NTSTATUS
DriverUnload(
    FLT_FILTER_UNLOAD_FLAGS Flags
);

FLT_PREOP_CALLBACK_STATUS
WdmPreCreate(
    _Inout_ PFLT_CALLBACK_DATA      Data,
    _In_    PCFLT_RELATED_OBJECTS   FltObjects,
    _Out_   PVOID                   *CompletionContext
);

FLT_POSTOP_CALLBACK_STATUS
WdmPostCreate(
    _Inout_  PFLT_CALLBACK_DATA       Data,
    _In_     PCFLT_RELATED_OBJECTS    FltObjects,
    _In_opt_ PVOID                    CompletionContext,
    _In_     FLT_POST_OPERATION_FLAGS Flags
);

FLT_PREOP_CALLBACK_STATUS
WdmPreClose(
    _Inout_ PFLT_CALLBACK_DATA      Data,
    _In_    PCFLT_RELATED_OBJECTS   FltObjects,
    _Out_   PVOID                   *CompletionContext
);

/************************
    Global variable
*************************/

DRIVER_INITIALIZE   DriverEntry;



typedef struct _WDM_STREAM_CTX
{
    UNICODE_STRING  FileName;           // FileName.Extension
    UNICODE_STRING  FileExtension;      // Extension
    UNICODE_STRING  FilePath;           // Path\FileName.Extension

    LARGE_INTEGER   LastChange;         // Last modification time
    LONG   State;                       // [!] volatile

}WDM_STREAM_CTX, *PWDM_STREAM_CTX;


#define WDM_STREAM_CTX_SIZE     sizeof(WDM_STREAM_CTX)
#define WDM_STREAM_CTX_TAG      'XTC:'


#define WdmInitContext(sc, fi)                                                      \
    {RtlInitUnicodeString(&(sc)->FileName,      (fi)->FinalComponent.Buffer);       \
     RtlInitUnicodeString(&(sc)->FileExtension, (fi)->Extension.Buffer);            \
     RtlInitUnicodeString(&(sc)->FilePath,      (fi)->Name.Buffer);                 \
     (sc)->State = WdmFileNotScanned;                                               \
    }


CONST FLT_CONTEXT_REGISTRATION gContexts[] = {

    {
        FLT_STREAM_CONTEXT,          // ContextType
        0,                           // Flags
        NULL,                        // ContextCleanupCallback
        WDM_STREAM_CTX_SIZE,         // Size
        WDM_STREAM_CTX_TAG           // PoolTag
    },

    { FLT_CONTEXT_END }
};


CONST FLT_OPERATION_REGISTRATION gCallbacks[] = {
    {
        IRP_MJ_CREATE,
        0,
        WdmPreCreate,
        WdmPostCreate
    },
    {
        IRP_MJ_CLOSE,
        0,
        WdmPreClose,
        NULL
    },
    { IRP_MJ_OPERATION_END }

};


CONST FLT_REGISTRATION gFilterRegistration = {

    sizeof(FLT_REGISTRATION),           // Size
    FLT_REGISTRATION_VERSION,           // Version
    0,                                  // Flags

    gContexts,                          // Context
    gCallbacks,                         // Operation callbacks

    DriverUnload,                       // FilterUnload

    NULL,                               // InstanceSetup
    NULL,                               // InstanceQueryTeardown
    NULL,                               // InstanceTeardownStart
    NULL,                               // InstanceTeardownComplete

    NULL,                               // GenerateFileName
    NULL,                               // GenerateDestinationFileName
    NULL                                // NormalizeNameComponent

};

//
//  Structure that contains all the global data structures
//  used throughout the filter.
//
WDM_DRIVER gDriver;


NTSTATUS
DriverEntry(
    _In_ PDRIVER_OBJECT DriverObject,
    _In_ PUNICODE_STRING RegistryPath
)
/*++

Routine Description:

    This is the initialization routine for this miniFilter driver.  This
    registers with FltMgr and initializes all global data structures.

Arguments:

    DriverObject - Pointer to driver object created by the system to
    represent this driver.

    RegistryPath - Unicode string identifying where the parameters for this
    driver are located in the registry.

Return Value:

    Routine can return non success error codes.

--*/
{
    OBJECT_ATTRIBUTES       objAttributes = { 0 };
    UNICODE_STRING          objName = { 0 };
    PSECURITY_DESCRIPTOR    securityDesc = NULL;
    NTSTATUS                status = STATUS_DEVICE_NOT_READY;

    UNREFERENCED_PARAMETER(RegistryPath);

    WPP_INIT_TRACING(DriverObject, RegistryPath);

    LogInfo("\"DriverEntry\" called");

    __try
    {
        gDriver.DriverObject = DriverObject;

        //
        // Register driver
        // 

        status = FltRegisterFilter(
            DriverObject,
            &gFilterRegistration,
            &gDriver.Filter);
        if (!NT_SUCCESS(status))
        {
            LogErrorNt("FltRegisterFilter", status);
            __leave;
        }

        //
        // Create comm port
        //

        status = RtlUnicodeStringInit(&objName, WDM_PORT_NAME);
        if (!NT_SUCCESS(status))
        {
            LogErrorNt("RtlUnicodeStringInit", status);
            __leave;
        }

        status = FltBuildDefaultSecurityDescriptor(&securityDesc, FLT_PORT_ALL_ACCESS);
        if (!NT_SUCCESS(status))
        {
            LogErrorNt("FltBuildDefaultSecurityDescriptor", status);
            __leave;
        }

        InitializeObjectAttributes(
            &objAttributes,
            &objName,
            OBJ_CASE_INSENSITIVE | OBJ_KERNEL_HANDLE,
            NULL,
            securityDesc);

        status = FltCreateCommunicationPort(
            gDriver.Filter,
            &gDriver.ServerPort,
            &objAttributes,
            NULL,
            WdmConnectNotify,
            WdmDisconnectNotify,
            WdmMessageNotify,
            WDM_MAX_CONNECTIONS);
        if (!NT_SUCCESS(status))
        {
            LogErrorNt("FltCreateCommunicationPort", status);
            __leave;
        }

        FltFreeSecurityDescriptor(securityDesc);
        securityDesc = NULL;

        //
        // Start Filtering
        //

        status = FltStartFiltering(gDriver.Filter);
        if (!NT_SUCCESS(status))
        {
            LogErrorNt("FltStartFiltering", status);
            FltUnregisterFilter(gDriver.Filter);
            __leave;
        }

        status = STATUS_SUCCESS;
    }
    __finally
    {
        if (securityDesc != NULL)
        {
            FltFreeSecurityDescriptor(securityDesc);
            securityDesc = NULL;
        }

        if (!NT_SUCCESS(status))
        {
            WPP_CLEANUP(DriverObject);
        }
    }

    return status;
}


NTSTATUS
DriverUnload(
    _In_ FLT_FILTER_UNLOAD_FLAGS Flags
)
{
    NTSTATUS status = STATUS_UNSUCCESSFUL;

    UNREFERENCED_PARAMETER(Flags);

    LogInfo("\"DriverUnload\" called");

    FltCloseCommunicationPort(gDriver.ServerPort);
    LogInfo("Closed communication port");

    FltUnregisterFilter(gDriver.Filter);
    LogInfo("Unregistered filter ");

    WPP_CLEANUP(gDriver.DriverObject);

    if (gDriver.Extension.Buffer != NULL)
    {
        ExFreePoolWithTag(gDriver.Extension.Buffer, WDM_TAG_NAME);
    }

    status = STATUS_SUCCESS;

    return status;
}


FLT_PREOP_CALLBACK_STATUS
WdmPreCreate(
    _Inout_ PFLT_CALLBACK_DATA      Data,
    _In_    PCFLT_RELATED_OBJECTS   FltObjects,
    _Out_   PVOID                   *CompletionContext
)
{
    FLT_PREOP_CALLBACK_STATUS   retStatus = FLT_PREOP_SUCCESS_NO_CALLBACK;

    UNREFERENCED_PARAMETER(Data);
    UNREFERENCED_PARAMETER(FltObjects);

    *CompletionContext = NULL;

    __try
    {
        if (!gDriver.MonitorExtention)
        {
            __leave;
        }

        retStatus = FLT_PREOP_SUCCESS_WITH_CALLBACK;
    }
    __finally
    {

    }

    return retStatus;
}


FLT_POSTOP_CALLBACK_STATUS
WdmPostCreate(
    _Inout_  PFLT_CALLBACK_DATA       Data,
    _In_     PCFLT_RELATED_OBJECTS    FltObjects,
    _In_opt_ PVOID                    CompletionContext,
    _In_     FLT_POST_OPERATION_FLAGS Flags
)
{
    NTSTATUS                    status = STATUS_UNSUCCESSFUL;
    PFLT_FILE_NAME_INFORMATION  fileNameInformation = NULL;
    PWDM_STREAM_CTX             streamCtx = NULL;

    UNREFERENCED_PARAMETER(CompletionContext);
    UNREFERENCED_PARAMETER(Flags);

    __try
    {
        status = WdmGetFniParsed(Data, &fileNameInformation);
        if (!NT_SUCCESS(status))
        {
            __leave;
        }

        if (!RtlEqualUnicodeString(&fileNameInformation->Extension, &gDriver.Extension, FALSE))
        {
            __leave;
        }

        //
        // Get context / Allocate & set context
        //

        status = FltGetStreamContext(
            FltObjects->Instance,
            FltObjects->FileObject,
            &streamCtx);
        if (status == STATUS_NOT_FOUND)
        {
            // Allocate 
            status = FltAllocateContext(
                FltObjects->Filter,
                FLT_STREAM_CONTEXT,
                WDM_STREAM_CTX_SIZE,
                PagedPool,
                &streamCtx);
            if (!NT_SUCCESS(status))
            {
                LogErrorNt("FltAllocateContext", status);
                __leave;
            }
            RtlZeroMemory(streamCtx, sizeof(WDM_STREAM_CTX));
        
            // init
            WdmInitContext(streamCtx, fileNameInformation);

            status = WdmQueryFileBasicInfo(
                FltObjects->Instance, 
                FltObjects->FileObject, 
                &streamCtx->LastChange);
            if (!NT_SUCCESS(status))
            {
                LogErrorNt("WdmQueryFileBasicInfo", status);
            }

            // set
            status = FltSetStreamContext(
                FltObjects->Instance,
                FltObjects->FileObject,
                FLT_SET_CONTEXT_KEEP_IF_EXISTS,
                streamCtx,
                NULL);
            if (!NT_SUCCESS(status) && status != STATUS_FLT_CONTEXT_ALREADY_DEFINED)
            {
                LogErrorNt("FltSetStreamContext", status);
                __leave;
            }
        }
        else if (!NT_SUCCESS(status))
        {
            // STATUS_NOT_SUPPORTED: The file system does not support per-stream contexts for this file stream
            // [!] should I block this?
            LogErrorNt("FltGetStreamContext", status);
            __leave;
        }
        
        if (streamCtx->State == WdmFileNotScanned)
        {
            status = WdmScanFile(streamCtx->FilePath, &(streamCtx->State));
            if (!NT_SUCCESS(status))
            {
                LogErrorNt("WdmScanFile", status);
            }
        }

        switch (streamCtx->State)
        {
        case WdmFileClean:

            Data->IoStatus.Status = STATUS_SUCCESS;
            Data->IoStatus.Information = 0;

            break;

        case WdmFileFuckedUp:

            if (!FlagOn(FltObjects->FileObject->Flags, FO_HANDLE_CREATED))
            {
                FltCancelFileOpen(FltObjects->Instance, FltObjects->FileObject);
            }

            Data->IoStatus.Status = STATUS_ACCESS_DENIED;
            Data->IoStatus.Information = 0;

            break;

        default:
            LogInfo("[error] %d", streamCtx->State);
            LogErrorHex("Unexpected state"  "dare", 0); 

            __debugbreak();

            break;
        }

        status = STATUS_SUCCESS;
    }
    __finally
    {
        if (streamCtx != NULL)
        {
            FltReleaseContext(streamCtx);
        }

        if (fileNameInformation != NULL)
        {
            WdmReleaseFniParsed(&fileNameInformation);
        }
    }

    return FLT_POSTOP_FINISHED_PROCESSING;
}


FLT_PREOP_CALLBACK_STATUS
WdmPreClose(
    _Inout_ PFLT_CALLBACK_DATA      Data,
    _In_    PCFLT_RELATED_OBJECTS   FltObjects,
    _Out_   PVOID                   *CompletionContext
)
{
    NTSTATUS            status = STATUS_UNSUCCESSFUL;
    PWDM_STREAM_CTX     streamCtx = NULL;
    LARGE_INTEGER       lastChange = { 0 };

    UNREFERENCED_PARAMETER(Data);
    UNREFERENCED_PARAMETER(FltObjects);

    *CompletionContext = NULL;

    __try
    {
        if (!gDriver.MonitorExtention)
        {
            __leave;
        }

        status = FltGetStreamContext(FltObjects->Instance, FltObjects->FileObject, &streamCtx);
        if (!NT_SUCCESS(status))
        {
            __leave;
        }

        status = WdmQueryFileBasicInfo(FltObjects->Instance, FltObjects->FileObject, &lastChange);
        if (!NT_SUCCESS(status))
        {
            LogErrorNt("WdmQueryFileBasicInfo", status);
        }

        if (WdmLargeIntCompare(streamCtx->LastChange, lastChange) != sizeof(lastChange))
        {
            LogInfo("[DEBUG] FltDeleteStreamContext");

            status = FltDeleteStreamContext(
                FltObjects->Instance,
                FltObjects->FileObject,
                &streamCtx);
            if (!NT_SUCCESS(status))
            {
                LogErrorNt("FltDeleteStreamContext", status);
                __leave;
            }

            FltReleaseContext(streamCtx);
        }
    }
    __finally
    {
        if (streamCtx != NULL)
        {
            FltReleaseContext(streamCtx);
        }
    }

    return FLT_PREOP_SUCCESS_NO_CALLBACK;
}
