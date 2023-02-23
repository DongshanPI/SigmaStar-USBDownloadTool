/*++
    
Copyright (c) 1997-2011 Microsoft Corporation

Module Name:

    ENUM.C

Abstract:

    This source file contains the routines which enumerate the USB bus
    and populate the TreeView control.

    The enumeration process goes like this:

    (1) Enumerate Host Controllers and Root Hubs
    EnumerateHostControllers()
    EnumerateHostController()
    Host controllers currently have symbolic link names of the form HCDx,
    where x starts at 0.  Use CreateFile() to open each host controller
    symbolic link.  Create a node in the TreeView to represent each host
    controller.

    GetRootHubName()
    After a host controller has been opened, send the host controller an
    IOCTL_USB_GET_ROOT_HUB_NAME request to get the symbolic link name of
    the root hub that is part of the host controller.

    (2) Enumerate Hubs (Root Hubs and External Hubs)
    EnumerateHub()
    Given the name of a hub, use CreateFile() to map the hub.  Send the
    hub an IOCTL_USB_GET_NODE_INFORMATION request to get info about the
    hub, such as the number of downstream ports.  Create a node in the
    TreeView to represent each hub.

    (3) Enumerate Downstream Ports
    EnumerateHubPorts()
    Given an handle to an open hub and the number of downstream ports on
    the hub, send the hub an IOCTL_USB_GET_NODE_CONNECTION_INFORMATION_EX
    request for each downstream port of the hub to get info about the
    device (if any) attached to each port.  If there is a device attached
    to a port, send the hub an IOCTL_USB_GET_NODE_CONNECTION_NAME request
    to get the symbolic link name of the hub attached to the downstream
    port.  If there is a hub attached to the downstream port, recurse to
    step (2).  
    
    GetAllStringDescriptors()
    GetConfigDescriptor()
    Create a node in the TreeView to represent each hub port
    and attached device.


Environment:

    user mode

Revision History:

    04-25-97 : created

--*/

//*****************************************************************************
// I N C L U D E S
//*****************************************************************************
#include "stdafx.h"
#include "enum.h"

//*****************************************************************************
// L O C A L    F U N C T I O N    P R O T O T Y P E S
//*****************************************************************************

VOID
EnumerateHostControllers (
	USBTreeShowDlg  *pDlg
);

VOID
EnumerateHostController (
	USBTreeShowDlg           *pDlg,
    HANDLE                   hHCDev,
    _Inout_ PCHAR            leafName,
    _In_    HANDLE           deviceInfo,
    _In_    PSP_DEVINFO_DATA deviceInfoData
);

VOID
EnumerateHub (
	USBTreeShowDlg						            *pDlg,
	HTREEITEM                                       hTreeParent,
	_In_opt_ PUSB_NODE_CONNECTION_INFORMATION_EX    ConnectionInfo,
    _In_reads_(cbHubName) PCHAR                     HubName,
    _In_ size_t                                     cbHubName
);

VOID
EnumerateHubPorts (
	USBTreeShowDlg						            *pDlg,
	HTREEITEM                                       hTreeParent,
    HANDLE      hHubDevice,
    ULONG       NumPorts
);

PCHAR GetRootHubName (
    HANDLE HostController
);

PCHAR GetExternalHubName (
    HANDLE  Hub,
    ULONG   ConnectionIndex
);

PCHAR GetDriverKeyName (
    HANDLE  Hub,
    ULONG   ConnectionIndex
);

PCHAR WideStrToMultiStr ( 
     _In_reads_bytes_(cbWideStr) PWCHAR WideStr, 
     _In_ size_t                   cbWideStr
     );

BOOL
SetUpdateFlagByGetStringCmd(
	HANDLE  hHubDevice,
	ULONG   ConnectionIndex
);

void
EnumerateAllDevices();


void
EnumerateAllDevicesWithGuid(
    PDEVICE_GUID_LIST DeviceList, 
    LPGUID Guid
    );

void
FreeDeviceInfoNode(
    _In_ PDEVICE_INFO_NODE *ppNode
    );

PDEVICE_INFO_NODE
FindMatchingDeviceNodeForDriverName(
    _In_ PSTR    DriverKeyName,
    _In_ BOOLEAN IsHub
    );

void
ClearDeviceList(
	PDEVICE_GUID_LIST DeviceList
);

//*****************************************************************************
// G L O B A L S
//*****************************************************************************

// List of enumerated host controllers.
//
LIST_ENTRY EnumeratedHCListHead =
{
    &EnumeratedHCListHead,
    &EnumeratedHCListHead
};

DEVICE_GUID_LIST    gHubList;
DEVICE_GUID_LIST    gDeviceList;
PDEVICE_INFO_NODE   gSelectDevice = NULL;
//*****************************************************************************
// G L O B A L S    P R I V A T E    T O    T H I S    F I L E
//*****************************************************************************

PCHAR ConnectionStatuses[] =
{
    "",                   // 0  - NoDeviceConnected
    "",                   // 1  - DeviceConnected
    "FailedEnumeration",  // 2  - DeviceFailedEnumeration
    "GeneralFailure",     // 3  - DeviceGeneralFailure
    "Overcurrent",        // 4  - DeviceCausedOvercurrent
    "NotEnoughPower",     // 5  - DeviceNotEnoughPower
    "NotEnoughBandwidth", // 6  - DeviceNotEnoughBandwidth
    "HubNestedTooDeeply", // 7  - DeviceHubNestedTooDeeply
    "InLegacyHub",        // 8  - DeviceInLegacyHub
    "Enumerating",        // 9  - DeviceEnumerating
    "Reset"               // 10 - DeviceReset
};

ULONG TotalDevicesConnected;
ULONG TotalHubs;

//*****************************************************************************
//
// EnumerateHostControllers()
//
// hTreeParent - Handle of the TreeView item under which host controllers
// should be added.
//
//*****************************************************************************
VOID EnumListInit(VOID)
{
	gHubList.DeviceInfo = INVALID_HANDLE_VALUE;
	InitializeListHead(&gHubList.ListHead);
	gDeviceList.DeviceInfo = INVALID_HANDLE_VALUE;
	InitializeListHead(&gDeviceList.ListHead);
}

