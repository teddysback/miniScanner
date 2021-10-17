#pragma once

#include <fltKernel.h>
#include <dontuse.h>
#include <suppress.h>
#include <ntstrsafe.h>

#define WDM_TAG_NAME            'ALUP'

#define WDM_MAX_CONNECTIONS     (1)     // Max client connection allowed to this server


typedef struct _WDM_DRIVER {

    PDRIVER_OBJECT  DriverObject;       // used for WPP_CLEANUP
    PFLT_FILTER     Filter;             // The filter handle that results from a call to FltRegisterFilter.
    PFLT_PORT       ServerPort;         // No more connections are allowed from UM after we close this. Previous connections are not affected
    PFLT_PORT       ClientPort;         // Received in ConnectNotify callback. Used to send messages back to UM

    UNICODE_STRING  Extension;          // File extension received from UM
    BOOLEAN         MonitorExtention;   // Start monitoring received file extension

} WDM_DRIVER, *PWDM_DRIVER;


typedef enum _WDM_FILE_STATE
{
    WdmFileNotScanned = 0,  // The file has not yet been scanned
    WdmFileFuckedUp,        // The file is infected and will be blocked
    WdmFileClean,           // The file is clean

}WDM_FILE_STATE;
