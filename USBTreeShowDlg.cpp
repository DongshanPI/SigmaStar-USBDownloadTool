// USBTreeShowDlg.cpp : implementation file
//

#include "stdafx.h"
#include "USBDownloadTool.h"
#include "USBTreeShowDlg.h"
#include "afxdialogex.h"
#include "enum.h"


// USBTreeShowDlg dialog

IMPLEMENT_DYNAMIC(USBTreeShowDlg, CDialogEx)

USBTreeShowDlg::USBTreeShowDlg(CWnd* pParent /*=NULL*/)
	: CDialogEx(IDD_USBTREESHOW, pParent)
{
	//log("%s start\n", __func__);
	Create(IDD_USBTREESHOW, pParent);
	ShowWindow(SW_SHOW);
	SetWindowTextA("Choose Device");
	DEV_BROADCAST_DEVICEINTERFACE   broadcastInterface;

	broadcastInterface.dbcc_size = sizeof(DEV_BROADCAST_DEVICEINTERFACE);
	broadcastInterface.dbcc_devicetype = DBT_DEVTYP_DEVICEINTERFACE;

	memcpy(&(broadcastInterface.dbcc_classguid),
		&(GUID_DEVINTERFACE_USB_DEVICE),
		sizeof(struct _GUID));

	m_NotifyDevHandle = RegisterDeviceNotification(this->GetSafeHwnd(),
		&broadcastInterface,
		DEVICE_NOTIFY_WINDOW_HANDLE);

	// Now register for Hub notifications.
	memcpy(&(broadcastInterface.dbcc_classguid),
		&(GUID_CLASS_USBHUB),
		sizeof(struct _GUID));

	m_NotifyHubHandle = RegisterDeviceNotification(this->GetSafeHwnd(),
		&broadcastInterface,
		DEVICE_NOTIFY_WINDOW_HANDLE);

	RefreshTree();
}

USBTreeShowDlg::~USBTreeShowDlg()
{
	//log("%s exit\n", __func__);
	if (m_NotifyDevHandle)
		UnregisterDeviceNotification(m_NotifyDevHandle);
	if (m_NotifyHubHandle)
		UnregisterDeviceNotification(m_NotifyHubHandle);

	DestroyTree();
	ClearDeviceList(&gDeviceList);
	ClearDeviceList(&gHubList);
}

void USBTreeShowDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_TREE1, m_usbTree);
}


BEGIN_MESSAGE_MAP(USBTreeShowDlg, CDialogEx)
	ON_NOTIFY(TVN_SELCHANGED, IDC_TREE1, &USBTreeShowDlg::OnTvnSelchangedTree1)
	ON_NOTIFY(TVN_ITEMCHANGED, IDC_TREE1, &USBTreeShowDlg::OnTvnItemChangedTree1)
	ON_BN_CLICKED(IDOK, &USBTreeShowDlg::OnBnClickedOk)
	ON_BN_CLICKED(IDCANCEL, &USBTreeShowDlg::OnBnClickedCancel)
	ON_WM_DEVICECHANGE()
END_MESSAGE_MAP()

void USBTreeShowDlg::ExpandTree(HTREEITEM hTreeItem)
{
	if (!m_usbTree.ItemHasChildren(hTreeItem))
	{
		return;
	}
	HTREEITEM hNextItem = m_usbTree.GetChildItem(hTreeItem);
	while (hNextItem != NULL)
	{
		ExpandTree(hNextItem);
		hNextItem = m_usbTree.GetNextSiblingItem(hNextItem);
	}
	m_usbTree.Expand(hTreeItem, TVE_EXPAND);
}

void USBTreeShowDlg::DestroyTree(void)
{
	// Clear the selection of the TreeView, so that when the tree is
	// destroyed, the control won't try to constantly "shift" the
	// selection to another item.
	//
	//m_usbTree.SelectItem(NULL);

	// Destroy the current contents of the TreeView
	//
	if (root)
	{
		m_usbTree.DeleteAllItems();

		root = NULL;
	}

	//ClearDeviceList(&gDeviceList);
	//ClearDeviceList(&gHubList);
}

