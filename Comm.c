#include "Comm.h"
#include "wdmuk.h"
#include "ntddk.h"

#include "Trace.h"
#include "Comm.tmh"

extern WDM_DRIVER gDriver;


NTSTATUS
WdmConnectNotify(
    _In_ PFLT_PORT ClientPort,
    _In_ PVOID ServerPortCookie,
    _In_ PVOID ConnectionContext,
    _In_ ULONG SizeOfContext,
    _Out_ PVOID *ConnectionPortCookie
)
{
    UNREFERENCED_PARAMETER(ClientPort);
    UNREFERENCED_PARAMETER(ServerPortCookie);
    UNREFERENCED_PARAMETER(ConnectionContext);
    UNREFERENCED_PARAMETER(SizeOfContext);
    UNREFERENCED_PARAMETER(ConnectionPortCookie);

    LogInfo("\"WdmConnectNotify\" was called");

    gDriver.ClientPort = ClientPort;

    return STATUS_SUCCESS;
}


VOID
WdmDisconnectNotify(
    _In_ PVOID ConnectionCookie
)
{
    UNREFERENCED_PARAMETER(ConnectionCookie);

    LogInfo("\"WdmDisconnectNotify\" was called");

    gDriver.ClientPort = NULL;
}


NTSTATUS
WdmMessageNotify(
    _In_  PVOID PortCookie,
    _In_  PVOID InputBuffer OPTIONAL,
    _In_  ULONG InputBufferLength,
    _Out_ PVOID OutputBuffer OPTIONAL,
    _In_  ULONG OutputBufferLength,
    _Out_ PULONG ReturnOutputBufferLength
)
{
    USHORT          extLen      = 0;
    COMMAND_CODE    cmdCode     = -1;
    NTSTATUS        retStatus   = STATUS_UNSUCCESSFUL;
    //NTSTATUS        status      = STATUS_UNSUCCESSFUL;

    UNREFERENCED_PARAMETER(PortCookie);
    UNREFERENCED_PARAMETER(InputBufferLength);
    UNREFERENCED_PARAMETER(OutputBuffer);
    UNREFERENCED_PARAMETER(OutputBufferLength);
    UNREFERENCED_PARAMETER(ReturnOutputBufferLength);

    if (InputBuffer == NULL)
    {
        LogWarning("\"WdmMessageNotify\" InputBuffer is null");
        return STATUS_INVALID_PARAMETER;
    }

    cmdCode = *(PCOMMAND_CODE)InputBuffer;
    LogInfo("\"WdmMessageNotify\" cmdCode: [%d]\n", cmdCode);

    __try
    {

        switch (cmdCode)
        {
            // Set extension and activate monitor
            case cmdExtension:
            {
                PEXTENSION  ext = (PEXTENSION)InputBuffer;

                if (gDriver.Extension.Buffer == NULL)
                {
                    gDriver.Extension.Buffer = ExAllocatePoolWithTag(NonPagedPool, WDM_BUFFER_SIZE * sizeof(WCHAR), WDM_TAG_NAME);
                    if (gDriver.Extension.Buffer == NULL)
                    {
                        LogErrorHex("ExAllocatePoolWithTag", 0);
                        __leave;
                    }
                }
                extLen = (USHORT)wcslen(ext->Extension);
                RtlCopyMemory(gDriver.Extension.Buffer, ext->Extension, (extLen + 1) * sizeof(WCHAR));

                gDriver.Extension.Buffer[extLen] = L'\0';
                gDriver.Extension.Length         = extLen * sizeof(WCHAR);
                gDriver.Extension.MaximumLength  = (USHORT)WDM_BUFFER_SIZE * sizeof(WCHAR);
                gDriver.MonitorExtention         = TRUE;

                LogInfo("Extension set [%wZ] \n", &gDriver.Extension);

                break;
            }
            // Generate custom BSOD
            case cmdCustomBsod:
            {
                // TODO: ...

                break;
            }
            default:
            {
                LogError("Unknown command code received: [%d]", cmdCode);
                __leave;
            }
        }

        retStatus = STATUS_SUCCESS;
    }
    __finally
    {
        if (!NT_SUCCESS(retStatus) && gDriver.Extension.Buffer != NULL)
        {
            ExFreePoolWithTag(gDriver.Extension.Buffer, WDM_TAG_NAME);
            gDriver.Extension.Buffer = NULL;
        }
    }

    return retStatus;
}