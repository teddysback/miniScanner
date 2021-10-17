/*++

Module Name: WdmClient.c
Abstract:    This is the main module of the WdmCLient User-Mode console application.
Environment: User mode

--*/

#include "main.h"

#define WDM_MAX_THREAD_NO           MAXIMUM_WAIT_OBJECTS
#define WDM_DEFAULT_THREAD_NO       (1)

#define EVER                        (;;)

//
//  The context information needed in the Notification thread
//
typedef struct _NOTIFICATION_CONTEXT 
{
    DWORD           Index;
    OVERLAPPED      Ovlp;

} NOTIFICATION_CONTEXT, *PNOTIFICATION_CONTEXT;

//
//  Notification message received from KM
//
typedef struct _NOTIFICATION_MSG 
{
    FILTER_MESSAGE_HEADER   Header;      // Required struct header
    NOTIFICATION            Data;        // Private struct for notifications

}NOTIFICATION_MSG, *PNOTIFICATION_MSG;

#define NOTIFICATION_MSG_SIZE    (sizeof(FILTER_MESSAGE_HEADER) + sizeof(NOTIFICATION))

//
//  Notification reply to send to KM
//
typedef struct _NOTIFICATION_REPLY_MSG 
{
    FILTER_REPLY_HEADER     Header;  // Required struct header
    NOTIFICATION_REPLY      Data;    // Private struct for reply

}NOTIFICATION_REPLY_MSG, *PNOTIFICATION_REPLY_MSG;

#define NOTIFICATION_REPLY_MSG_SIZE    (sizeof(FILTER_REPLY_HEADER) + sizeof(NOTIFICATION_REPLY))


HANDLE                  gFilterPort = NULL;             // Comm port
HANDLE                  gTerminateThreadEvent = NULL;   // Signaled to terminate notification thread
HANDLE                  gCommTh[WDM_MAX_THREAD_NO];     // Notification thread handles
PNOTIFICATION_CONTEXT   gThContext[WDM_MAX_THREAD_NO];  // Notification thread contexts
DWORD                   gThreadNo;                      // Thread count


BOOLEAN
InitComm(
    _In_ DWORD NumberOfThreads
);

VOID
UninitComm(
    VOID
);


VOID
PrintHelp(
    VOID
);

DWORD WINAPI
NotificationWatch(
    LPVOID lpParam
);

BOOLEAN
ParseCmd(
    _Out_ WCHAR Arguments[][MAX_PATH],
    _Out_ PDWORD ArgumentsNr
);

VOID
ProcessInput(
    VOID 
);


int
wmain(int argc, WCHAR *argv[])
{
    UNREFERENCED_PARAMETER(argc);
    UNREFERENCED_PARAMETER(argv);

    __try
    {
        if (!InitComm(WDM_DEFAULT_THREAD_NO))
        {
            LOG_ERROR(0, L"InitComm failed!");
            __leave;
        }
        ProcessInput();
    }
    __finally
    {
        UninitComm();
    }

    return 0;
}

VOID
PrintHelp(
    VOID
)
{
    LOG_HELP(L"Commands:");
    LOG_HELP(L"%s        - show help", CMD_OPT_HELP);
    LOG_HELP(L"%s        - exit client", CMD_OPT_EXIT);
    LOG_HELP(L"%s [.ext] - send extension to driver", CMD_OPT_EXT);
    LOG_HELP(L"%s        - Surprise me feature!", CMD_OPT_BSOD);

    return;
}

