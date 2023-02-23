#include <windows.h>
#include "stdafx.h"
#include "devioctl.h"
#include "ntdddisk.h"
#include "ntddscsi.h"
#include "SCSIDef.h"

#define DATA_TRANSFER_LENGTH	(32*1024) 

/* SSTAR SCSI */
#define SSTAR_SCSICMD_OPCODE	0xE8
#define SSTAR_SCSICMD_SUBCODE_DOWNLOAD_KEEP	0x1
#define SSTAR_SCSICMD_SUBCODE_GET_RESULT    0x2 // Get Result
#define SSTAR_SCSICMD_SUBCODE_GET_STATE     0x3 // Get device state
#define SSTAR_SCSICMD_SUBCODE_DOWNLOAD_END	0x4
// Additional subcode for usb firmware update gadget in uboot
#define SSTAR_SCSICMD_SUBCODE_UFU_LOADINFO	0x5 // Give load addr & size
#define SSTAR_SCSICMD_SUBCODE_UFU_RUN_CMD	0x6 // Give a command string to run

/* Device State */
enum {
	DEV_STATE_ROM = 0,
	DEV_STATE_UPDATER,
	DEV_STATE_UBOOT,
	DEV_STATE_UNKNOWN,
};

typedef enum _SCSI_BULK_DIRECTION{
	SCSI_BULK_OUT = 0,
	SCSI_BULK_IN
}SCSI_BULK_DIRECTION;

int USB_ScsiWrite(unsigned char *buf, int len, ULONG max_packet);
int USB_ScsiRead(unsigned char *databuf, int len, int retry);
BOOL USB_ScsiInquiry(UCHAR *ucDataBuf);
int USB_ScsiRunCmd(const char *cmd, int len);
int USB_ScsiLoadInfo(void *info, int len);
HANDLE USB_ScsiFindDevice(int *dev_state);
HANDLE USB_ScsiRefindDevice(int *dev_state);