VOID EnumerateHostControllers (
	USBTreeShowDlg  *pDlg
)
{
    HANDLE                           hHCDev = NULL;
    HDEVINFO                         deviceInfo = NULL;
    SP_DEVINFO_DATA                  deviceInfoData;
    SP_DEVICE_INTERFACE_DATA         deviceInterfaceData;
    PSP_DEVICE_INTERFACE_DETAIL_DATA  deviceDetailData;
    ULONG                            index = 0;
    ULONG                            requiredLength = 0;
    BOOL                             success;

    TotalHubs = 0;
	//log("%s %d\n", __func__, __LINE__);
    EnumerateAllDevices();

    // Iterate over host controllers using the new GUID based interface
    //

    deviceInfo = SetupDiGetClassDevs((LPGUID)&GUID_CLASS_USB_HOST_CONTROLLER,
                                     NULL,
                                     NULL,
                                     (DIGCF_PRESENT | DIGCF_DEVICEINTERFACE));

    deviceInfoData.cbSize = sizeof(SP_DEVINFO_DATA);

    for (index=0;
         SetupDiEnumDeviceInfo(deviceInfo,
                               index,
                               &deviceInfoData);
         index++)
    {
        deviceInterfaceData.cbSize = sizeof(SP_DEVICE_INTERFACE_DATA);

        success = SetupDiEnumDeviceInterfaces(deviceInfo,
                                              0,
                                              (LPGUID)&GUID_CLASS_USB_HOST_CONTROLLER,
                                              index,
                                              &deviceInterfaceData);

        if (!success)
        {
            break;
        }

        success = SetupDiGetDeviceInterfaceDetail(deviceInfo,
                                                  &deviceInterfaceData,
                                                  NULL,
                                                  0,
                                                  &requiredLength,
                                                  NULL);

        if (!success && GetLastError() != ERROR_INSUFFICIENT_BUFFER)
        {
            break;
        }

        deviceDetailData = (PSP_DEVICE_INTERFACE_DETAIL_DATA)ALLOC(requiredLength);
        if (deviceDetailData == NULL)
        {
            break;
        }

        deviceDetailData->cbSize = sizeof(SP_DEVICE_INTERFACE_DETAIL_DATA);

        success = SetupDiGetDeviceInterfaceDetail(deviceInfo,
                                                  &deviceInterfaceData,
                                                  deviceDetailData,
                                                  requiredLength,
                                                  &requiredLength,
                                                  NULL);

        if (!success)
        {
            break;
        }

        hHCDev = CreateFile(deviceDetailData->DevicePath,
                            GENERIC_WRITE,
                            FILE_SHARE_WRITE,
                            NULL,
                            OPEN_EXISTING,
                            0,
                            NULL);

        // If the handle is valid, then we've successfully opened a Host
        // Controller.  Display some info about the Host Controller itself,
        // then enumerate the Root Hub attached to the Host Controller.
        //
        if (hHCDev != INVALID_HANDLE_VALUE)
        {
            EnumerateHostController(pDlg,
									hHCDev,
                                    deviceDetailData->DevicePath,
                                    deviceInfo,
                                    &deviceInfoData);

            CloseHandle(hHCDev);
        }

        FREE(deviceDetailData);
    }

    SetupDiDestroyDeviceInfoList(deviceInfo);

    return;
}

//*****************************************************************************
//
// EnumerateHostController()
//
// hTreeParent - Handle of the TreeView item under which host controllers
// should be added.
//
//*****************************************************************************

VOID
EnumerateHostController (
	USBTreeShowDlg           *pDlg,
    HANDLE                   hHCDev,    
	_Inout_ PCHAR            leafName,
    _In_    HANDLE           deviceInfo,
    _In_    PSP_DEVINFO_DATA deviceInfoData
)
{
    PCHAR                   driverKeyName = NULL;
    PCHAR                   rootHubName = NULL;
    PLIST_ENTRY             listEntry = NULL;
    //DWORD                   dwSuccess;
    BOOL                    success = FALSE;
    ULONG                   deviceAndFunction = 0;
	HTREEITEM               hHCItem = NULL;

#if 0
	hHCItem = pDlg->m_usbTree.InsertItem("Root Hub ¿ØÖÆÆ÷", 1, 1, pDlg->root, TVI_LAST);
#else
	TV_INSERTSTRUCT tvins;
	// Set the parent item
	//
	tvins.hParent = pDlg->root;
	// pszText and lParam members are valid
	//
	tvins.item.mask = TVIF_TEXT | TVIF_PARAM;

	// Set the text of the item.
	//
	tvins.item.pszText = "Root Hub ¿ØÖÆÆ÷";

	// Set the user context item
	//
	tvins.item.lParam = NULL;

	// Add the item to the tree-view control.
	//
	hHCItem = pDlg->m_usbTree.InsertItem(&tvins);
#endif
    // Get the name of the root hub for this host
    // controller and then enumerate the root hub.
    //
	if (NULL == hHCItem)
	{
		return;
	}

    rootHubName = GetRootHubName(hHCDev);

    if (rootHubName != NULL)
    {
        size_t cbHubName = 0;
        HRESULT hr = S_OK;

        hr = StringCbLength(rootHubName, MAX_DRIVER_KEY_NAME, &cbHubName);
        if (SUCCEEDED(hr))
        {
            EnumerateHub(pDlg,
						 hHCItem,
						 NULL,
						 rootHubName,
                         cbHubName);
        }
    }

	if (rootHubName)
	{
		FREE(rootHubName);
	}
    return;
}


