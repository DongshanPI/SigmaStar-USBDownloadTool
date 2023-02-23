#pragma once

#define WM_CHILD_MSG (WM_USER + 2)
// USBTreeShowDlg dialog

class USBTreeShowDlg : public CDialogEx
{
	DECLARE_DYNAMIC(USBTreeShowDlg)

public:
	USBTreeShowDlg(CWnd* pParent = NULL);   // standard constructor
	virtual ~USBTreeShowDlg();

// Dialog Data
#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_USBTREESHOW };
#endif

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

	DECLARE_MESSAGE_MAP()
public:
	CTreeCtrl    m_usbTree;
	HTREEITEM    root;
	BOOL         flag = FALSE;
	HDEVNOTIFY   m_NotifyDevHandle = NULL;
	HDEVNOTIFY   m_NotifyHubHandle = NULL;
	
	void ExpandTree(HTREEITEM hTreeItem);
	void DestroyTree(void);
	void RefreshTree(void);
	afx_msg void OnTvnSelchangedTree1(NMHDR *pNMHDR, LRESULT *pResult);
	afx_msg void OnTvnItemChangedTree1(NMHDR *pNMHDR, LRESULT *pResult);
	afx_msg void OnBnClickedOk();
	afx_msg void OnBnClickedCancel();
	BOOL OnDeviceChange(UINT nEventType, DWORD_PTR dwData);
};
