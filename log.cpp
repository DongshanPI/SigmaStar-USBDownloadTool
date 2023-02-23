#include "stdafx.h"

#define MAX_LOG_LEN  4096
/*class CWnd *pWnd = NULL;

void log(char *format, ...)
{
	CString strTime;
	CString strTemp;
	CTime curTime = CTime::GetCurrentTime();
	va_list arguments;
	char sprint_buf[MAX_LOG_LEN];
	memset(sprint_buf,0x00,MAX_LOG_LEN);

	va_start(arguments, format);
    vsnprintf(sprint_buf, MAX_LOG_LEN, format, arguments);
    va_end(arguments);

	CString str = CString(sprint_buf);
	strTime = curTime.Format(_T("%Y-%m-%d %H:%M:%S "));
	if (pWnd){
		if (pWnd->IsWindowEnabled())
		{
			strTemp = _T("");
			pWnd->GetWindowText(strTemp);
			strTemp += _T("\r\n");
			strTemp += strTime;
			strTemp += str;
			pWnd->SetWindowText(strTemp);
			//pWnd->LineScroll(pWnd->GetLineCount());
		}
	}
}*/

class CEdit *pEdit = NULL;
void log(char *format, ...)
{
	CString strTime;
	CString strTemp;
	CTime curTime = CTime::GetCurrentTime();
	va_list arguments;
	char sprint_buf[MAX_LOG_LEN];
	memset(sprint_buf,0x00,MAX_LOG_LEN);

	va_start(arguments, format);
    vsnprintf(sprint_buf, MAX_LOG_LEN, format, arguments);
    va_end(arguments);

	CString str = CString(sprint_buf);
	//strTime = curTime.Format(_T("%Y-%m-%d %H:%M:%S "));
	strTime = curTime.Format(_T("%H:%M:%S "));
	if (pEdit){
		if (pEdit->IsWindowEnabled())
		{
			strTemp = _T("");
			pEdit->GetWindowText(strTemp);
			strTemp += strTime;
			strTemp += str;
			strTemp += _T("\r\n");
			pEdit->SetWindowText(strTemp);
			pEdit->LineScroll(pEdit->GetLineCount());
		}
	}
}
