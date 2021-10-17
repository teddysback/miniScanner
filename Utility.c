#include "Utility.h"
#include "wdmuk.h"

#include "Trace.h"
#include "Utility.tmh"


extern WDM_DRIVER gDriver;


NTSTATUS
WdmQueryFileBasicInfo(
    _In_        PFLT_INSTANCE   Instance,
    _In_        PFILE_OBJECT    FileObject,
    _Out_opt_   PLARGE_INTEGER  ChangeTime
)
{
    NTSTATUS                status          = STATUS_UNSUCCESSFUL;
    FILE_BASIC_INFORMATION  fileBasicInfo   = { 0 };

    __try
    {
        status = FltQueryInformationFile(
            Instance,
            FileObject,
            &fileBasicInfo,
            sizeof(fileBasicInfo),
            FileBasicInformation,
            NULL);
        if (!NT_SUCCESS(status))
        {
            LogErrorNt("FltQueryInformationFile", status);
            __leave;
        }

        status = STATUS_SUCCESS;
    }
    __finally
    {
        if (ChangeTime != NULL)
        {
            if (NT_SUCCESS(status))
            {
                WdmLargeIntCopy(*ChangeTime, fileBasicInfo.ChangeTime);
            }
            else
            {
                RtlZeroMemory(ChangeTime, sizeof(*ChangeTime));
            }
        }
    }

    return status;
}


NTSTATUS
WdmGetFniParsed(
    _In_     PFLT_CALLBACK_DATA          Data,
    _Outptr_ PFLT_FILE_NAME_INFORMATION  *FileNameInformation
)
{
    NTSTATUS                    retStatus           = STATUS_UNSUCCESSFUL;
    NTSTATUS                    status              = STATUS_UNSUCCESSFUL;
    PFLT_FILE_NAME_INFORMATION  fileNameInformation = NULL;


    __try
    {
        status = FltGetFileNameInformation(
            Data,
            FLT_FILE_NAME_OPENED | FLT_FILE_NAME_QUERY_DEFAULT,
            &fileNameInformation);
        if (!NT_SUCCESS(status))
        {
            //LogErrorNt("FltGetFileNameInformation", status);
            __leave;
        }

        status = FltParseFileNameInformation(fileNameInformation);
        if (!NT_SUCCESS(status))
        {
            __leave;
        }

        retStatus = STATUS_SUCCESS;
    }
    __finally
    {
        if (NT_SUCCESS(retStatus))
        {
            *FileNameInformation = fileNameInformation;
        }
        else
        {
            *FileNameInformation = NULL;
        }
    }

    return retStatus;
}


VOID
WdmReleaseFniParsed(
    _Inout_ PFLT_FILE_NAME_INFORMATION *FileNameInformation
)
{
    ASSERT(*FileNameInformation != NULL);

    FltReleaseFileNameInformation(*FileNameInformation);
    *FileNameInformation = NULL;

    return;
}


NTSTATUS
WdmScanFile(
    _In_    UNICODE_STRING  FilePath,
    _Out_   PLONG           FileState
)
{
    NTSTATUS            status = STATUS_UNSUCCESSFUL;
    ULONG               repLength = 0;
    NOTIFICATION_REPLY  repData = { 0 };
    NOTIFICATION        msgData = { 0 };

    __try
    {
        repLength = sizeof(FILTER_REPLY_HEADER) + sizeof(NOTIFICATION_REPLY);
       
        msgData.CommandCode = cmdNotification;

        RtlCopyMemory(msgData.FilePath, FilePath.Buffer, FilePath.Length + 1);
        msgData.FilePath[FilePath.Length + 1] = '\0';

        status = FltSendMessage(
            gDriver.Filter,
            &gDriver.ClientPort,
            &msgData,
            sizeof(NOTIFICATION),
            &repData,
            &repLength,
            NULL);
        if (!NT_SUCCESS(status))
        {
            LogErrorNt("FltSendMessage", status);
            __leave;
        }

        status = STATUS_SUCCESS;
    }
    __finally
    {
        if (NT_SUCCESS(status))
        {
            if (repData.BlockCreate)
            {
                InterlockedExchange(FileState, WdmFileFuckedUp);
            }
            else
            {
                InterlockedExchange(FileState, WdmFileClean);
            }
        }
        else
        {
            // scan failed, we cannot trust this file
            InterlockedExchange(FileState, WdmFileFuckedUp);
        }
    }

    return status;
}