//*****************************************************************************
//
// EnumerateHub()
//
// hTreeParent - Handle of the TreeView item under which this hub should be
// added.
//
// HubName - Name of this hub.  This pointer is kept so the caller can neither
// free nor reuse this memory.
//
// ConnectionInfo - NULL if this is a root hub, else this is the connection
// info for an external hub.  This pointer is kept so the caller can neither
// free nor reuse this memory.
//
// ConfigDesc - NULL if this is a root hub, else this is the Configuration
// Descriptor for an external hub.  This pointer is kept so the caller can
// neither free nor reuse this memory.
//
// StringDescs - NULL if this is a root hub.
//
// DevProps - Device properties of the hub
//
//*****************************************************************************

VOID
EnumerateHub (
	USBTreeShowDlg						            *pDlg,
	HTREEITEM                                       hTreeParent,
	_In_opt_ PUSB_NODE_CONNECTION_INFORMATION_EX    ConnectionInfo,
    _In_reads_(cbHubName) PCHAR                     HubName,
    _In_ size_t                                     cbHubName
)
{
    // Initialize locals to not allocated state so the error cleanup routine
    // only tries to cleanup things that were successfully allocated.
    //
    PUSB_NODE_INFORMATION    hubInfo = NULL;
    HANDLE                  hHubDevice = INVALID_HANDLE_VALUE;
    PCHAR                   deviceName = NULL;
    ULONG                   nBytes = 0;
    BOOL                    success = 0;
    HRESULT                 hr = S_OK;
    size_t                  cchHeader = 0;
    size_t                  cchFullHubName = 0;
	HTREEITEM               hItem = NULL;
	DWORD                   dwSizeOfLeafName = 0;
	CHAR                    leafName[512] = { 0 };

	// Allocate some space for a USB_NODE_INFORMATION structure for this Hub
	//
	hubInfo = (PUSB_NODE_INFORMATION)ALLOC(sizeof(USB_NODE_INFORMATION));
	if (hubInfo == NULL)
	{
		goto EnumerateHubError;
	}

    // Allocate a temp buffer for the full hub device name.
    //
    hr = StringCbLength("\\\\.\\", MAX_DEVICE_PROP, &cchHeader);
    if (FAILED(hr))
    {
        goto EnumerateHubError;
    }
    cchFullHubName = cchHeader + cbHubName + 1;
    deviceName = (PCHAR)ALLOC((DWORD) cchFullHubName);
    if (deviceName == NULL)
    {
        goto EnumerateHubError;
    }

    // Create the full hub device name
    //
    hr = StringCchCopyN(deviceName, cchFullHubName, "\\\\.\\", cchHeader);
    if (FAILED(hr))
    {
        goto EnumerateHubError;
    }
    hr = StringCchCatN(deviceName, cchFullHubName, HubName, cbHubName);
    if (FAILED(hr))
    {
        goto EnumerateHubError;
    }

    // Try to hub the open device
    //
    hHubDevice = CreateFile(deviceName,
                            GENERIC_WRITE,
                            FILE_SHARE_WRITE,
                            NULL,
                            OPEN_EXISTING,
                            0,
                            NULL);

	// Done with temp buffer for full hub device name
	//
	FREE(deviceName);

	if (hHubDevice == INVALID_HANDLE_VALUE)
	{
		goto EnumerateHubError;
	}

	//
	// Now query USBHUB for the USB_NODE_INFORMATION structure for this hub.
	// This will tell us the number of downstream ports to enumerate, among
	// other things.
	//
	success = DeviceIoControl(hHubDevice,
		IOCTL_USB_GET_NODE_INFORMATION,
		hubInfo,
		sizeof(USB_NODE_INFORMATION),
		hubInfo,
		sizeof(USB_NODE_INFORMATION),
		&nBytes,
		NULL);

	if (!success)
	{
		goto EnumerateHubError;
	}

	dwSizeOfLeafName = sizeof(leafName);
	if (ConnectionInfo != NULL) {
		StringCchPrintf(leafName, dwSizeOfLeafName, "[Port%d] ", ConnectionInfo->ConnectionIndex);
		StringCchCat(leafName,
			dwSizeOfLeafName,
			ConnectionStatuses[ConnectionInfo->ConnectionStatus]);
		StringCchCatN(leafName,
			dwSizeOfLeafName - 1,
			" :  ",
			sizeof(" :  "));
		// External hub
		StringCchCatN(leafName,
			dwSizeOfLeafName - 1,
			"ExternelHub",
			sizeof("ExternelHub"));
	}
	else
	{
		// Root hub
		StringCchCatN(leafName,
			dwSizeOfLeafName - 1,
			"RootHub",
			sizeof("RootHub"));
	}
#if 0
	hItem = pDlg->m_usbTree.InsertItem(HubName, 1, 1, hTreeParent, TVI_LAST);
#else
	TV_INSERTSTRUCT tvins;
	// Set the parent item
	//
	tvins.hParent = hTreeParent;
	// pszText and lParam members are valid
	//
	tvins.item.mask = TVIF_TEXT | TVIF_PARAM;

	// Set the text of the item.
	//
	tvins.item.pszText = leafName;

	// Set the user context item
	//
	tvins.item.lParam = NULL;

	// Add the item to the tree-view control.
	//
	hItem = pDlg->m_usbTree.InsertItem(&tvins);
#endif
	if (hItem == NULL)
	{
		goto EnumerateHubError;
	}
    // Now recursively enumerate the ports of this hub.
    //
    EnumerateHubPorts(
		pDlg,
		hItem,
        hHubDevice,
        hubInfo->u.HubInformation.HubDescriptor.bNumberOfPorts
        );

	FREE(hubInfo);
    CloseHandle(hHubDevice);
    return;

EnumerateHubError:
    //
    // Clean up any stuff that got allocated
    //

	if (hubInfo)
	{
		FREE(hubInfo);
	}

    if (hHubDevice != INVALID_HANDLE_VALUE)
    {
        CloseHandle(hHubDevice);
        hHubDevice = INVALID_HANDLE_VALUE;
    }
}

//*****************************************************************************
//
// EnumerateHubPorts()
//
// hTreeParent - Handle of the TreeView item under which the hub port should
// be added.
//
// hHubDevice - Handle of the hub device to enumerate.
//
// NumPorts - Number of ports on the hub.
//
//*****************************************************************************

