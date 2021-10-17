/*++
Module Name:    wdmuk.h
Abstract:       Header file which contains the structures, type definitions,
    constants, global variables and function prototypes that are
    shared between kernel and user mode.

Environment:    Kernel & user mode

--*/

#pragma once


#define WDM_PORT_NAME       L"\\MyPort" // Communication port name for UM access
#define WDM_BUFFER_SIZE     1024        // Max size of buffer 

//
// Command codes used to handle messages 
//
typedef enum _COMMAND_CODE 
{
    cmdNotification = 0,        // km -> um

    cmdExtension,               // um -> km
    cmdCustomBsod

} COMMAND_CODE, *PCOMMAND_CODE;


typedef struct _EXTENSION 
{
    COMMAND_CODE    CoomandCode;
    WCHAR           Extension[WDM_BUFFER_SIZE];

}EXTENSION, *PEXTENSION;

//typedef struct _CUSTOM_BSOD {
//
//    COMMAND_CODE    CoomandCode;
//    //BOOLEAN         
//
//}CUSTOM_BSOD, *PCUSTOM_BSOD;

typedef struct _NOTIFICATION 
{
    COMMAND_CODE    CommandCode;
    WCHAR           FilePath[WDM_BUFFER_SIZE];

}NOTIFICATION, *PNOTIFICATION;

typedef struct _NOTIFICATION_REPLY 
{
    COMMAND_CODE    CommandCode;
    BOOLEAN         BlockCreate;

}NOTIFICATION_REPLY, *PNOTIFICATION_REPLY;
