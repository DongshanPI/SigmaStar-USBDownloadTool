
// USBDownloadToolDlg.cpp : 实现文件
//

#include "stdafx.h"
#include "USBDownloadTool.h"
#include "USBDownloadToolDlg.h"
#include "USBTreeShowDlg.h"
#include "afxdialogex.h"
#include "burnImage.h"
#include "usbscsicmd.h"
#include "AitApi.h"
#include "enum.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif
static const GUID g_GUID_DEVINTERFACE_LIST[] =
{
	//GUID_DEVINTERFACE_USB_DEVICE, {A5DCBF10-6530-11D2-901F-00C04FB951ED}
	{0xA5DCBF10, 0x6530, 0x11D2, {0x90, 0x1F, 0x00, 0xC0, 0x4F, 0xB9, 0x51, 0xED}},

	//GUID_DEVINTERFACE_DISK, {53F56307-B6BF-11D0-94F2-00A0C91EFB8B}
	{0x53F56307, 0xB6BF, 0x11D0, {0x94, 0xF2, 0x00, 0xA0, 0xC9, 0x1E, 0xFB, 0x8B}},

	//GUID_DEVINTERFACE_HID, {4D1E55B2-F16F-11CF-88CB-001111000030}
	{0x4D1E55B2, 0xF16F, 0x11CF, {0x88, 0xCB, 0x00, 0x11, 0x11, 0x00, 0x00, 0x30}}
};

static const GUID g_GUIDS[] = {
	{ 0x00000000, 0x0000, 0x0000,{ 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 ,0x00 ,0x00 } },
	{ 0x23E49ED0, 0x1178, 0x4F31,{ 0xAE, 0x52, 0xD2, 0xFB, 0x8A, 0x8D ,0x3B ,0x48 } }, //AIT XU1
	{ 0x2C49D16A, 0x32B8, 0x4485,{ 0x3E, 0xA8, 0x64, 0x3A, 0x15, 0x23 ,0x62 ,0xF2 } },	//AIT XU2
	{ 0xA29E7641, 0xDE04, 0x47E3,{ 0x8B, 0x2B, 0xF4, 0x34, 0x1A, 0xFF ,0x00 ,0x3B } },	//AIT XU3
	{ 0xB42153BD, 0x35D6, 0x45CA,{ 0xB2, 0x03, 0x4E, 0x01, 0x49, 0xB3, 0x01, 0xBC } },
	{ 0xF679EF5B, 0x54DF, 0x4bfc,{ 0xB8, 0xCB, 0xCA, 0x7E, 0x89, 0xA5, 0x23, 0x3F } },
	{ 0xA29E7641, 0xDE04, 0x47e3,{ 0x8B,	0x2B, 0xF4, 0x34, 0x1A, 0xFF, 0x00, 0x3B } },
	//8280f5c1-dbe3-43d4-9778-3870432581c0 
	{ 0x8280f5c1, 0xdbe3, 0x43d4,{ 0x97, 0x78, 0x38, 0x70, 0x43, 0x25, 0x81, 0xC0 } },		//Add.
};
// 用于应用程序“关于”菜单项的 CAboutDlg 对话框

class USBTreeShowDlg  *pChildDlg;

class CAboutDlg : public CDialogEx
{
public:
	CAboutDlg();

// 对话框数据
	enum { IDD = IDD_ABOUTBOX };

// 实现
protected:
	DECLARE_MESSAGE_MAP()
};

CAboutDlg::CAboutDlg() : CDialogEx(CAboutDlg::IDD)
{
}

BEGIN_MESSAGE_MAP(CAboutDlg, CDialogEx)
END_MESSAGE_MAP()


// CUSBDownloadToolDlg 对话框




CUSBDownloadToolDlg::CUSBDownloadToolDlg(CWnd* pParent /*=NULL*/)
	: CDialogEx(CUSBDownloadToolDlg::IDD, pParent)
{
	m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
}

void CUSBDownloadToolDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_EDIT1, m_logEdit);
}