VOID
EnumerateHubPorts (
	USBTreeShowDlg						            *pDlg,
	HTREEITEM                                       hTreeParent,
    HANDLE											hHubDevice,
    ULONG											NumPorts
)
{
	BOOL        success = 0;
	ULONG       index = 0;
	ULONG       nBytesEx;
	ULONG       nBytes = 0;
	HRESULT     hr = S_OK;
	PCHAR       driverKeyName = NULL;
	CHAR        leafName[512];
	PDEVICE_INFO_NODE                      pNode = NULL;
	PUSB_NODE_CONNECTION_INFORMATION_EX    connectionInfoEx;
	PSTRING_DESCRIPTOR_NODE                stringDescs = NULL;
    //PUSB_DESCRIPTOR_REQUEST                configDesc;

    // Loop over all ports of the hub.
    //
    // Port indices are 1 based, not 0 based.
    //
    for (index = 1; index <= NumPorts; index++)
    {
		//
		// Allocate space to hold the connection info for this port.
		// For now, allocate it big enough to hold info for 30 pipes.
		//
		// Endpoint numbers are 0-15.  Endpoint number 0 is the standard
		// control endpoint which is not explicitly listed in the Configuration
		// Descriptor.  There can be an IN endpoint and an OUT endpoint at
		// endpoint numbers 1-15 so there can be a maximum of 30 endpoints
		// per device configuration.
		//
		// Should probably size this dynamically at some point.
		//

		nBytesEx = sizeof(USB_NODE_CONNECTION_INFORMATION_EX) +
			(sizeof(USB_PIPE_INFO) * 30);

		connectionInfoEx = (PUSB_NODE_CONNECTION_INFORMATION_EX)ALLOC(nBytesEx);

		if (connectionInfoEx == NULL)
		{
			break;
		}
		connectionInfoEx->ConnectionIndex = index;

		success = DeviceIoControl(hHubDevice,
								  IOCTL_USB_GET_NODE_CONNECTION_INFORMATION_EX,
								  connectionInfoEx,
								  nBytesEx,
								  connectionInfoEx,
								  nBytesEx,
								  &nBytesEx,
								  NULL);

		if (success)
		{
			//
			// Since the USB_NODE_CONNECTION_INFORMATION_EX is used to display
			// the device speed, but the hub driver doesn't support indication
			// of superspeed, we overwrite the value if the super speed
			// data structures are available and indicate the device is operating
			// at SuperSpeed.
			// 

			/*if (connectionInfoEx->Speed == UsbHighSpeed
				&& connectionInfoExV2 != NULL
				&& (connectionInfoExV2->Flags.DeviceIsOperatingAtSuperSpeedOrHigher ||
					connectionInfoExV2->Flags.DeviceIsOperatingAtSuperSpeedPlusOrHigher))
			{
				connectionInfoEx->Speed = UsbSuperSpeed;
			}*/
		}
		else
		{
			PUSB_NODE_CONNECTION_INFORMATION    connectionInfo = NULL;

			// Try using IOCTL_USB_GET_NODE_CONNECTION_INFORMATION
			// instead of IOCTL_USB_GET_NODE_CONNECTION_INFORMATION_EX
			//

			nBytes = sizeof(USB_NODE_CONNECTION_INFORMATION) +
				sizeof(USB_PIPE_INFO) * 30;

			connectionInfo = (PUSB_NODE_CONNECTION_INFORMATION)ALLOC(nBytes);

			if (connectionInfo == NULL)
			{
				continue;
			}

			connectionInfo->ConnectionIndex = index;

			success = DeviceIoControl(hHubDevice,
									  IOCTL_USB_GET_NODE_CONNECTION_INFORMATION,
									  connectionInfo,
									  nBytes,
									  connectionInfo,
									  nBytes,
									  &nBytes,
									  NULL);

			if (!success)
			{
				FREE(connectionInfo);
				FREE(connectionInfoEx);
				continue;
			}

			// Copy IOCTL_USB_GET_NODE_CONNECTION_INFORMATION into
			// IOCTL_USB_GET_NODE_CONNECTION_INFORMATION_EX structure.
			//
			connectionInfoEx->ConnectionIndex = connectionInfo->ConnectionIndex;
			connectionInfoEx->DeviceDescriptor = connectionInfo->DeviceDescriptor;
			connectionInfoEx->CurrentConfigurationValue = connectionInfo->CurrentConfigurationValue;
			connectionInfoEx->Speed = connectionInfo->LowSpeed ? UsbLowSpeed : UsbFullSpeed;
			connectionInfoEx->DeviceIsHub = connectionInfo->DeviceIsHub;
			connectionInfoEx->DeviceAddress = connectionInfo->DeviceAddress;
			connectionInfoEx->NumberOfOpenPipes = connectionInfo->NumberOfOpenPipes;
			connectionInfoEx->ConnectionStatus = connectionInfo->ConnectionStatus;

			memcpy(&connectionInfoEx->PipeList[0],
				&connectionInfo->PipeList[0],
				sizeof(USB_PIPE_INFO) * 30);

			FREE(connectionInfo);
		}
		//log("Port[%d]: %04x %04x\n", index, connectionInfoEx->DeviceDescriptor.idVendor, connectionInfoEx->DeviceDescriptor.idProduct);
		// Update the count of connected devices
		//
		if (connectionInfoEx->ConnectionStatus == DeviceConnected)
		{
			TotalDevicesConnected++;
		}

		if (connectionInfoEx->DeviceIsHub)
		{
			TotalHubs++;
		}

		// If there is a device connected, get the Device Description
		//

		if (connectionInfoEx->ConnectionStatus != NoDeviceConnected)
		{
			driverKeyName = GetDriverKeyName(hHubDevice, index);

			if (driverKeyName)
			{
				size_t cbDriverName = 0;

				hr = StringCbLength(driverKeyName, MAX_DRIVER_KEY_NAME, &cbDriverName);
				if (SUCCEEDED(hr))
				{
					/*DevProps = DriverNameToDeviceProperties(driverKeyName, cbDriverName);*/
					pNode = FindMatchingDeviceNodeForDriverName(driverKeyName, connectionInfoEx->DeviceIsHub);
					//if (pNode)
						//log("%s(%d)Port[%d]: %s %s\n", __func__, __LINE__, index, pNode->DeviceDescName, driverKeyName);
				}
				FREE(driverKeyName);
			}
		}

		if (connectionInfoEx->DeviceIsHub)
		{
			PCHAR extHubName;
			size_t cbHubName = 0;

			extHubName = GetExternalHubName(hHubDevice, index);
			if (extHubName != NULL)
			{
				hr = StringCbLength(extHubName, MAX_DRIVER_KEY_NAME, &cbHubName);
				if (SUCCEEDED(hr))
				{
					EnumerateHub(pDlg,
						hTreeParent,
						connectionInfoEx,
						extHubName,
						cbHubName);
				}
				FREE(extHubName);
			}
		}
		else
		{
			// Allocate some space for a USBDEVICEINFO structure to hold the
			// hub info, hub name, and connection info pointers.  GPTR zero
			// initializes the structure for us.
			//

			StringCchPrintf(leafName, sizeof(leafName), "[Port%d] ", index);

			// Add error description if ConnectionStatus is other than NoDeviceConnected / DeviceConnected
			StringCchCat(leafName,
				sizeof(leafName),
				ConnectionStatuses[connectionInfoEx->ConnectionStatus]);

			if (pNode)
			{
				size_t dwSizeOfLeafName = sizeof(leafName);
				size_t cchDeviceDescName = 0;
				StringCchCatN(leafName,
					dwSizeOfLeafName - 1,
					" :  ",
					sizeof(" :  "));

				hr = StringCbLength(pNode->DeviceDescName, MAX_DEVICE_PROP, &cchDeviceDescName);
				if (FAILED(hr))
				{
					FREE(connectionInfoEx);
					return;
				}
				StringCchCatN(leafName,
					dwSizeOfLeafName - 1,
					pNode->DeviceDescName,
					cchDeviceDescName);
			}

#if 0
			pDlg->m_usbTree.InsertItem(leafName, 1, 1, hTreeParent, TVI_LAST);
#else
			TV_INSERTSTRUCT tvins;
			// Set the parent item
			//
			tvins.hParent = hTreeParent;
			// pszText and lParam members are valid
			//
			tvins.item.mask = TVIF_TEXT | TVIF_PARAM;

			// Set the text of the item.
			//
			tvins.item.pszText = leafName;

			// Set the user context item
			//
			tvins.item.lParam = (LPARAM)pNode;

			// Add the item to the tree-view control.
			//
			pDlg->m_usbTree.InsertItem(&tvins);
#endif
			if (gSelectDevice 
				&& gSelectDevice->DeviceDriverName
				&& pNode
				&& pNode->DeviceDriverName) {
				//log("gSelectDevice: %s\n", gSelectDevice->DeviceDriverName);
				//log("Port[%d]: %s %s\n", index, pNode->DeviceDescName, pNode->DeviceDriverName);
				if (_stricmp(gSelectDevice->DeviceDriverName, pNode->DeviceDriverName) == 0)
				{
					pDlg->flag = SetUpdateFlagByGetStringCmd(hHubDevice,
												index);
					FREE(gSelectDevice->DeviceDriverName);
					FREE(gSelectDevice);
					gSelectDevice = NULL;
				}
			}
			pNode = NULL;
		}
		FREE(connectionInfoEx);
    } // for
}


