#include "stdafx.h"
#include "usbscsicmd.h"

static HANDLE m_DeviceHandle;
static int m_DeviceDisk = 0;

//------------------------------------------------------------------------------
//  Function    : SendMSTARScsiCmd
//  Parameter   : None
//  Return      : TRUE: success, FALSE: fail
//  Description :
//------------------------------------------------------------------------------
int ScsiCmdSend(BYTE Cmd,BYTE SubCommand, SCSI_BULK_DIRECTION bIn_Out,ULONG DataTransferLength,UCHAR* databuf)
{
	UINT status, k;
	SCSI_PASS_THROUGH_WITH_BUFFERS sptwb;
	ZeroMemory(&sptwb,sizeof(SCSI_PASS_THROUGH_WITH_BUFFERS));

	unsigned long length = 0, returned = 0;
	if (bIn_Out == SCSI_BULK_IN)	//bulk in
	{
		//SCSI_PASS_THROUGH_WITH_BUFFERS sptwb;
		sptwb.spt.Length = sizeof(SCSI_PASS_THROUGH);
		sptwb.spt.PathId = 0;
		sptwb.spt.TargetId = 0;
		sptwb.spt.Lun = 0;
		sptwb.spt.SenseInfoLength = 24;
		sptwb.spt.DataIn = SCSI_IOCTL_DATA_IN; 
		sptwb.spt.DataTransferLength = DataTransferLength;
		sptwb.spt.TimeOutValue = 120;    // [BC] change from 3 to 5 sec to see if fewer timeout issues happened
		sptwb.spt.DataBufferOffset = offsetof(SCSI_PASS_THROUGH_WITH_BUFFERS,ucDataBuf);
		sptwb.spt.SenseInfoOffset = offsetof(SCSI_PASS_THROUGH_WITH_BUFFERS,ucSenseBuf);
		sptwb.spt.Cdb[0] = Cmd;
		sptwb.spt.CdbLength = 10; 
		memcpy(sptwb.ucDataBuf, databuf, DataTransferLength);
		sptwb.spt.Cdb[1] = SubCommand;
		sptwb.spt.Cdb[6]  = (BYTE)((DataTransferLength) >> 24);
		sptwb.spt.Cdb[7]  = (BYTE)((DataTransferLength) >> 16);
		sptwb.spt.Cdb[8]  = (BYTE)((DataTransferLength) >>  8);
		sptwb.spt.Cdb[9]  = (BYTE)( DataTransferLength);
    	length = offsetof(SCSI_PASS_THROUGH_WITH_BUFFERS,ucDataBuf) +
		sptwb.spt.DataTransferLength;
		status=DeviceIoControl(m_DeviceHandle,
								IOCTL_SCSI_PASS_THROUGH,
								&sptwb,
								length,
								&sptwb,
								length,
								&returned,
								FALSE);
		/*
		sptwb.spt.Cdb[0] = Cmd;
		sptwb.spt.Cdb[1] = SubCommand;
		sptwb.spt.Cdb[6]  = (BYTE)((DataTransferLength) >> 24);
		sptwb.spt.Cdb[7]  = (BYTE)((DataTransferLength) >> 16);
		sptwb.spt.Cdb[8]  = (BYTE)((DataTransferLength) >>  8);
		sptwb.spt.Cdb[9]  = (BYTE)( DataTransferLength);	

		//status=SendScsiCmdIn(&sptwb, DataTransferLength);
		if (status==FALSE)
			return FALSE;
        */
		for (k=0;k<DataTransferLength;k++)
			databuf[k]=sptwb.ucDataBuf[k];
		if (status == FALSE)
			return -1;
	}
	else
	{
		//SCSI_PASS_THROUGH_DIRECT_WITH_BUFFER sptdwb;

		//ZeroMemory(&sptdwb,sizeof(SCSI_PASS_THROUGH_DIRECT_WITH_BUFFER));

		sptwb.spt.Length = sizeof(SCSI_PASS_THROUGH);
		sptwb.spt.PathId = 0;
		sptwb.spt.TargetId = 0;
		sptwb.spt.Lun = 0;
		sptwb.spt.SenseInfoLength = 24;
		sptwb.spt.DataIn = SCSI_IOCTL_DATA_OUT; 
		sptwb.spt.DataTransferLength = DataTransferLength;
		sptwb.spt.TimeOutValue = 120;	// ATTENTION: u-boot might takes a logn time to run a command, e.g. nand.write, ubi write
	    sptwb.spt.DataBufferOffset = offsetof(SCSI_PASS_THROUGH_WITH_BUFFERS,ucDataBuf);
    	sptwb.spt.SenseInfoOffset = offsetof(SCSI_PASS_THROUGH_WITH_BUFFERS,ucSenseBuf);	
		sptwb.spt.Cdb[0] = Cmd;

		sptwb.spt.Cdb[1] = SubCommand;
		memcpy(sptwb.ucDataBuf, databuf, DataTransferLength);
		sptwb.spt.CdbLength = 10;
		sptwb.spt.Cdb[6]  = (BYTE)((DataTransferLength) >> 24);
		sptwb.spt.Cdb[7]  = (BYTE)((DataTransferLength) >> 16);
		sptwb.spt.Cdb[8]  = (BYTE)((DataTransferLength) >>  8);
		sptwb.spt.Cdb[9]  = (BYTE)( DataTransferLength);
    	length = offsetof(SCSI_PASS_THROUGH_WITH_BUFFERS,ucDataBuf) +
		sptwb.spt.DataTransferLength;
		status=DeviceIoControl(m_DeviceHandle,
								IOCTL_SCSI_PASS_THROUGH,
								&sptwb,
								length,
								&sptwb,
								length,
								&returned,
								FALSE);
		//status=SendScsiCmdOut(&sptdwb,DataTransferLength,databuf);
		if (status==FALSE)
			return -1;
	}

	return 0;
}