BEGIN_MESSAGE_MAP(CUSBDownloadToolDlg, CDialogEx)
	ON_WM_SYSCOMMAND()
	ON_WM_PAINT()
	ON_WM_QUERYDRAGICON()
	ON_BN_CLICKED(IDC_BUTTON2, &CUSBDownloadToolDlg::OnBnClickedButton2)
	ON_MESSAGE(WM_CHILD_MSG, OnChildMessage)
	ON_WM_DEVICECHANGE()
	ON_WM_CTLCOLOR()
	ON_WM_DRAWITEM()
END_MESSAGE_MAP()


// CUSBDownloadToolDlg 消息处理程序

BOOL CUSBDownloadToolDlg::OnInitDialog()
{	
/*#ifdef _DEBUG
	AllocConsole();
#endif*/
	CDialogEx::OnInitDialog();

	// 将“关于...”菜单项添加到系统菜单中。

	// IDM_ABOUTBOX 必须在系统命令范围内。
	ASSERT((IDM_ABOUTBOX & 0xFFF0) == IDM_ABOUTBOX);
	ASSERT(IDM_ABOUTBOX < 0xF000);

	CMenu* pSysMenu = GetSystemMenu(FALSE);
	if (pSysMenu != NULL)
	{
		BOOL bNameValid;
		CString strAboutMenu;
		bNameValid = strAboutMenu.LoadString(IDS_ABOUTBOX);
		ASSERT(bNameValid);
		if (!strAboutMenu.IsEmpty())
		{
			pSysMenu->AppendMenu(MF_SEPARATOR);
			pSysMenu->AppendMenu(MF_STRING, IDM_ABOUTBOX, strAboutMenu);
		}
	}

	// 设置此对话框的图标。当应用程序主窗口不是对话框时，框架将自动
	//  执行此操作
	SetIcon(m_hIcon, TRUE);			// 设置大图标
	SetIcon(m_hIcon, FALSE);		// 设置小图标
	
    HGetDebLogThread = NULL;
	dev_state = DEV_STATE_ROM;
	//GetVideoDeviceList(NameList);
	/*获取日志窗口指针*/
	pEdit = (CEdit *)GetDlgItem(IDC_EDIT1);
	EnumListInit();
	//EnumerateHostControllers();
	m_font.CreatePointFont(120, "黑体");
	m_bkBrush.CreateSolidBrush(GetSysColor(COLOR_3DFACE));
	GetDlgItem(IDC_STATIC1)->SetWindowText(_T("DisConnected"));
	WindowSwitch(FALSE);

	HDEVNOTIFY	hDevNotify;
	DEV_BROADCAST_DEVICEINTERFACE NotificationFilter;
	ZeroMemory(&NotificationFilter, sizeof(DEV_BROADCAST_DEVICEINTERFACE));
	NotificationFilter.dbcc_size = sizeof(DEV_BROADCAST_DEVICEINTERFACE);
	NotificationFilter.dbcc_devicetype = DBT_DEVTYP_DEVICEINTERFACE;
	for (int i = 0; i < sizeof(g_GUID_DEVINTERFACE_LIST) / sizeof(GUID); i++)
	{
		NotificationFilter.dbcc_classguid = g_GUID_DEVINTERFACE_LIST[i];
		hDevNotify = RegisterDeviceNotification(this->GetSafeHwnd(), &NotificationFilter, DEVICE_NOTIFY_WINDOW_HANDLE);
		if (!hDevNotify)
		{
			CString msg;
			msg.Format(_T("Can't register device notification"));
			logTrace(msg);
			return FALSE;
		}
	}
	return TRUE;  // 除非将焦点设置到控件，否则返回 TRUE
}

void CUSBDownloadToolDlg::OnSysCommand(UINT nID, LPARAM lParam)
{
	if ((nID & 0xFFF0) == IDM_ABOUTBOX)
	{
		CAboutDlg dlgAbout;
		dlgAbout.DoModal();
	}
	else
	{
		CDialogEx::OnSysCommand(nID, lParam);
	}
}

// 如果向对话框添加最小化按钮，则需要下面的代码
//  来绘制该图标。对于使用文档/视图模型的 MFC 应用程序，
//  这将由框架自动完成。