//*****************************************************************************
//
// WideStrToMultiStr()
//
//*****************************************************************************

PCHAR WideStrToMultiStr ( 
                         _In_reads_bytes_(cbWideStr) PWCHAR WideStr, 
                         _In_ size_t                   cbWideStr
                         )
{
    ULONG  nBytes = 0;
    PCHAR  MultiStr = NULL;
    PWCHAR pWideStr = NULL;

    // Use local string to guarantee zero termination
    pWideStr = (PWCHAR) ALLOC((DWORD) cbWideStr + sizeof(WCHAR));
    if (NULL == pWideStr)
    {
        return NULL;
    }
    memset(pWideStr, 0, cbWideStr + sizeof(WCHAR));
    memcpy(pWideStr, WideStr, cbWideStr);

    // Get the length of the converted string
    //
    nBytes = WideCharToMultiByte(
                 CP_ACP,
                 WC_NO_BEST_FIT_CHARS,
                 pWideStr,
                 -1,
                 NULL,
                 0,
                 NULL,
                 NULL);

    if (nBytes == 0)
    {
        FREE(pWideStr);
        return NULL;
    }

    // Allocate space to hold the converted string
    //
    MultiStr = (PCHAR)ALLOC(nBytes);
    if (MultiStr == NULL)
    {
        FREE(pWideStr);
        return NULL;
    }

    // Convert the string
    //
    nBytes = WideCharToMultiByte(
                 CP_ACP,
                 WC_NO_BEST_FIT_CHARS,
                 pWideStr,
                 -1,
                 MultiStr,
                 nBytes,
                 NULL,
                 NULL);

    if (nBytes == 0)
    {
        FREE(MultiStr);
        FREE(pWideStr);
        return NULL;
    }

    FREE(pWideStr);
    return MultiStr;
}

//*****************************************************************************
//
// GetRootHubName()
//
//*****************************************************************************

