#pragma once

#include "WdmDriver.h"


NTSTATUS
WdmConnectNotify(
    _In_  PFLT_PORT ClientPort,
    _In_  PVOID ServerPortCookie,
    _In_  PVOID ConnectionContext,
    _In_  ULONG SizeOfContext,
    _Out_ PVOID *ConnectionPortCookie
);

VOID
WdmDisconnectNotify(
    _In_ PVOID ConnectionCookie
);

NTSTATUS
WdmMessageNotify(
    _In_  PVOID PortCookie,
    _In_  PVOID InputBuffer OPTIONAL,
    _In_  ULONG InputBufferLength,
    _Out_ PVOID OutputBuffer OPTIONAL,
    _In_  ULONG OutputBufferLength,
    _Out_ PULONG ReturnOutputBufferLength
);