void CUSBDownloadToolDlg::OnPaint()
{
	if (IsIconic())
	{
		CPaintDC dc(this); // 用于绘制的设备上下文

		SendMessage(WM_ICONERASEBKGND, reinterpret_cast<WPARAM>(dc.GetSafeHdc()), 0);

		// 使图标在工作区矩形中居中
		int cxIcon = GetSystemMetrics(SM_CXICON);
		int cyIcon = GetSystemMetrics(SM_CYICON);
		CRect rect;
		GetClientRect(&rect);
		int x = (rect.Width() - cxIcon + 1) / 2;
		int y = (rect.Height() - cyIcon + 1) / 2;

		// 绘制图标
		dc.DrawIcon(x, y, m_hIcon);
	}
	else
	{
		CDialogEx::OnPaint();
	}
}

//当用户拖动最小化窗口时系统调用此函数取得光标
//显示。
HCURSOR CUSBDownloadToolDlg::OnQueryDragIcon()
{
	return static_cast<HCURSOR>(m_hIcon);
}

#ifndef _UNICODE
CStringW CUSBDownloadToolDlg::char2CStringW(char *pstr)
{
	int dBufSize = MultiByteToWideChar(CP_UTF8, 0, pstr, -1, NULL, 0);
	WCHAR *pBuf = new WCHAR[dBufSize + 1];
	memset(pBuf, 0, dBufSize + 1);
	MultiByteToWideChar(CP_UTF8, 0, pstr, -1, pBuf, dBufSize);
	pBuf[dBufSize] = '\0';
	CStringW tmpstrW = pBuf;
	delete pBuf;
	return tmpstrW;
}
#endif // !_UNICODE

void CUSBDownloadToolDlg::Load_Updater()
{
	if (loadUpdater("usb_updater.bin") == 0)
		dev_state = DEV_STATE_UPDATER;
}

void CUSBDownloadToolDlg::Load_Uboot()
{
	CString errInfo;
	int tryNum = 0;
	logTrace(_T("Wait for downloading......"));
	if (loadUboot("u-boot.bin") == 0) {
		//wait u-boot enumeration done
		logTrace(_T("Wait for bringup......"));
		Sleep(1000);
		while (1)
		{
			Sleep(100);
			if (USB_ScsiFindDevice(&dev_state) != NULL)
			{
				break;
			}
			if (tryNum++ > 5)
			{
				logTrace(_T("U-boot error: can not detect msdc"));
				break;
			}
		}
		if (dev_state != DEV_STATE_UBOOT) {
			errInfo.Format(_T("Invalid device state %d."), dev_state);
			logTrace(errInfo);
		}
	}
}

void CUSBDownloadToolDlg::Auto_Update()
{
	RunScript("auto_update.txt");
}

DWORD WINAPI CUSBDownloadToolDlg::AutoUpdateThreadEntry(LPVOID param)
{
    CUSBDownloadToolDlg  *Cthis = (CUSBDownloadToolDlg*) param;
	/*Clear ota upgrade status first*/
	USB_ScsiRunCmd("setenv ota_upgrade_status 0", strlen("setenv ota_upgrade_status 0"));
	Cthis->Auto_Update();
    Cthis->MessageBox(_T("Upgrade done, reboot now."));

	return 0;
}