int USB_ScsiWrite(unsigned char *buf, int len, ULONG max_packet)
{
	ULONG total = len;
	ULONG wlen = 0;
	ULONG temp = 0;
	BYTE sub_cmd = SSTAR_SCSICMD_SUBCODE_DOWNLOAD_KEEP;
	ULONG ulTimeout = 0;

	log("USB_ScsiWrite len %d maxpacket %lu\n", len, max_packet);
	while(total){
		if (total > max_packet) {
			temp = max_packet;
		}
		else {
			temp = total;
			sub_cmd = SSTAR_SCSICMD_SUBCODE_DOWNLOAD_END;
		}

		_USB_SEND_DATA:
		if (ScsiCmdSend(SSTAR_SCSICMD_OPCODE, sub_cmd, SCSI_BULK_OUT, temp, buf + wlen)) {
			Sleep(100);
			if (ulTimeout++ >= 1) {
				log("Scsi write time out:%lu\n", ulTimeout);
				return -1;
			}
			goto _USB_SEND_DATA;
		}
		total -= temp;
		wlen += temp;
	}
	return 0;
}

int USB_ScsiRead(UCHAR *databuf, int len, int retry)
{
	int ret;

	if (!databuf)
		return -1;
	if (retry == 0)
		retry = 10;
		
	databuf[0] = 0xFF;
	databuf[1] = 0xFF;
	databuf[2] = 0xFF;
	databuf[3] = 0xFF;
	do {
		ret = ScsiCmdSend(SSTAR_SCSICMD_OPCODE, SSTAR_SCSICMD_SUBCODE_GET_RESULT, SCSI_BULK_IN, len, databuf);
		if (ret == 0) {
			break;
		}
		else {
			Sleep(100);
			retry--;
		}
	} while (retry);
	return 0;
}

int USB_ScsiRunCmd(const char *cmd, int len)
{
	BYTE sub_cmd = SSTAR_SCSICMD_SUBCODE_UFU_RUN_CMD;
	ULONG ulTimeout = 0;

	if (ScsiCmdSend(SSTAR_SCSICMD_OPCODE, sub_cmd, SCSI_BULK_OUT, len, (UCHAR *)cmd)) {
		Sleep(500);
		if (ulTimeout++ >= 100) {
			log("Scsi runcmd time out:%lu\n", ulTimeout);
			return -1;
		}
	}
	return 0;
}

int USB_ScsiLoadInfo(void *info, int len)
{
	BYTE sub_cmd = SSTAR_SCSICMD_SUBCODE_UFU_LOADINFO;
	ULONG ulTimeout = 0;

	if (ScsiCmdSend(SSTAR_SCSICMD_OPCODE, sub_cmd, SCSI_BULK_OUT, len, (UCHAR *)info)) {
		Sleep(500);
		if (ulTimeout++ >= 100) {
			log("Scsi loadinfo time out:%lu\n", ulTimeout);
			return -1;
		}
	}
	return 0;
}

BOOL USB_ScsiInquiry(UCHAR *ucDataBuf)
{
	SCSI_PASS_THROUGH_WITH_BUFFERS sptwb;
	ULONG length;
	ULONG returned = 0;
	BOOL ret;

	ZeroMemory(&sptwb, sizeof(SCSI_PASS_THROUGH_WITH_BUFFERS));

	sptwb.spt.Cdb[0] = SCSIOP_INQUIRY;
	sptwb.spt.Cdb[4] = 0x24;
	sptwb.spt.Length = sizeof(SCSI_PASS_THROUGH);
	sptwb.spt.PathId = 0;
	sptwb.spt.TargetId = 1;
	sptwb.spt.Lun = 0;
	sptwb.spt.CdbLength = 6;
	sptwb.spt.SenseInfoLength = 24;
	sptwb.spt.DataIn = SCSI_IOCTL_DATA_IN;
	sptwb.spt.DataTransferLength = 0x24;
	sptwb.spt.TimeOutValue = 2;
	sptwb.spt.DataBufferOffset = offsetof(SCSI_PASS_THROUGH_WITH_BUFFERS, ucDataBuf);
	sptwb.spt.SenseInfoOffset = offsetof(SCSI_PASS_THROUGH_WITH_BUFFERS, ucSenseBuf);
	length = offsetof(SCSI_PASS_THROUGH_WITH_BUFFERS, ucDataBuf) + sptwb.spt.DataTransferLength;
	ret = DeviceIoControl(m_DeviceHandle,
							IOCTL_SCSI_PASS_THROUGH,
							&sptwb,
							sizeof(SCSI_PASS_THROUGH),
							&sptwb,
							length,
							&returned,
							FALSE);
	memcpy_s(ucDataBuf, 36, sptwb.ucDataBuf, 36);
	return ret;
}