DWORD WINAPI
NotificationWatch(
    LPVOID lpParam
)
{
    NOTIFICATION_MSG            msg                             = { 0 };
    NOTIFICATION_REPLY_MSG      rep                             = { 0 };
    DWORD                       waitRes                         = 0;
    DWORD                       cmdOption                       = 0;
    PNOTIFICATION_CONTEXT       thContext                       = NULL;
    INT                         msgboxId                        = IDCONTINUE;
    HRESULT                     hStatus                         = E_FAIL;
    WCHAR                       filePath[MAX_PATH];
    HANDLE                      events[MAXIMUM_WAIT_OBJECTS];

    thContext = (PNOTIFICATION_CONTEXT)lpParam;

    __try
    {
        for EVER
        {
			ZeroMemory(&msg, sizeof(NOTIFICATION_MSG));
			ZeroMemory(&rep, sizeof(NOTIFICATION_REPLY_MSG));

            // Wait for message from driver 
            hStatus = FilterGetMessage(gFilterPort, &msg.Header, NOTIFICATION_MSG_SIZE, &thContext->Ovlp);
            if (HRESULT_FROM_WIN32(ERROR_IO_PENDING) == hStatus)
            {
                events[0] = gTerminateThreadEvent;
                events[1] = thContext->Ovlp.hEvent;

                waitRes = WaitForMultipleObjects(2, events, FALSE, INFINITE);

                if (WAIT_OBJECT_0 == waitRes)
                {
                    LOG_INFO(L"NotificaitionWatch: EXIT, because gTerminateThreadEvent is set");
                    __leave;
                }
                else if (WAIT_OBJECT_0 + 1 == waitRes)
                {
                    LOG_INFO(L"NotificaitionWatch: WAKE, because msg.Ovlp.hEvent is set");
                }
                else
                {
                    LOG_INFO(L"NotificaitionWatch: EXIT, error: %d", waitRes);
                    __leave;
                }

            }
            else if (S_OK != hStatus)
            {
                LOG_ERROR(hStatus, L"invalid result from filtergetmessage => terminate thread");
                __leave;
            }

            // Get KM command from message & Copy file path
            // TODO: make function for Handle and Build reply
            {
                cmdOption = msg.Data.CommandCode;

                switch (cmdOption)
                {
                // Messages received from POST CREATE
                case cmdNotification:

                    swprintf_s(filePath, MAX_PATH, L"%s", msg.Data.FilePath);

                    msgboxId = MessageBox(
                        NULL,
                        (LPCWSTR)filePath,
                        (LPCWSTR)L"Block create?",
                        MB_YESNO | MB_ICONQUESTION | MB_ICONEXCLAMATION
                    );

                    switch (msgboxId)
                    {
                    case IDYES:
                        rep.Data.BlockCreate = TRUE;
                        break;

                    case IDNO:
                    default:
                        rep.Data.BlockCreate = FALSE;
                        break;
                    }

                    rep.Data.CommandCode = cmdOption;
                    break; 

                // UNKOWN
                default:
                    LOG_ERROR(0, L"[CRITICAL] Command Option UNKWON");
                    __leave;
                }
            }

            // Send reply back to KM
            rep.Header.Status = 0;
            rep.Header.MessageId = msg.Header.MessageId;

            hStatus = FilterReplyMessage(gFilterPort, &rep.Header, NOTIFICATION_REPLY_MSG_SIZE);
            if (S_OK != hStatus)
            {
                LOG_ERROR(hStatus, L"FilterReplyMessage failed");
                __leave;
            }
        }
    }
    __finally
    {
    }

    return 0;
}

BOOLEAN
ParseCmd(
    _Out_ WCHAR Arguments[][MAX_PATH],
    _Out_ PDWORD ArgumentsNr
)
{
    DWORD   argsNr = 0;
    WCHAR  line[MAX_PATH];
    PWCHAR  cmd = NULL;
    PWCHAR  buffer = NULL;
    BOOLEAN bFailed = TRUE;

    __try
    {
        wprintf(L">");
        fgetws(line, MAX_PATH, stdin);

        cmd = wcstok_s(line, CMD_DELIMITER, &buffer);
        while (cmd)
        {
            if (argsNr >= CMD_MAX_ARGS)
            {
                LOG_WARN(L"Too many arguments. Max:[%d]", CMD_MAX_ARGS);
                __leave;
            }

            wcscpy_s(Arguments[argsNr++], MAX_PATH, cmd);
            cmd = wcstok_s(NULL, CMD_DELIMITER, &buffer);
        }

        bFailed = FALSE;
    }
    __finally
    {
        if (!bFailed)
        {
            *ArgumentsNr = argsNr;
        }
    }
     
    return !bFailed;
}