void CUSBDownloadToolDlg::OnBnClickedButton2()
{
	// TODO: 在此添加控件通知处理程序代码
#if 0  //ZK.
    HGetDebLogThread = CreateThread(NULL, 0, F_GetDebugThreadEntry, this, 0, NULL);
#else
	HANDLE hd = NULL;
	hd = USB_ScsiFindDevice(&dev_state);

	if (hd != NULL) {
		if (dev_state == DEV_STATE_ROM)
			Load_Updater();
		AfxGetApp()->PumpMessage();

		if (dev_state == DEV_STATE_UPDATER)
			Load_Uboot();
		//AfxGetApp()->PumpMessage();

		if (dev_state == DEV_STATE_UBOOT) {
			CreateThread(NULL, 0, AutoUpdateThreadEntry, this, 0, NULL);
		}
		AfxGetApp()->PumpMessage();
	} else {
		AitDeviceHandle uvc_handle = NULL;
		CString DevPath;
		HRESULT hr = 0;
		ULONG BytesRet = 0;
		CNameList::iterator i;
		int n = 0;
		int tryNum = 0;
		//CString info;
		logTrace(_T("No accessible device, try to find uvc camera"));
		GetVideoDeviceList(NameList);
		if (NameList.empty())
		{
			logTrace(_T("Can not find uvc camera"));
			pChildDlg = new USBTreeShowDlg(this);
			return;
		}
		for (i = NameList.begin(); i != NameList.end(); ++i)
		{
			//info.Format(_T("Device: %s\n"), *i);
			//MessageBox(info);
			DevPath.Format(_T("%s"), *i);
			if (IdentifyDevice(DevPath)) {
#ifndef _UNICODE
				int size = MultiByteToWideChar(CP_UTF8, 0, DevPath, -1, NULL, 0);
				PWSTR pDevpath = new WCHAR[size + 1];
				memset(pDevpath, 0, size + 1);
				MultiByteToWideChar(CP_UTF8, 0, DevPath, -1, pDevpath, size);
				pDevpath[size] = '\0';
				AITAPI_OpenDeviceByPath((TCHAR *)pDevpath, &uvc_handle);
				delete pDevpath;
#else
				AITAPI_OpenDeviceByPath(DevPath, &uvc_handle);
#endif // !_UNICODE
				logTrace(_T("Identify Device: ok"));
			}//modified by TX
			else continue;

			if (uvc_handle == 0)
			{
				//info.Format(TEXT("Failed to open device: %s.\n"), DevPath);
				logTrace(_T("Failed to open device: ") + DevPath);
				//return;
				continue;//modified by TX
			}

			DataBuf[0] = 0;
			for (n = 0; n < sizeof(g_GUIDS) / sizeof(g_GUIDS[0]); n++)
			{
				/*control select: 4  data len: 8*/
				CurSelGUID = g_GUIDS[n];
				hr = AITAPI_UvcExtSet(uvc_handle, &CurSelGUID, 4, DataBuf, 8, &BytesRet);
				if (SUCCEEDED(hr)) {
					// after sending XU for OTA, wait device to enter ufu mode
					logTrace(_T("Waiting 1 min for rebooting......"));
					while (1)
					{
						Sleep(1000);
						if (USB_ScsiFindDevice(&dev_state) != NULL)
						{
							break;
						}

						if (tryNum++ > 60) {
							logTrace(_T("Error:Can not dectect uboot running"));
							return;
						}
					}
					if (dev_state == DEV_STATE_UBOOT) {
						CreateThread(NULL, 0, AutoUpdateThreadEntry, this, 0, NULL);
						break;//modified by TX
					}
					if (dev_state != DEV_STATE_UBOOT) {
						//logTrace(_T("Continue because of Invalid Device"));
						continue;
					}//modified by TX
					//break;  modified by TX
				}
			}
			if (SUCCEEDED(hr) && (dev_state == DEV_STATE_UBOOT))
				break;//modified by TX
			if (FAILED(hr))
				logTrace(_T("XU Command failed."));
		}//modified by TX

		if (i == NameList.end())
		{
			logTrace(_T("Can not find uvc camera"));
			pChildDlg = new USBTreeShowDlg(this);
			return;
		}
	}
	/*else{
		fprintf(stderr, "Can not find GC device.\n");
		//::MessageBox(NULL, _T("Please ensure device pluged in!"), _T("Warnnig"), MB_ICONERROR|MB_OK);
		MessageBox(_T("Please ensure device pluged in!"), _T("Warning"), MB_ICONERROR|MB_OK);
	}*/
#endif
	return;
}