static BOOL _ScsiFindDevice(TCHAR *diskname, int *dev_state)
{
	HANDLE fileHandle = NULL;
	BOOL ret;
	UCHAR ucDataBuf[36]; //inquire data

	fileHandle = CreateFile(diskname, (GENERIC_WRITE | GENERIC_READ), (FILE_SHARE_READ | FILE_SHARE_WRITE), NULL, OPEN_EXISTING, 0, NULL);
	if (fileHandle != INVALID_HANDLE_VALUE)
	{
		m_DeviceHandle = fileHandle;
		ret = USB_ScsiInquiry(ucDataBuf);
		/* Check Inquiry response.*/
		// Manufacturer: 'GCREADER
		log("Inquiry (%d):\n", ret);
		//fprintf(stdout, "%2x %2x %2x %2x %2x %2x %2x %2x\n", ucDataBuf[0], ucDataBuf[1], ucDataBuf[2], ucDataBuf[3], ucDataBuf[4], ucDataBuf[5], ucDataBuf[6], ucDataBuf[7]);
		//fprintf(stdout, "%2x %2x %2x %2x %2x %2x %2x %2x\n", ucDataBuf[8], ucDataBuf[9], ucDataBuf[10], ucDataBuf[11], ucDataBuf[12], ucDataBuf[13], ucDataBuf[14], ucDataBuf[15]);
		if ((ret == TRUE) && (ucDataBuf[8] == 'G') && (ucDataBuf[9] == 'C'))
		{
			if (ucDataBuf[16] == 0x0) {
				*dev_state = DEV_STATE_ROM;
				log("device state: ROM\n");
			}
			else if ((ucDataBuf[16] == 'U') && (ucDataBuf[17] == 'P') && (ucDataBuf[18] == 'D')) {
				*dev_state = DEV_STATE_UPDATER;
				log("device state: Updater\n");
			}
			else if ((ucDataBuf[16] == 'U') && (ucDataBuf[17] == 'B') && (ucDataBuf[18] == 'O')) {
				*dev_state = DEV_STATE_UBOOT;
				log("device state: u-Boot\n");
			}
			else {
				*dev_state = DEV_STATE_UNKNOWN;
			}
			return TRUE;
		}
		else
		{
			return FALSE; //modified by TX
			//m_DeviceHandle = NULL;     modified by TX
			//CloseHandle(fileHandle);   modified by TX
		}
	}
	else {
		m_DeviceHandle = NULL;
	}
	return FALSE;
}

HANDLE USB_ScsiRefindDevice(int *dev_state)
{
	TCHAR  diskname[10];
	size_t len;
	HRESULT hr;

	if (m_DeviceHandle)
	{
		CloseHandle(m_DeviceHandle);
		m_DeviceHandle = NULL;
	}

	if ((m_DeviceDisk < 0x44) || (m_DeviceDisk > 0x5A))
		return NULL;

	hr = StringCbLength("\\\\.\\F:", 10, &len);
	if (FAILED(hr))
		return NULL;

	hr = StringCchCopyN(diskname, 10, "\\\\.\\F:", len);
	if (FAILED(hr))
		return NULL;

	diskname[4] = m_DeviceDisk;
	if (_ScsiFindDevice(diskname, dev_state) == FALSE)
	{
		log("Wait disk %c ready\n", diskname[4]);
		*dev_state = DEV_STATE_UNKNOWN;
	}
	return m_DeviceHandle;
}

HANDLE USB_ScsiFindDevice(int *dev_state)
{
	TCHAR diskname[10];
	size_t len;
	HRESULT hr;

	hr = StringCbLength("\\\\.\\F:", 10, &len);
	if (FAILED(hr))
		return NULL;

	hr = StringCchCopyN(diskname, 10, "\\\\.\\F:", len);
	if (FAILED(hr))
		return NULL;

	for (int i=0x44;i<0x5B;i++)
	{
		diskname[4]=i;

		if (_ScsiFindDevice(diskname, dev_state))
		{
			m_DeviceDisk = i;
			return m_DeviceHandle;
		}
	}
	if (m_DeviceHandle == NULL)
		*dev_state = DEV_STATE_UNKNOWN;
	return m_DeviceHandle;
}
