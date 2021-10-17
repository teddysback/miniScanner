#pragma once

#include "WdmDriver.h"


#define WdmLargeIntCopy(Destination, Source)        RtlCopyMemory((&Destination), (&Source), sizeof(LARGE_INTEGER))
#define WdmLargeIntCompare(Source1, Source2)        RtlCompareMemory((&Source1), (&Source2), sizeof(LARGE_INTEGER))


/*++

Routine Description:

    Scans a file and then set it's state

Arguments:
    
    FilePath - File to be scanned
    
    FileState - Here we return the state of the file after scan

Return Value:
    
    STATUS_SUCCESS - file scanned, and State set (Clean or FuckedUp)
    
    STATUS_UNSUCCESFUL - scan failed, file cannot be trusted, State set to FuckedUp

--*/
NTSTATUS
WdmScanFile(
    _In_    UNICODE_STRING  FilePath,
    _Out_   PLONG           FileState
);

/*++

Warning:
    
    Call WdmReleaseFniParsed() after FileNameInformation is no longer needed

--*/
NTSTATUS
WdmGetFniParsed(
    _In_     PFLT_CALLBACK_DATA          Data,
    _Outptr_ PFLT_FILE_NAME_INFORMATION  *FileNameInformation
);

VOID
WdmReleaseFniParsed(
    _Inout_ PFLT_FILE_NAME_INFORMATION *FileNameInformation
);

NTSTATUS
WdmQueryFileBasicInfo(
    _In_        PFLT_INSTANCE   Instance,
    _In_        PFILE_OBJECT    FileObject,
    _Out_opt_   PLARGE_INTEGER  ChangeTime
);
