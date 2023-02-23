
// USBDownloadToolDlg.h : 头文件
//

#pragma once
#include <list>


// CUSBDownloadToolDlg 对话框
class CUSBDownloadToolDlg : public CDialogEx
{
// 构造
public:
	typedef std::list<CString> CNameList;
	CUSBDownloadToolDlg(CWnd* pParent = NULL);	// 标准构造函数

// 对话框数据
	enum { IDD = IDD_USBDOWNLOADTOOL_DIALOG };

	protected:
	virtual void DoDataExchange(CDataExchange* pDX);	// DDX/DDV 支持
// 实现
protected:
	HICON m_hIcon;

	// 生成的消息映射函数
	virtual BOOL OnInitDialog();
	afx_msg LRESULT OnChildMessage(WPARAM wParam, LPARAM lParam);
	afx_msg void OnSysCommand(UINT nID, LPARAM lParam);
	afx_msg void OnPaint();
	afx_msg HCURSOR OnQueryDragIcon();
	DECLARE_MESSAGE_MAP()
public:
#ifndef _UNICODE
	CStringW char2CStringW(char *pstr);
#endif
	afx_msg BOOL OnDeviceChange(UINT nEventType, DWORD_PTR dwData);
	afx_msg HBRUSH OnCtlColor(CDC* pDC, CWnd* pWnd, UINT nCtlColor);
	afx_msg void OnBnClickedButton2();
	afx_msg void Load_Updater();
	afx_msg void Load_Uboot();
	afx_msg void Auto_Update();	// Usb Firmware Update
	CEdit       m_logEdit;
	void logTrace(CString str);
	void GetVideoDeviceList(CNameList &name_list);
	void WindowSwitch(bool on);
	int dev_state;
	CNameList NameList;
	BYTE DataBuf[64];
	GUID CurSelGUID;

	CBrush      m_bkBrush;
	CFont       m_font;
	char showFlag = 0;
	enum {
		SHOW_STATUS_PLUG_IN = 0,
		SHOW_STATUS_PLUG_OUT,
		SHOW_STATUS_SUCCESS,
		SHOW_STATUS_FAIL,
	};

    //Zk add thread.
    HANDLE HGetDebLogThread;					// to get debug log thread handle.
    static DWORD WINAPI AutoUpdateThreadEntry(LPVOID param);
    void CloseGetDebugThread()
    {
        OutputDebugString(TEXT("Enter CloseGetDebugThread()!\n"));
        WaitForSingleObject(HGetDebLogThread, INFINITE);
        CloseHandle(HGetDebLogThread);
        HGetDebLogThread = NULL;
    };
	afx_msg void OnDrawItem(int nIDCtl, LPDRAWITEMSTRUCT lpDrawItemStruct);
};