void CUSBDownloadToolDlg::GetVideoDeviceList(CNameList &name_list)
{
	HRESULT hr;
	IBaseFilter * pSrc = NULL;
	CComPtr <IMoniker> pMoniker = NULL;
	ULONG cFetched;

	// Create the system device enumerator
	CComPtr <ICreateDevEnum> pDevEnum = NULL;
	CoInitialize(NULL);
	hr = CoCreateInstance(CLSID_SystemDeviceEnum, NULL, CLSCTX_INPROC,
		IID_ICreateDevEnum, (void **)&pDevEnum);
	if (FAILED(hr))
	{
		return;
	}

	// Create an enumerator for the video capture devices
	CComPtr <IEnumMoniker> pClassEnum = NULL;

	hr = pDevEnum->CreateClassEnumerator(CLSID_VideoInputDeviceCategory, &pClassEnum, 0);
	if (FAILED(hr))
	{
		return;
	}

	// If there are no enumerators for the requested type, then 
	// CreateClassEnumerator will succeed, but pClassEnum will be NULL.
	if (pClassEnum == NULL)
	{
		return;
	}

	// Use the first video capture device on the device list.
	// Note that if the Next() call succeeds but there are no monikers,
	// it will return S_FALSE (which is not a failure).  Therefore, we
	// check that the return code is S_OK instead of using SUCCEEDED() macro.
	while (S_OK == (pClassEnum->Next(1, &pMoniker, &cFetched)))
	{
		IMalloc *iMalloc;
		LPOLESTR device_name;
		hr = pMoniker->GetDisplayName(0, 0, &device_name);
		CString str(device_name);
		name_list.push_back(str);
		CoGetMalloc(1, &iMalloc);
		iMalloc->Free((void*)device_name);
		iMalloc->Release();
		pMoniker.Release();
	}
}

void CUSBDownloadToolDlg::logTrace(CString str)
{
	CString strTime;
	CString strTemp;
	CTime curTime = CTime::GetCurrentTime();
	//strTime = curTime.Format(_T("%Y-%m-%d %H:%M:%S "));
	strTime = curTime.Format(_T("%H:%M:%S "));

	if (m_logEdit.IsWindowEnabled())
	{
		int nLength;

		strTemp = _T("");
		this->UpdateData(TRUE);
		nLength = m_logEdit.GetWindowTextLength();
		strTemp += strTime;
		strTemp += str;
		strTemp += _T("\r\n");
		// put the selection at the end of text
		m_logEdit.SetSel(nLength, nLength);
		m_logEdit.ReplaceSel(strTemp);
		m_logEdit.LineScroll(m_logEdit.GetLineCount());
		this->UpdateData(FALSE);
	}
}

afx_msg LRESULT CUSBDownloadToolDlg::OnChildMessage(WPARAM wParam, LPARAM lParam)
{
	int tryNum = 0;
	log("Get child message: %d\n", lParam);
	if ((BOOL)lParam)
	{
		logTrace(_T("Waiting for upgrading......"));

		while (1)
		{
			Sleep(500);
			if (USB_ScsiFindDevice(&dev_state) != NULL)
			{
				break;
			}

			if (tryNum++ > 5) {
				logTrace(_T("Error:Can not dectect uboot running"));
				break;
			}
		}
		if (dev_state == DEV_STATE_UBOOT) {
			CreateThread(NULL, 0, AutoUpdateThreadEntry, this, 0, NULL);
		}
	}
	delete pChildDlg;
	return 0;
}


void CUSBDownloadToolDlg::WindowSwitch(bool on)
{
	if (on) {
		//GetDlgItem(IDC_STATIC1)->SetWindowText(_T("Connected"));
		showFlag = SHOW_STATUS_PLUG_IN;
	}
	else {
		//GetDlgItem(IDC_STATIC1)->SetWindowText(_T("DisConnected"));
		showFlag = SHOW_STATUS_PLUG_OUT;
	}
	GetDlgItem(IDC_STATIC1)->Invalidate();
	GetDlgItem(IDC_BUTTON2)->Invalidate();
}

