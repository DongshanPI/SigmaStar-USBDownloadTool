#include <windows.h>
#include <windowsx.h>
#include <initguid.h>
#include <devioctl.h>
#include <dbt.h>
#include <stdio.h>
#include <stddef.h>
#include <commctrl.h>
#include <usbioctl.h>
#include <usbiodef.h>

#include <usb.h>
#include <usbuser.h>

#include <objbase.h>
#include <io.h>
#include <conio.h>
#include <shellapi.h>
#include <cfgmgr32.h>
#include <shlwapi.h>
#include <setupapi.h>
#include <winioctl.h>
#include <devpkey.h>
#include <math.h>
#include "USBTreeShowDlg.h"

typedef struct _STRING_DESCRIPTOR_NODE
{
	struct _STRING_DESCRIPTOR_NODE *Next;
	UCHAR                           DescriptorIndex;
	USHORT                          LanguageID;
	USB_STRING_DESCRIPTOR           StringDescriptor[1];
} STRING_DESCRIPTOR_NODE, *PSTRING_DESCRIPTOR_NODE;

typedef struct _DEVICE_GUID_LIST {
	HDEVINFO   DeviceInfo;
	LIST_ENTRY ListHead;
} DEVICE_GUID_LIST, *PDEVICE_GUID_LIST;

typedef struct _DEVICE_INFO_NODE {
	HDEVINFO                         DeviceInfo;
	LIST_ENTRY                       ListEntry;
	SP_DEVINFO_DATA                  DeviceInfoData;
	SP_DEVICE_INTERFACE_DATA         DeviceInterfaceData;
	PSP_DEVICE_INTERFACE_DETAIL_DATA DeviceDetailData;
	PSTR                             DeviceDescName;
	ULONG                            DeviceDescNameLength;
	PSTR                             DeviceDriverName;
	ULONG                            DeviceDriverNameLength;
	DEVICE_POWER_STATE               LatestDevicePowerState;
} DEVICE_INFO_NODE, *PDEVICE_INFO_NODE;

//*****************************************************************************
// D E F I N E S
//*****************************************************************************

#define NUM_STRING_DESC_TO_GET 32
#define MAX_DRIVER_KEY_NAME 256
#define MAX_DEVICE_PROP 200

#define ALLOC(dwBytes) GlobalAlloc(GPTR,(dwBytes))

#define REALLOC(hMem, dwBytes) GlobalReAlloc((hMem), (dwBytes), (GMEM_MOVEABLE|GMEM_ZEROINIT))

#define FREE(hMem)  GlobalFree((hMem))

FORCEINLINE
VOID
InitializeListHead(
	_Out_ PLIST_ENTRY ListHead
)
{
	ListHead->Flink = ListHead->Blink = ListHead;
}

//
//  BOOLEAN
//  IsListEmpty(
//      PLIST_ENTRY ListHead
//      );
//

#define IsListEmpty(ListHead) \
    ((ListHead)->Flink == (ListHead))

//
//  PLIST_ENTRY
//  RemoveHeadList(
//      PLIST_ENTRY ListHead
//      );
//

#define RemoveHeadList(ListHead) \
    (ListHead)->Flink;\
    {RemoveEntryList((ListHead)->Flink)}

//
//  VOID
//  RemoveEntryList(
//      PLIST_ENTRY Entry
//      );
//

#define RemoveEntryList(Entry) {\
    PLIST_ENTRY _EX_Blink;\
    PLIST_ENTRY _EX_Flink;\
    _EX_Flink = (Entry)->Flink;\
    _EX_Blink = (Entry)->Blink;\
    _EX_Blink->Flink = _EX_Flink;\
    _EX_Flink->Blink = _EX_Blink;\
    }

//
//  VOID
//  InsertTailList(
//      PLIST_ENTRY ListHead,
//      PLIST_ENTRY Entry
//      );
//
#define InsertTailList(ListHead,Entry) {\
    PLIST_ENTRY _EX_Blink;\
    PLIST_ENTRY _EX_ListHead;\
    _EX_ListHead = (ListHead);\
    _EX_Blink = _EX_ListHead->Blink;\
    (Entry)->Flink = _EX_ListHead;\
    (Entry)->Blink = _EX_Blink;\
    _EX_Blink->Flink = (Entry);\
    _EX_ListHead->Blink = (Entry);\
    }

extern DEVICE_GUID_LIST    gHubList;
extern DEVICE_GUID_LIST    gDeviceList;
extern PDEVICE_INFO_NODE   gSelectDevice;

extern VOID
EnumerateHostControllers(
	USBTreeShowDlg  *pDlg
);

extern VOID
EnumListInit(
	VOID
);

extern void
ClearDeviceList(
	PDEVICE_GUID_LIST DeviceList
);