/*Destroy the tree and then rebuild the tree.*/
void USBTreeShowDlg::RefreshTree(void)
{
	TV_INSERTSTRUCT tvins;
	
	DestroyTree();

	// Set the parent item
	//
	tvins.hParent = TVI_ROOT;
	// pszText and lParam members are valid
	//
	tvins.item.mask = TVIF_TEXT | TVIF_PARAM;

	// Set the text of the item.
	//
	tvins.item.pszText = "Device Tree";

	// Set the user context item
	//
	tvins.item.lParam = NULL;

	// Add the item to the tree-view control.
	//
	root = m_usbTree.InsertItem(&tvins);
	EnumerateHostControllers(this);
	ExpandTree(root);
}

void USBTreeShowDlg::OnTvnSelchangedTree1(NMHDR *pNMHDR, LRESULT *pResult)
{
	LPNMTREEVIEW pNMTreeView = reinterpret_cast<LPNMTREEVIEW>(pNMHDR);
	// TODO: Add your control notification handler code here
	*pResult = 0;
	HTREEITEM ht = m_usbTree.GetSelectedItem();
	TV_ITEM tvi;
	CHAR    tviName[512];
	PDEVICE_INFO_NODE   pNode = NULL;
	//
	// Get the name of the TreeView item, along with the a pointer to the
	// info we stored about the item in the item's lParam.
	//
	tvi.mask = TVIF_HANDLE | TVIF_TEXT | TVIF_PARAM;
	tvi.hItem = ht;
	tvi.pszText = (LPSTR)tviName;
	tvi.cchTextMax = sizeof(tviName);
	m_usbTree.GetItem(&tvi);
	log("Choose device: %s\n", tvi.pszText);
	pNode = (PDEVICE_INFO_NODE)tvi.lParam;
	if (pNode != NULL)
	{
		log("\tKey: %s\n", pNode->DeviceDriverName);
		if (NULL == gSelectDevice){
			gSelectDevice = (PDEVICE_INFO_NODE)ALLOC(sizeof(DEVICE_INFO_NODE));
			if (!gSelectDevice)
			{
				log("\tAlloc memory for selected device failed\n");
				return;
			}
		}
		memcpy(gSelectDevice, pNode, sizeof(DEVICE_INFO_NODE));
		gSelectDevice->DeviceDriverName = (PSTR)ALLOC(strlen(pNode->DeviceDriverName) + 1);
		if (!gSelectDevice->DeviceDriverName)
		{
			log("\tAlloc memory for selected device driver name failed\n");
			FREE(gSelectDevice);
			return;
		}
		memcpy(gSelectDevice->DeviceDriverName, pNode->DeviceDriverName, strlen(pNode->DeviceDriverName));
	}
	else
		log("\tDevice information is null\n");
}

BOOL USBTreeShowDlg::OnDeviceChange(UINT nEventType, DWORD_PTR dwData)
{
	DEV_BROADCAST_DEVICEINTERFACE* dbd = (DEV_BROADCAST_DEVICEINTERFACE*)dwData;

	switch (nEventType)
	{
	case DBT_DEVICEARRIVAL:
	case DBT_DEVICEREMOVECOMPLETE:
		RefreshTree();
		break;
	}

	return TRUE;
}

void USBTreeShowDlg::OnTvnItemChangedTree1(NMHDR *pNMHDR, LRESULT *pResult)
{
	NMTVITEMCHANGE *pNMTVItemChange = reinterpret_cast<NMTVITEMCHANGE*>(pNMHDR);
	// TODO: Add your control notification handler code here
	*pResult = 0;
}

void USBTreeShowDlg::OnBnClickedOk()
{
	// TODO: Add your control notification handler code here
	CDialogEx::OnOK();

	if (gSelectDevice && gSelectDevice->DeviceDriverName) {
		//log("\tSelected:%s strlen=%d\n", gSelectDevice->DeviceDriverName, strlen(gSelectDevice->DeviceDriverName));
		RefreshTree();
		GetParent()->SendMessage(WM_CHILD_MSG, 0, flag);
		flag = FALSE;
	}
	else
		GetParent()->SendMessage(WM_CHILD_MSG, 0, FALSE);
}

void USBTreeShowDlg::OnBnClickedCancel()
{
	// TODO: Add your control notification handler code here
	CDialogEx::OnCancel();
	GetParent()->SendMessage(WM_CHILD_MSG, 0, FALSE);
	if (gSelectDevice)
	{
		if (gSelectDevice->DeviceDriverName) {
			FREE(gSelectDevice->DeviceDriverName);
			gSelectDevice->DeviceDriverName = NULL;
		}

		FREE(gSelectDevice);
		gSelectDevice = NULL;
	}
}