PCHAR GetRootHubName (
    HANDLE HostController
)
{
    BOOL                success = 0;
    ULONG               nBytes = 0;
    USB_ROOT_HUB_NAME   rootHubName;
    PUSB_ROOT_HUB_NAME  rootHubNameW = NULL;
    PCHAR               rootHubNameA = NULL;

    // Get the length of the name of the Root Hub attached to the
    // Host Controller
    //
    success = DeviceIoControl(HostController,
                              IOCTL_USB_GET_ROOT_HUB_NAME,
                              0,
                              0,
                              &rootHubName,
                              sizeof(rootHubName),
                              &nBytes,
                              NULL);

    if (!success)
    {
        goto GetRootHubNameError;
    }

    // Allocate space to hold the Root Hub name
    //
    nBytes = rootHubName.ActualLength;

    rootHubNameW = (PUSB_ROOT_HUB_NAME)ALLOC(nBytes);
    if (rootHubNameW == NULL)
    {
        goto GetRootHubNameError;
    }

    // Get the name of the Root Hub attached to the Host Controller
    //
    success = DeviceIoControl(HostController,
                              IOCTL_USB_GET_ROOT_HUB_NAME,
                              NULL,
                              0,
                              rootHubNameW,
                              nBytes,
                              &nBytes,
                              NULL);
    if (!success)
    {
        goto GetRootHubNameError;
    }

    // Convert the Root Hub name
    //
    rootHubNameA = WideStrToMultiStr(rootHubNameW->RootHubName, nBytes - sizeof(USB_ROOT_HUB_NAME) + sizeof(WCHAR));

    // All done, free the uncoverted Root Hub name and return the
    // converted Root Hub name
    //
    FREE(rootHubNameW);

    return rootHubNameA;

GetRootHubNameError:
    // There was an error, free anything that was allocated
    //
    if (rootHubNameW != NULL)
    {
        FREE(rootHubNameW);
        rootHubNameW = NULL;
    }
    return NULL;
}


//*****************************************************************************
//
// GetExternalHubName()
//
//*****************************************************************************

PCHAR GetExternalHubName (
    HANDLE  Hub,
    ULONG   ConnectionIndex
)
{
    BOOL                        success = 0;
    ULONG                       nBytes = 0;
    USB_NODE_CONNECTION_NAME    extHubName;
    PUSB_NODE_CONNECTION_NAME   extHubNameW = NULL;
    PCHAR                       extHubNameA = NULL;

    // Get the length of the name of the external hub attached to the
    // specified port.
    //
    extHubName.ConnectionIndex = ConnectionIndex;

    success = DeviceIoControl(Hub,
                              IOCTL_USB_GET_NODE_CONNECTION_NAME,
                              &extHubName,
                              sizeof(extHubName),
                              &extHubName,
                              sizeof(extHubName),
                              &nBytes,
                              NULL);

    if (!success)
    {
        goto GetExternalHubNameError;
    }

    // Allocate space to hold the external hub name
    //
    nBytes = extHubName.ActualLength;

    if (nBytes <= sizeof(extHubName))
    {
        goto GetExternalHubNameError;
    }

    extHubNameW = (PUSB_NODE_CONNECTION_NAME)ALLOC(nBytes);

    if (extHubNameW == NULL)
    {
        goto GetExternalHubNameError;
    }

    // Get the name of the external hub attached to the specified port
    //
    extHubNameW->ConnectionIndex = ConnectionIndex;

    success = DeviceIoControl(Hub,
                              IOCTL_USB_GET_NODE_CONNECTION_NAME,
                              extHubNameW,
                              nBytes,
                              extHubNameW,
                              nBytes,
                              &nBytes,
                              NULL);

    if (!success)
    {
        goto GetExternalHubNameError;
    }

    // Convert the External Hub name
    //
    extHubNameA = WideStrToMultiStr(extHubNameW->NodeName, nBytes - sizeof(USB_NODE_CONNECTION_NAME) + sizeof(WCHAR));

    // All done, free the uncoverted external hub name and return the
    // converted external hub name
    //
    FREE(extHubNameW);

    return extHubNameA;


GetExternalHubNameError:
    // There was an error, free anything that was allocated
    //
    if (extHubNameW != NULL)
    {
        FREE(extHubNameW);
        extHubNameW = NULL;
    }

    return NULL;
}


//*****************************************************************************
//
// GetDriverKeyName()
//
//*****************************************************************************

PCHAR GetDriverKeyName (
    HANDLE  Hub,
    ULONG   ConnectionIndex
)
{
    BOOL                                success = 0;
    ULONG                               nBytes = 0;
    USB_NODE_CONNECTION_DRIVERKEY_NAME  driverKeyName;
    PUSB_NODE_CONNECTION_DRIVERKEY_NAME driverKeyNameW = NULL;
    PCHAR                               driverKeyNameA = NULL;

    // Get the length of the name of the driver key of the device attached to
    // the specified port.
    //
    driverKeyName.ConnectionIndex = ConnectionIndex;

    success = DeviceIoControl(Hub,
                              IOCTL_USB_GET_NODE_CONNECTION_DRIVERKEY_NAME,
                              &driverKeyName,
                              sizeof(driverKeyName),
                              &driverKeyName,
                              sizeof(driverKeyName),
                              &nBytes,
                              NULL);

    if (!success)
    {
        goto GetDriverKeyNameError;
    }

    // Allocate space to hold the driver key name
    //
    nBytes = driverKeyName.ActualLength;

    if (nBytes <= sizeof(driverKeyName))
    {
        goto GetDriverKeyNameError;
    }

    driverKeyNameW = (PUSB_NODE_CONNECTION_DRIVERKEY_NAME)ALLOC(nBytes);
    if (driverKeyNameW == NULL)
    {
        goto GetDriverKeyNameError;
    }

    // Get the name of the driver key of the device attached to
    // the specified port.
    //
    driverKeyNameW->ConnectionIndex = ConnectionIndex;

    success = DeviceIoControl(Hub,
                              IOCTL_USB_GET_NODE_CONNECTION_DRIVERKEY_NAME,
                              driverKeyNameW,
                              nBytes,
                              driverKeyNameW,
                              nBytes,
                              &nBytes,
                              NULL);

    if (!success)
    {
        goto GetDriverKeyNameError;
    }

    // Convert the driver key name
    //
    driverKeyNameA = WideStrToMultiStr(driverKeyNameW->DriverKeyName, nBytes - sizeof(USB_NODE_CONNECTION_DRIVERKEY_NAME) + sizeof(WCHAR));

    // All done, free the uncoverted driver key name and return the
    // converted driver key name
    //
    FREE(driverKeyNameW);

    return driverKeyNameA;


GetDriverKeyNameError:
    // There was an error, free anything that was allocated
    //
    if (driverKeyNameW != NULL)
    {
        FREE(driverKeyNameW);
        driverKeyNameW = NULL;
    }

    return NULL;
}