VOID
ProcessInput(
    VOID
)
{
    WCHAR   cmd[CMD_MAX_ARGS][MAX_PATH];
    BOOLEAN bExit = FALSE;
    DWORD   cmdLen = 0;
    DWORD   bytesRet = 0;
    HRESULT hStatus = E_FAIL;

    __try
    {
        do
        {
            ZeroMemory(cmd, sizeof(cmd));

            if (!ParseCmd(cmd, &cmdLen))
            {
                LOG_ERROR(0, L"parse_cmd failed");
                continue;
            }

            // EXIT opt
            if (!wcscmp(cmd[0], CMD_OPT_EXIT))
            {
                bExit = TRUE;
            }
            // HELP opt
            else if (!wcscmp(cmd[0], CMD_OPT_HELP))
            {
                PrintHelp();
            }
            // EXT opt
            else if (!wcscmp(cmd[0], CMD_OPT_EXT))
            {
                EXTENSION msg = { 0 };

                if (2 != cmdLen)
                {
                    LOG_WARN(L"cmd EXT expects 1 argument, not [%d]", cmdLen - 1);
                    continue;
                }

                msg.CoomandCode = cmdExtension;

                memcpy_s(msg.Extension, WDM_BUFFER_SIZE, cmd[1], (wcslen(cmd[1]) + 1) * sizeof(WCHAR));
                
                hStatus = FilterSendMessage(gFilterPort, &msg, sizeof(EXTENSION), NULL, 0, &bytesRet);
                if (S_OK != hStatus)
                {
                    LOG_ERROR(hStatus, L"FilterSendMessage failed for [%s]", cmd[1]);
                    continue;
                }
            }
            //else if (!wcscmp(cmd[0], CMD_OPT_BSOD))
            //{
            //    CUSTOM_BSOD msg = { 0 };

            //   /* if (2 != cmdLen)
            //    {
            //        LOG_WARN(L"cmd EXT expects 1 argument, not [%d]", cmdLen - 1);
            //        continue;
            //    }*/

            //    msg.CoomandCode = cmdCustomBsod;


            //    hStatus = FilterSendMessage(
            //        gFilterPort,
            //        &msg,
            //        sizeof(CUSTOM_BSOD),
            //        NULL,
            //        0,
            //        &bytesRet);

            //    if (S_OK != hStatus)
            //    {
            //        LOG_ERROR(hStatus, L"FilterSendMessage failed for [%s]", cmd[1]);
            //        continue;
            //    }
            //}
            else
            {
                LOG_WARN(L"Command [%s] not found", cmd[0]);
                PrintHelp();
            }
        } while (!bExit);
    }
    __finally
    {
    }

    return;
}

BOOLEAN
InitComm(
    _In_ DWORD NumberOfThreads
)
{
    DWORD       i       = 0;
    BOOLEAN     bOk     = FALSE;
    HRESULT     hStatus = E_FAIL;

    if (WDM_MAX_THREAD_NO < NumberOfThreads)
    {
        LOG_ERROR(0, L"NumberOfThreads: %d > max: %d", NumberOfThreads, WDM_MAX_THREAD_NO);
        return bOk;
    }

    __try
    {
        gTerminateThreadEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
        if (!gTerminateThreadEvent)
        {
            LOG_ERROR(GetLastError(), L"CreateEvent failed");
            __leave;
        }

        //  Connect to driver port
        hStatus = FilterConnectCommunicationPort(WDM_PORT_NAME, 0, NULL, 0, NULL, &gFilterPort);
        if (hStatus != S_OK)
        {
            LOG_ERROR(hStatus, L"FilterConnectCommunicationPort failed");
            __leave;
        }

        for (i = 0; i < NumberOfThreads; ++i)
        {
            gThContext[i] = (PNOTIFICATION_CONTEXT)malloc(sizeof(NOTIFICATION_CONTEXT));
            if (!gThContext[i])
            {
                LOG_ERROR(GetLastError(), L"failed for PNOTIFICATION_CONTEXT");
                break;
            }
            ZeroMemory(gThContext[i], sizeof(gThContext[i]));

            gThContext[i]->Index = i;

            gThContext[i]->Ovlp.hEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
            if (!gThContext[i]->Ovlp.hEvent)
            {
                LOG_ERROR(GetLastError(), L"CreateEvent failed");
                __leave;
            }

            gCommTh[i] = CreateThread(NULL, 0, NotificationWatch, (PVOID)gThContext[i], 0, NULL);
            if (!gCommTh[i])
            {
                LOG_ERROR(GetLastError(), L"CreateThread failed for NotificationWatch");
                __leave;
            }
        }

        bOk = TRUE;
    }
    __finally
    {
        if (bOk)
        {
            gThreadNo = i;
        }
    }

    return bOk;
}

VOID
UninitComm(
    VOID
)
{
    DWORD index;

    if (NULL != gTerminateThreadEvent)
    {
        SetEvent(gTerminateThreadEvent);
    }

    if (gThreadNo)
    {
        WaitForMultipleObjects(gThreadNo, gCommTh, TRUE, INFINITE);
    }
    for (index  = 0; index < gThreadNo; ++index)
    {
        CloseHandle(gCommTh[index]);
        gCommTh[index] = NULL;

        if (gThContext[index] != NULL)
        {
            CloseHandle(gThContext[index]->Ovlp.hEvent);
            gThContext[index]->Ovlp.hEvent = NULL;

            free(gThContext[index]);
            gThContext[index] = NULL;
        }
    }
    gThreadNo = 0;

    if (NULL != gTerminateThreadEvent)
    {
        CloseHandle(gTerminateThreadEvent);
        gTerminateThreadEvent = NULL;
    }

    CloseHandle(gFilterPort);
    gFilterPort = NULL;

    return;
}