BOOL CUSBDownloadToolDlg::OnDeviceChange(UINT nEventType, DWORD_PTR dwData)
{
	CString info;
	DEV_BROADCAST_DEVICEINTERFACE* dbd = (DEV_BROADCAST_DEVICEINTERFACE*)dwData;
	
	//logTrace(dbd->dbcc_name);
	//info.Format(_T("Device Change:0x%04x"), nEventType);
	//logTrace(info);
	switch (nEventType)
	{
	case DBT_DEVICEARRIVAL:
		if (char2CStringW(dbd->dbcc_name).Find(L"VID_1B20") >= 0)
		{
			GetDlgItem(IDC_STATIC1)->SetWindowText(_T("MSDC Connected"));
			WindowSwitch(TRUE);
			info.Format(_T("Mass storage device pluged IN!"));
			logTrace(info);
		}
		else if ((char2CStringW(dbd->dbcc_name).Find(L"VID_1D6B") >= 0) ||
			(char2CStringW(dbd->dbcc_name).Find(L"VID_114D") >= 0))
		{
			GetDlgItem(IDC_STATIC1)->SetWindowText(_T("UVC Connected"));
			WindowSwitch(TRUE);
			info.Format(_T("UVC pluged IN!"));
			logTrace(info);
		}
		break;
	case DBT_DEVICEREMOVECOMPLETE:
		if (char2CStringW(dbd->dbcc_name).Find(L"VID_1B20") >= 0)
		{
			GetDlgItem(IDC_STATIC1)->SetWindowText(_T("MSDC DisConnected"));
			WindowSwitch(FALSE);
			info.Format(_T("Mass storage device pluged OUT!"));
			logTrace(info);
			if (dev_state != DEV_STATE_ROM) {
				dev_state = DEV_STATE_ROM;
			}
		}
		else if ((char2CStringW(dbd->dbcc_name).Find(L"VID_1D6B") >= 0) ||
			(char2CStringW(dbd->dbcc_name).Find(L"VID_114D") >= 0))
		{
			GetDlgItem(IDC_STATIC1)->SetWindowText(_T("UVC DisConnected"));
			WindowSwitch(FALSE);
			info.Format(_T("UVC pluged OUT!"));
			logTrace(info);
		}
		break;
	}
	return TRUE;
}

HBRUSH CUSBDownloadToolDlg::OnCtlColor(CDC* pDC, CWnd* pWnd, UINT nCtlColor)
{
	HBRUSH hbr = CDialogEx::OnCtlColor(pDC, pWnd, nCtlColor);

	// TODO:  在此更改 DC 的任何特性
	switch (pWnd->GetDlgCtrlID())
	{
	case IDC_STATIC1:
		if (showFlag == SHOW_STATUS_PLUG_IN) {
			pDC->SetTextColor(RGB(0, 255, 0));
			pDC->SelectObject(&m_font);
		}
		else if (showFlag == SHOW_STATUS_PLUG_OUT){
			pDC->SetTextColor(RGB(255, 0, 0));
			pDC->SelectObject(&m_font);
		}
		return m_bkBrush;
	}

	// TODO:  如果默认的不是所需画笔，则返回另一个画笔
	return hbr;
}


void CUSBDownloadToolDlg::OnDrawItem(int nIDCtl, LPDRAWITEMSTRUCT lpDrawItemStruct)
{
	// TODO: 在此添加消息处理程序代码和/或调用默认值
	HDC  dc = lpDrawItemStruct->hDC;
	CDC* pDC = CDC::FromHandle(dc);
	switch (nIDCtl) {
	case IDC_BUTTON2:
		CRect drawRect;
		CRect textRect;
		CBrush pBrush(RGB(200, 200, 200));
		CString btnCaption = _T("Upgrade Firmware");

		drawRect.CopyRect(&(lpDrawItemStruct->rcItem)); //拷贝控件矩形区域到我们的CRect对象
		textRect.CopyRect(&drawRect);
		pDC->DrawFrameControl(&drawRect, DFC_MENU, lpDrawItemStruct->CtlType);
		//pDC->DrawEdge(&drawRect, EDGE_ETCHED | BDR_SUNKENOUTER | BDR_RAISEDINNER, BF_RECT);
		//pDC->FrameRect(&drawRect, &pBrush);
		pDC->FillRect(drawRect, &pBrush);
		pDC->Draw3dRect(&drawRect, RGB(200,200,200), RGB(100, 100, 100));
		//pDC->SetTextColor(RGB(0, 0, 0));
		pDC->SetBkColor(RGB(200, 200, 200));
		textRect.top += (textRect.Height() - pDC->GetTextExtent(btnCaption).cy)/2;
		pDC->DrawText(btnCaption, &textRect, DT_CENTER);
		break;
	}
	CDialogEx::OnDrawItem(nIDCtl, lpDrawItemStruct);
}