BOOL
SetUpdateFlagByGetStringCmd(
	HANDLE  hHubDevice,
	ULONG   ConnectionIndex
)
{
	BOOL    success = 0;
	ULONG   nBytes = 0;
	ULONG   nBytesReturned = 0;
	UCHAR   stringDescReqBuf[sizeof(USB_DESCRIPTOR_REQUEST) +
		MAXIMUM_USB_STRING_LENGTH];

	PUSB_DESCRIPTOR_REQUEST stringDescReq = NULL;
	PUSB_STRING_DESCRIPTOR  stringDesc = NULL;
	//PSTRING_DESCRIPTOR_NODE stringDescNode = NULL;
	//
	// We use this special descriptor index(0xff) as Firmware Update command for gadget driver.
	// None of the gadget drivers should use this index for standard get string operation.
	//
	UCHAR         DescriptorIndex = 0xff;
	const CHAR   *specialStringDesc = "Set UFU Firmware Update OK";

	nBytes = sizeof(stringDescReqBuf);

	stringDescReq = (PUSB_DESCRIPTOR_REQUEST)stringDescReqBuf;
	stringDesc = (PUSB_STRING_DESCRIPTOR)(stringDescReq + 1);

	// Zero fill the entire request structure
	//
	memset(stringDescReq, 0, nBytes);

	// Indicate the port from which the descriptor will be requested
	//
	stringDescReq->ConnectionIndex = ConnectionIndex;

	//
	// USBHUB uses URB_FUNCTION_GET_DESCRIPTOR_FROM_DEVICE to process this
	// IOCTL_USB_GET_DESCRIPTOR_FROM_NODE_CONNECTION request.
	//
	// USBD will automatically initialize these fields:
	//     bmRequest = 0x80
	//     bRequest  = 0x06
	//
	// We must inititialize these fields:
	//     wValue    = Descriptor Type (high) and Descriptor Index (low byte)
	//     wIndex    = Zero (or Language ID for String Descriptors)
	//     wLength   = Length of descriptor buffer
	//
	stringDescReq->SetupPacket.wValue = (USB_STRING_DESCRIPTOR_TYPE << 8) | DescriptorIndex;

	stringDescReq->SetupPacket.wIndex = 0;

	stringDescReq->SetupPacket.wLength = (USHORT)(nBytes - sizeof(USB_DESCRIPTOR_REQUEST));

	// Now issue the get descriptor request.
	//
	success = DeviceIoControl(hHubDevice,
		IOCTL_USB_GET_DESCRIPTOR_FROM_NODE_CONNECTION,
		stringDescReq,
		nBytes,
		stringDescReq,
		nBytes,
		&nBytesReturned,
		NULL);
	log("%s to port %d OK\n", __func__, ConnectionIndex);
	//
	// Do some sanity checks on the return from the get descriptor request.
	//

	if (!success)
	{
		return FALSE;
	}

	if (nBytesReturned < 2)
	{
		return FALSE;
	}

	if (stringDesc->bDescriptorType != USB_STRING_DESCRIPTOR_TYPE)
	{
		return FALSE;
	}

	if (stringDesc->bLength != nBytesReturned - sizeof(USB_DESCRIPTOR_REQUEST))
	{
		return FALSE;
	}

	if (stringDesc->bLength % 2 != 0)
	{
		return FALSE;
	}

	//log("Get string:%s\n", stringDesc->bString);
	if (memcmp(stringDesc->bString, specialStringDesc, strlen(specialStringDesc)) == 0)
		return TRUE;

	return FALSE;
}

void
EnumerateAllDevices()
{
    EnumerateAllDevicesWithGuid(&gDeviceList, 
                                (LPGUID)&GUID_DEVINTERFACE_USB_DEVICE);

    EnumerateAllDevicesWithGuid(&gHubList, 
                                (LPGUID)&GUID_DEVINTERFACE_USB_HUB);
}



_Success_(return == TRUE)
BOOL
GetDeviceProperty(
    _In_    HDEVINFO         DeviceInfoSet,
    _In_    PSP_DEVINFO_DATA DeviceInfoData,
    _In_    DWORD            Property,
    _Outptr_  LPTSTR        *ppBuffer
    )
{
    BOOL bResult;
    DWORD requiredLength = 0;
    DWORD lastError;

    if (ppBuffer == NULL)
    {
        return FALSE;
    }

    *ppBuffer = NULL;

    bResult = SetupDiGetDeviceRegistryProperty(DeviceInfoSet,
                                               DeviceInfoData,
                                               Property ,
                                               NULL,
                                               NULL,
                                               0,
                                               &requiredLength);
    lastError = GetLastError();

    if ((requiredLength == 0) || (bResult != FALSE && lastError != ERROR_INSUFFICIENT_BUFFER))
    {
        return FALSE;
    }

    *ppBuffer = (LPTSTR)ALLOC(requiredLength);

    if (*ppBuffer == NULL)
    {
        return FALSE;
    }

    bResult = SetupDiGetDeviceRegistryProperty(DeviceInfoSet,
                                                DeviceInfoData,
                                                Property ,
                                                NULL,
                                                (PBYTE) *ppBuffer,
                                                requiredLength,
                                                &requiredLength);
    if(bResult == FALSE)
    {
        FREE(*ppBuffer);
        *ppBuffer = NULL;
        return FALSE;
    }

    return TRUE;
}


void
EnumerateAllDevicesWithGuid(
    PDEVICE_GUID_LIST DeviceList, 
    LPGUID Guid
    )
{
    if (DeviceList->DeviceInfo != INVALID_HANDLE_VALUE)
    {
        ClearDeviceList(DeviceList);
    }

    DeviceList->DeviceInfo = SetupDiGetClassDevs(Guid,
                                     NULL,
                                     NULL,
                                     (DIGCF_PRESENT | DIGCF_DEVICEINTERFACE));

    if (DeviceList->DeviceInfo != INVALID_HANDLE_VALUE)
    {
        ULONG                    index;
        DWORD error;

        error = 0;
        index = 0;

        while (error != ERROR_NO_MORE_ITEMS)
        {
            BOOL success;
            PDEVICE_INFO_NODE pNode;

            pNode = (PDEVICE_INFO_NODE)ALLOC(sizeof(DEVICE_INFO_NODE));
            if (pNode == NULL)
            {
                break;
            }
            pNode->DeviceInfo = DeviceList->DeviceInfo;
            pNode->DeviceInterfaceData.cbSize = sizeof(SP_DEVICE_INTERFACE_DATA);
            pNode->DeviceInfoData.cbSize = sizeof(SP_DEVINFO_DATA);

            success = SetupDiEnumDeviceInfo(DeviceList->DeviceInfo,
                                            index,
                                            &pNode->DeviceInfoData);

            index++;

            if (success == FALSE)
            {
                error = GetLastError();

                if (error != ERROR_NO_MORE_ITEMS)
                {

                }

                FreeDeviceInfoNode(&pNode);
            }
            else
            {
                BOOL   bResult;
                ULONG  requiredLength;

                bResult = GetDeviceProperty(DeviceList->DeviceInfo,
                                            &pNode->DeviceInfoData,
                                            SPDRP_DEVICEDESC,
                                            &pNode->DeviceDescName);
                if (bResult == FALSE)
                {
                    FreeDeviceInfoNode(&pNode);
                    break;
                }

                bResult = GetDeviceProperty(DeviceList->DeviceInfo,
                                            &pNode->DeviceInfoData,
                                            SPDRP_DRIVER,
                                            &pNode->DeviceDriverName);
                if (bResult == FALSE)
                {
                    FreeDeviceInfoNode(&pNode);
                    break;
                }

                pNode->DeviceInterfaceData.cbSize = sizeof(SP_DEVICE_INTERFACE_DATA);
        
                success = SetupDiEnumDeviceInterfaces(DeviceList->DeviceInfo,
                                                      0,
                                                      Guid,
                                                      index-1,
                                                      &pNode->DeviceInterfaceData);
                if (!success)
                {
                    FreeDeviceInfoNode(&pNode);
                    break;
                }
             
                success = SetupDiGetDeviceInterfaceDetail(DeviceList->DeviceInfo,
                                                          &pNode->DeviceInterfaceData,
                                                          NULL,
                                                          0,
                                                          &requiredLength,
                                                          NULL);
        
                error = GetLastError();
                
                if (!success && error != ERROR_INSUFFICIENT_BUFFER)
                {
                    FreeDeviceInfoNode(&pNode);
                    break;
                }
                
                pNode->DeviceDetailData = (PSP_DEVICE_INTERFACE_DETAIL_DATA)ALLOC(requiredLength);

                if (pNode->DeviceDetailData == NULL)
                {
                    FreeDeviceInfoNode(&pNode);
                    break;
                }
                
                pNode->DeviceDetailData->cbSize = sizeof(SP_DEVICE_INTERFACE_DETAIL_DATA);
                
                success = SetupDiGetDeviceInterfaceDetail(DeviceList->DeviceInfo,
                                                          &pNode->DeviceInterfaceData,
                                                          pNode->DeviceDetailData,
                                                          requiredLength,
                                                          &requiredLength,
                                                          NULL);
                if (!success)
                {
                    FreeDeviceInfoNode(&pNode);
                    break;
                }        
        
                InsertTailList(&DeviceList->ListHead, &pNode->ListEntry);
            }
        }
    }
}

void
ClearDeviceList(
	PDEVICE_GUID_LIST DeviceList
)
{
	if (DeviceList->DeviceInfo != INVALID_HANDLE_VALUE)
	{
		SetupDiDestroyDeviceInfoList(DeviceList->DeviceInfo);
		DeviceList->DeviceInfo = INVALID_HANDLE_VALUE;
	}

	while (!IsListEmpty(&DeviceList->ListHead))
	{
		PDEVICE_INFO_NODE pNode = NULL;
		PLIST_ENTRY pEntry;

		pEntry = RemoveHeadList(&DeviceList->ListHead);

		pNode = CONTAINING_RECORD(pEntry,
			DEVICE_INFO_NODE,
			ListEntry);

		FreeDeviceInfoNode(&pNode);
	}
}

VOID
FreeDeviceInfoNode(
	_In_ PDEVICE_INFO_NODE *ppNode
)
{
	if (ppNode == NULL)
	{
		return;
	}

	if (*ppNode == NULL)
	{
		return;
	}

	if ((*ppNode)->DeviceDetailData != NULL)
	{
		FREE((*ppNode)->DeviceDetailData);
	}

	if ((*ppNode)->DeviceDescName != NULL)
	{
		FREE((*ppNode)->DeviceDescName);
	}

	if ((*ppNode)->DeviceDriverName != NULL)
	{
		FREE((*ppNode)->DeviceDriverName);
	}

	FREE(*ppNode);
	*ppNode = NULL;
}

PDEVICE_INFO_NODE
FindMatchingDeviceNodeForDriverName(
	_In_ PSTR   DriverKeyName,
	_In_ BOOLEAN IsHub
)
{
	PDEVICE_INFO_NODE pNode = NULL;
	PDEVICE_GUID_LIST pList = NULL;
	PLIST_ENTRY       pEntry = NULL;

	pList = IsHub ? &gHubList : &gDeviceList;

	pEntry = pList->ListHead.Flink;

	while (pEntry != &pList->ListHead)
	{
		pNode = CONTAINING_RECORD(pEntry,
			DEVICE_INFO_NODE,
			ListEntry);
		if (_stricmp(DriverKeyName, pNode->DeviceDriverName) == 0)
		{
			return pNode;
		}

		pEntry = pEntry->Flink;
	}

	return NULL;
}

