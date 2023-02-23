#ifndef AITAPI_H
#define AITAPI_H

#include <dshow.h>
#include <vidcap.h>

#pragma comment(lib,"winmm.lib")

#ifndef AITUVCEXTAPI_API
	#ifdef AITAPI_EXPORTS
		#define AITUVCEXTAPI_API __declspec(dllexport)
	#else
		#define AITUVCEXTAPI_API __declspec(dllimport)
	#endif
#endif

#define SUCCESS					0x00000000
#define FAIL					0x80000000
#define AITAPI_DEV_NOT_EXIST	0x80000003 
#define AITAPI_NOT_SUPPORT		0x80000001
#define AITAPI_NOT_ENOUGH_MEM	0x80000002
#define AITAPI_TIMEOUT			0x80000004

//old version
//#define SUCCESS 0
#define NOT_EXIST 0x8000000
#define	NOT_SUPPORT 0x80000001
#define NOT_ENOUGH_MEM 0x80000002

#define PAYLOAD_FORMAT_ID_YUY2 0	//YUY2
#define PAYLOAD_FORMAT_ID_MJPG 1	//MJPG
#define PAYLOAD_FORMAT_ID_UVC_11_H264	2	//UVC 1.1 H264

#define AIT_NVMEM_TABLE_SIZE (8*1024)

//////////////////////////////////////////////////////
typedef struct _AitH264FrameDesc
{
	USHORT	Length;		//Length of this descriptor
	USHORT	PayloadFmt;	//Payload format
	USHORT	Index;		//Frame index
	BYTE	FinalElem;	//If this frame descriptor is the final elememt in list this flag must be 1, other 
	USHORT	Width;		//H264 Frame width
	USHORT	Height;		//H264 Frame height
	USHORT	LocalWidth; //Local preview width
	USHORT	LocalHeight;//Local preview height
	UINT	MaxBitrate;	//Max bitrate
	UINT	MinBitrate; //Min bitrate
	USHORT	MaxFps;		//Max fps
	USHORT	MinFps;		//Min fps
	BYTE	data[1];	//attachment data
}AitH264FrameDesc;

typedef struct AitH264FrameList_
{
	USHORT Length;			//Length if this descriptor
	USHORT NumFrameDesc;	//Number of frame descriptor in this list
	BYTE FrameList[1];	//frame descriptor
}AitH264FrameList;

typedef void* AitDeviceHandle;
typedef void* AitNonVolatileHandle;
EXTERN_C typedef UINT (STDAPICALLTYPE *FP_Progress)(USHORT precent,void* callbackData);

typedef struct _AfACC
{
	UINT val[9];
}AfACC;

//// FW ERROR CODE , get from MmpCommand // 
#define FW_NO_ERROR 0x00
#define FW_ERROR 0x80
#define FW_FLASH_BURNING 0x80
#define FW_FLASH_BURNING_ERROR 0x82


//h264 payload and resolutions
EXTERN_C AITUVCEXTAPI_API UINT WINAPI AITAPI_GetH264FrameList(AitDeviceHandle DevHandle,AitH264FrameList **frame_list);
EXTERN_C AITUVCEXTAPI_API void WINAPI AITAPI_ReleaseH264FrameList(AitH264FrameList *frame_list);
EXTERN_C AITUVCEXTAPI_API HRESULT WINAPI AITAPI_SetH264FrameIndex(AitDeviceHandle DevHandle,USHORT Index);
EXTERN_C AITUVCEXTAPI_API HRESULT WINAPI AITAPI_GetH264_UVC_PayloadFormat(AitDeviceHandle DevHandle,USHORT PayloadFmtID,USHORT *width,USHORT *height,GUID *guid);

//
EXTERN_C AITUVCEXTAPI_API HRESULT WINAPI AITAPI_OpenDeviceByPath(const TCHAR *devPath,AitDeviceHandle* pDevHandle);
EXTERN_C AITUVCEXTAPI_API HRESULT WINAPI AITAPI_OpenDeviceBySourceFilter(IBaseFilter *src_filter,AitDeviceHandle* pDevHandle);
EXTERN_C AITUVCEXTAPI_API HRESULT WINAPI AITAPI_OpenDevice(UINT slave_id,AitDeviceHandle* pDevHandle);
EXTERN_C AITUVCEXTAPI_API HRESULT WINAPI AITAPI_OpenDeviceEx(UINT slave_id,AitDeviceHandle* pDevHandle,UINT Vid,UINT Pid);
EXTERN_C AITUVCEXTAPI_API HRESULT WINAPI AITAPI_OpenDeviceByDevPath(const TCHAR *devPath,AitDeviceHandle* pDevHandle);
EXTERN_C AITUVCEXTAPI_API HRESULT WINAPI AITAPI_OpenAitDevice(AitDeviceHandle* pDevHandle);
EXTERN_C AITUVCEXTAPI_API void WINAPI AITAPI_CloseDevice(AitDeviceHandle* pDevHandle);

EXTERN_C AITUVCEXTAPI_API HRESULT WINAPI AITAPI_UvcExtSet(AitDeviceHandle DevHandle,const GUID *euGuid,USHORT CmdId,BYTE* dataBuf,UINT BufLen,ULONG *bytesRet);
EXTERN_C AITUVCEXTAPI_API HRESULT WINAPI AITAPI_UvcExtGet(AitDeviceHandle DevHandle,const GUID *euGuid,USHORT CmdId,BYTE* dataBuf,UINT BufLen,ULONG *bytesRet);
EXTERN_C AITUVCEXTAPI_API HRESULT WINAPI AITAPI_AitExtSet(AitDeviceHandle DevHandle,USHORT CmdId,BYTE* dataBuf,UINT BufLen,ULONG *bytesRet);
EXTERN_C AITUVCEXTAPI_API HRESULT WINAPI AITAPI_AitExtGet(AitDeviceHandle DevHandle,USHORT CmdId,BYTE* dataBuf,UINT BufLen,ULONG *bytesRet);
EXTERN_C AITUVCEXTAPI_API HRESULT WINAPI AITAPI_IspCommand(AitDeviceHandle DevHandle,BYTE *cmdIn,BYTE *cmdOut);
EXTERN_C AITUVCEXTAPI_API HRESULT WINAPI AITAPI_MmpCommand(AitDeviceHandle DevHandle,BYTE *cmdIn,BYTE *cmdOut);
EXTERN_C AITUVCEXTAPI_API HRESULT WINAPI AITAPI_UvcCheckXuGuid(AitDeviceHandle DevHandle,const GUID *euGuid);
EXTERN_C AITUVCEXTAPI_API HRESULT WINAPI AITAPI_SetFps(AitDeviceHandle DevHandle,USHORT fps);
EXTERN_C AITUVCEXTAPI_API HRESULT WINAPI AITAPI_SetBitRate(AitDeviceHandle DevHandle,USHORT kbps);
EXTERN_C AITUVCEXTAPI_API HRESULT WINAPI AITAPI_SetH264Mode(AitDeviceHandle DevHandle,UINT Width,UINT Height,UINT Mode);
EXTERN_C AITUVCEXTAPI_API HRESULT WINAPI AITAPI_ForceIFrame(AitDeviceHandle DevHandle);
EXTERN_C AITUVCEXTAPI_API HRESULT WINAPI AITAPI_GetDeviceInfo(AitDeviceHandle DevHandle,BYTE *info);
EXTERN_C AITUVCEXTAPI_API HRESULT WINAPI AITAPI_GetFWVersion(AitDeviceHandle DevHandle,BYTE *fwVer);
EXTERN_C AITUVCEXTAPI_API HRESULT WINAPI AITAPI_SetMaxQP(AitDeviceHandle DevHandle,BYTE MaxQp);
EXTERN_C AITUVCEXTAPI_API HRESULT WINAPI AITAPI_SetMinQP(AitDeviceHandle DevHandle,BYTE MinQp);
EXTERN_C AITUVCEXTAPI_API HRESULT WINAPI AITAPI_SetPframeCount(AitDeviceHandle DevHandle,ULONG PCount);

//AITAPI_GetSensorSN
//read sensor serial number
//[out]BYTE *sn: 32 byte buffer to receive sensor number,
// format:	BYTE[0] ErrCode
//			BYTE[1] length
//			BYTE[2~31]	serial number
EXTERN_C AITUVCEXTAPI_API HRESULT WINAPI AITAPI_GetSensorSN(AitDeviceHandle DevHandle, BYTE *sn);


//AITAPI_SetRegister
//Write ait chip register 
//[in] USHORT Addr: OPR register address 
//[in] BYTE Value: 
EXTERN_C AITUVCEXTAPI_API HRESULT WINAPI AITAPI_SetRegister(AitDeviceHandle DevHandle,USHORT Addr,BYTE Val);
//AITAPI_SetRegister
//Write ait chip register 
//[in] USHORT Addr: OPR register address 
//[out] BYTE *Val: 
EXTERN_C AITUVCEXTAPI_API HRESULT WINAPI AITAPI_GetRegister(AitDeviceHandle DevHandle,USHORT Addr,BYTE *Val);
EXTERN_C AITUVCEXTAPI_API HRESULT WINAPI AITAPI_GetSkypeXUVersion(AitDeviceHandle DevHandle,BYTE *ver);

//for customer setting
//[in] USHORT r: R gain
//[in] USHORT g: G gain
//[in] USHORT b: B gain
EXTERN_C AITUVCEXTAPI_API HRESULT WINAPI AITAPI_SetAWBGain(AitDeviceHandle DevHandle,USHORT r,USHORT g,USHORT b);

//AITAPI_SetAWBGain
//[out] USHORT *r: pointer to unsigned short to receivce R gain from device
//[out] USHORT *g: 
//[out] USHORT *b: 
EXTERN_C AITUVCEXTAPI_API HRESULT WINAPI AITAPI_GetAWBGain(AitDeviceHandle DevHandle,USHORT *r,USHORT *g,USHORT *b);


#ifndef ISP_CFG_GET_MISC
#define	ISP_MISC_FLIP	0x01
#define	ISP_MISC_MIRROR	0x02
#define ISP_MISC_BLACK_IMG	0x04
#endif
//AITAPI_SetMiscSetting
//[in] BYTE *data : pointer to 6 bytes data for Miscellaneous Setting
//				BYTE 0
//						Bit0: Flip
//						Bit1: Mirror
//						Bit2: Black Image 
EXTERN_C AITUVCEXTAPI_API HRESULT WINAPI AITAPI_SetMiscSetting(AitDeviceHandle DevHandle,BYTE *data);

//AITAPI_GetMiscSetting
//[out] BYTE *data : pointer to 6 bytes data for Miscellaneous Setting
//				BYTE 0
//						Bit0: Flip
//						Bit1: Mirror
//						Bit2: Black Image 
EXTERN_C AITUVCEXTAPI_API HRESULT WINAPI AITAPI_GetMiscSetting(AitDeviceHandle DevHandle,BYTE *data);


//AITAPI_SetColorEffect
//[in] BYTE EffectID: Effect ID 
#define ISP_IMAGE_EFFECT_NORMAL   0
#define ISP_IMAGE_EFFECT_GREY      1
//ISP_IMAGE_EFFECT_SEPIA      2
//ISP_IMAGE_EFFECT_NEGATIVE  3
//ISP_IMAGE_EFFECT_ANTIQUE   4
//ISP_IMAGE_EFFECT_WATERCOLOR  5
//ISP_IMAGE_EFFECT_PORTRAIT  6
//ISP_IMAGE_EFFECT_LANDSCAPE  7
//ISP_IMAGE_EFFECT_SUNSET    8
//ISP_IMAGE_EFFECT_DUSK      9
//ISP_IMAGE_EFFECT_DAWN     10
//ISP_IMAGE_EFFECT_RED       11
//ISP_IMAGE_EFFECT_GREEN    12
//ISP_IMAGE_EFFECT_BLUE      13
//ISP_IMAGE_EFFECT_YELLOW   15
//ISP_IMAGE_EFFECT_EMBOSS   17
//ISP_IMAGE_EFFECT_OIL       18
//ISP_IMAGE_EFFECT_BW       19
//ISP_IMAGE_EFFECT_SKETCH   20
//ISP_IMAGE_EFFECT_CRAYONE  21
//ISP_IMAGE_EFFECT_WHITEBOARD 22
//ISP_IMAGE_EFFECT_BLACKBOARD  23
//ISP_IMAGE_EFFECT_VIVID     24
EXTERN_C AITUVCEXTAPI_API HRESULT WINAPI AITAPI_SetImageEffect(AitDeviceHandle DevHandle,BYTE EffectID);

EXTERN_C AITUVCEXTAPI_API HRESULT WINAPI AITAPI_SetShadding(AitDeviceHandle DevHandle,BOOL Enable);
EXTERN_C AITUVCEXTAPI_API HRESULT WINAPI AITAPI_EnableManualGainCtrl(AitDeviceHandle DevHandle,BOOL Enable);

EXTERN_C AITUVCEXTAPI_API HRESULT WINAPI AITAPI_SetMjpgQuality(AitDeviceHandle DevHandle,BYTE Quality);

#define VID_REC_MODE_DISABLE	0
#define VID_REC_MODE_FIX_30_FPS	1
#define VID_REC_MODE_FIX_24_FPS	2
EXTERN_C AITUVCEXTAPI_API HRESULT WINAPI AITAPI_SetVideoRecMode(AitDeviceHandle DevHandle,BYTE mode);
EXTERN_C AITUVCEXTAPI_API HRESULT WINAPI AITAPI_IspCommandEx(AitDeviceHandle DevHandle, BYTE *cmdIn, BYTE *cmdOut);
EXTERN_C AITUVCEXTAPI_API HRESULT WINAPI AITAPI_MmpCommandEx(AitDeviceHandle DevHandle, BYTE *cmdIn, BYTE *cmdOut);

//------------------------------------- firmware r/w -----------------------------------------------------------------
EXTERN_C AITUVCEXTAPI_API HRESULT WINAPI AITAPI_GetCheckSumResult(AitDeviceHandle DevHandle);

EXTERN_C AITUVCEXTAPI_API HRESULT WINAPI AITAPI_UpdateFW_842x(AitDeviceHandle DevHandle,BYTE* fw_data,UINT len,FP_Progress ProgressCB,void *callbackData,BYTE mode);
EXTERN_C AITUVCEXTAPI_API HRESULT WINAPI AITAPI_UpdateFW_843x(AitDeviceHandle DevHandle,BYTE* fw_data,UINT len,FP_Progress ProgressCB,void *callbackData,BYTE mode);
EXTERN_C AITUVCEXTAPI_API HRESULT WINAPI AITAPI_WriteFWData(AitDeviceHandle DevHandle,BYTE* data,UINT Len,FP_Progress ProgressCB,void* callbackData);
EXTERN_C AITUVCEXTAPI_API HRESULT WINAPI AITAPI_GetFWBuildDate(AitDeviceHandle DevHandle,char *fwBuildDay);
EXTERN_C AITUVCEXTAPI_API HRESULT WINAPI AITAPI_SendData(AitDeviceHandle DevHandle, BYTE* fw_data, UINT len, FP_Progress ProgressCB, void *callbackData, BYTE StorageType, BYTE PackageType);
EXTERN_C AITUVCEXTAPI_API HRESULT WINAPI AITAPI_UpdateFlash(AitDeviceHandle DevHandle,BYTE* fw_data,UINT len,FP_Progress ProgressCB,void *callbackData,BYTE StorageType, BYTE PackageType);
EXTERN_C AITUVCEXTAPI_API HRESULT WINAPI AITAPI_UpdateCaliData(AitDeviceHandle DevHandle, BYTE* fw_data,UINT len,FP_Progress ProgressCB,void *callbackData,BYTE mode);

//--------------------------------------------------------------------------------------------------------------------

//-------------------------------------- Sensor r/w ------------------------------------------------------------------
//AITAPI_GetSensorRegister
//Read image sensor register 
//[in]USHORT Addr: sensor address
//[out]USHORT *val: 1 byte buffer to receive register value
EXTERN_C AITUVCEXTAPI_API HRESULT WINAPI AITAPI_GetSensorRegister(AitDeviceHandle DevHandle,USHORT Addr,BYTE *val);
//AITAPI_SetSensorRegister
//Set image sensor register 
//[in]USHORT Addr: sensor address
//[in]USHORT Val: 1 byte value to write to sensor register 
EXTERN_C AITUVCEXTAPI_API HRESULT WINAPI AITAPI_SetSensorRegister(AitDeviceHandle DevHandle,USHORT Addr,BYTE Val);
//AITAPI_ReadCodecReg
//Read image sensor register 
//[in]USHORT addr: sensor address
//[out]USHORT *val: 1 byte buffer to receive register value
EXTERN_C AITUVCEXTAPI_API HRESULT WINAPI AITAPI_ReadCodecReg(AitDeviceHandle handle, USHORT addr, BYTE *val);
//AITAPI_WriteCodecReg
//Read image sensor register 
//[in]USHORT addr: sensor address
//[out]USHORT *val: 1 byte value to write to sensor register 
EXTERN_C AITUVCEXTAPI_API HRESULT WINAPI AITAPI_WriteCodecReg(AitDeviceHandle handle, USHORT addr, BYTE val);

EXTERN_C AITUVCEXTAPI_API HRESULT WINAPI AITAPI_SetAudioRegisterW(AitDeviceHandle DevHandle,USHORT Addr,USHORT Val);
EXTERN_C AITUVCEXTAPI_API HRESULT WINAPI AITAPI_GetAudioRegisterW(AitDeviceHandle DevHandle,USHORT Addr,USHORT *val);
//-----------------------------------------------------------------------------------------------------------------------

//for Camellia project only
EXTERN_C AITUVCEXTAPI_API HRESULT WINAPI AITAPI_CAMELLIA_WriteNVMEM(AitDeviceHandle DevHandle,BYTE *buf,UINT len,UINT *actual_write );
EXTERN_C AITUVCEXTAPI_API HRESULT WINAPI AITAPI_CAMELLIA_ReadNVMEM(AitDeviceHandle DevHandle,BYTE *buf,UINT buf_size,UINT *actual_read);
EXTERN_C AITUVCEXTAPI_API HRESULT WINAPI AITAPI_CAMELLIA_LedCtrl(AitDeviceHandle DevHandle,BYTE ctrl);
//EXTERN_C AITUVCEXTAPI_API HRESULT WINAPI AITAPI_WriteNVMEM(AitDeviceHandle DevHandle, UINT write_addr, BYTE *buf,UINT buf_len,UINT *actual_write_len);
//EXTERN_C AITUVCEXTAPI_API HRESULT WINAPI AITAPI_WriteNVMEM(AitDeviceHandle DevHandle,BYTE *buf,UINT buf_size,UINT *actual_read);

#define VIDEO_REC_MODE_DISABLE		0
#define VIDEO_REC_MODE_CONST_30FPS	1
#define VIDEO_REC_MODE_CONST_24FPS	2
#define VIDEO_REC_MODE_VAR_FPS		3
EXTERN_C AITUVCEXTAPI_API HRESULT WINAPI AITAPI_SetVideoRecMode(AitDeviceHandle DevHandle,BYTE mode);

EXTERN_C AITUVCEXTAPI_API HRESULT WINAPI AITAPI_ReadTable(AitDeviceHandle DevHandle, BYTE TableID, BYTE *buf, UINT buf_len, UINT *actual_read_len);
EXTERN_C AITUVCEXTAPI_API HRESULT WINAPI AITAPI_WriteTable(AitDeviceHandle DevHandle, BYTE TableID, BYTE *buf,UINT buf_len,UINT *actual_write_len);

//AIT Non-Volatile Table function
#define TABLE_ID_VID_PID	0x01		//pid, vid
#define TABLE_ID_MSTRING	0x11		//manufacturer string
#define TABLE_ID_PSTRING	0x12		//product string
#define TABLE_ID_SNSTRING	0x13		//serial number
#define TABLE_ID_MAX_POWER	0x21		//maximum power consumption
#define TABLE_ID_TEMP_DESC	0x31		//
#define TABLE_ID_SENSOR_INIT	0x41	//image sensor initial data
#define TABLE_ID_VCM			0x51	//vcm controller initial data
#define TABLE_ID_VCM_INIT		0x51	
#define TABLE_ID_ADC_INIT		0x61	//ADC initial data
#define TABLE_ID_IO_INIT		0x71	//IO initial data
#define TABLE_ID_PU_INIT		0x72	//IO initial data
#define TABLE_ID_CT_INIT		0x73	//IO initial data
#define TABLE_ID_CD_INIT		0x74	//IO initial data
#define TABLE_ID_VENDER_TABLE	0x81		

EXTERN_C AITUVCEXTAPI_API HRESULT WINAPI AITAPI_OpenTableHandle(AitDeviceHandle DevHandle,AitNonVolatileHandle *pTableHandle);
EXTERN_C AITUVCEXTAPI_API HRESULT WINAPI AITAPI_CloseTableHandle(AitDeviceHandle DevHandle,AitNonVolatileHandle *pTableHandle);
EXTERN_C AITUVCEXTAPI_API HRESULT WINAPI AITAPI_GetTableSize(AitDeviceHandle DevHandle,AitNonVolatileHandle TableHandle,BYTE TableID,USHORT *tableSize);
EXTERN_C AITUVCEXTAPI_API HRESULT WINAPI AITAPI_WriteTableEx(AitDeviceHandle DevHandle,AitNonVolatileHandle TableHandle,BYTE TableID, BYTE *buf,UINT buf_len,UINT *actual_write_len);
EXTERN_C AITUVCEXTAPI_API HRESULT WINAPI AITAPI_ReadTableEx(AitDeviceHandle DevHandle,AitNonVolatileHandle TableHandle,BYTE TableID, BYTE *buf, UINT buf_len, UINT *actual_read_len);

//AF control
EXTERN_C AITUVCEXTAPI_API HRESULT WINAPI AITAPI_SetVCMRegister(AitDeviceHandle DevHandle,USHORT Addr,USHORT Val);
EXTERN_C AITUVCEXTAPI_API HRESULT WINAPI AITAPI_GetVCMRegister(AitDeviceHandle DevHandle,USHORT Addr,USHORT *val);
EXTERN_C AITUVCEXTAPI_API HRESULT WINAPI AITAPI_EnableAF(AitDeviceHandle DevHandle,BYTE Enable);
EXTERN_C AITUVCEXTAPI_API HRESULT WINAPI AITAPI_EnableAE(AitDeviceHandle DevHandle,BYTE Enable);
EXTERN_C AITUVCEXTAPI_API HRESULT WINAPI AITAPI_GetAFACC(AitDeviceHandle DevHandle,AfACC *afacc);


//device streaming
typedef void* AitVideoStreamHandle;
EXTERN_C AITUVCEXTAPI_API AitVideoStreamHandle WINAPI AITSTREAM_CreateVideoStreamHandle(AitDeviceHandle DevHandle);
EXTERN_C AITUVCEXTAPI_API void WINAPI AITSTREAM_CloseVideoStreamHandle(AitVideoStreamHandle *handle);
EXTERN_C AITUVCEXTAPI_API HRESULT WINAPI AITSTREAM_StartVideoStreaming(AitVideoStreamHandle Handle,UINT Width,UINT Height,UINT FmtFCC);
EXTERN_C AITUVCEXTAPI_API HRESULT WINAPI AITSTREAM_StopVideoStreaming(AitVideoStreamHandle Handle);
EXTERN_C AITUVCEXTAPI_API HRESULT WINAPI AITSTREAM_SetVideoWindowPos(AitVideoStreamHandle Handle,UINT Top, UINT Left, UINT Right, UINT Bottom);

EXTERN_C AITUVCEXTAPI_API void NV12_to_RGB(BYTE* nv12_buf,BYTE* rgb_buf,UINT ImgW,UINT ImgH);
EXTERN_C AITUVCEXTAPI_API BOOL WINAPI ConvertYUY2toBMP(LPCWSTR file_name,BYTE *yuv_buffer,UINT width,UINT height);
EXTERN_C AITUVCEXTAPI_API BOOL WINAPI ConvertNV12toBMP(LPCWSTR file_name,BYTE *nv12_buffer,UINT width,UINT height);
EXTERN_C AITUVCEXTAPI_API BOOL WINAPI PackWavFile( LPCTSTR RawFileName, LPCTSTR WavFileName, BYTE Ch, DWORD SmpRate, BYTE BitPerSmp);
EXTERN_C AITUVCEXTAPI_API BOOL WINAPI ConvertRGBtoBMP(LPCWSTR file_name,BYTE *rgb_buffer,UINT width,UINT height);

EXTERN_C AITUVCEXTAPI_API HRESULT WINAPI AITAPI_GetVideoProcAmp(AitDeviceHandle DevHandle,long Property,long *vaule, long *flag);
EXTERN_C AITUVCEXTAPI_API HRESULT WINAPI AITAPI_SetVideoProcAmp(AitDeviceHandle DevHandle,long Property,long vaule, long flag);
EXTERN_C AITUVCEXTAPI_API HRESULT WINAPI AITAPI_GetVideoProcAmpRange(AitDeviceHandle DevHandle,
															long Property,
															long *pMin,
															long *pMax,
															long *pSteppingDelta,
															long *pDefault,
															long *pCapsFlags
															);
AITUVCEXTAPI_API HRESULT WINAPI AITAPI_SetCameraControl(AitDeviceHandle DevHandle,long Property,long value, long flag);
AITUVCEXTAPI_API HRESULT WINAPI AITAPI_GetCameraControlRange(AitDeviceHandle DevHandle,
															long Property,
															long *pMin,
															long *pMax,
															long *pSteppingDelta,
															long *pDefault,
															long *pCapsFlags
															);
AITUVCEXTAPI_API HRESULT WINAPI AITAPI_GetCameraControl(AitDeviceHandle DevHandle,
															long Property,
															long *Value,
															long *flag
															);
EXTERN_C AITUVCEXTAPI_API HRESULT WINAPI AITAPI_WriteInitialTable(AitDeviceHandle DevHandle,BYTE *buf, UINT buf_len, UINT *actual_write_len);
EXTERN_C AITUVCEXTAPI_API HRESULT WINAPI AITAPI_DumpWholeTable(AitDeviceHandle DevHandle,AitNonVolatileHandle TableHandle,BYTE *buf,UINT buf_len,UINT *actual_dump_len);
EXTERN_C AITUVCEXTAPI_API HRESULT WINAPI AITAPI_ReadFWData(AitDeviceHandle DevHandle,BYTE* data,UINT BufLen,UINT *byteRet,FP_Progress ProgressCB,void* callbackData);
EXTERN_C AITUVCEXTAPI_API HRESULT WINAPI AITAPI_ReadPartialFlashData(AitDeviceHandle DevHandle,UINT FlashAddr,USHORT Len,BYTE* buf,USHORT *byteRet);
EXTERN_C AITUVCEXTAPI_API HRESULT WINAPI AITAPI_ResetToRomboot(AitDeviceHandle DevHandle);

EXTERN_C AITUVCEXTAPI_API HRESULT WINAPI AITAPI_ResetToRomboot(AitDeviceHandle DevHandle);
EXTERN_C AITUVCEXTAPI_API HRESULT WINAPI AITAPI_ResetToRomboot_MMP8(AitDeviceHandle DevHandle);

//for directshow
#ifdef __IVideoProcAmp_INTERFACE_DEFINED__
EXTERN_C AITUVCEXTAPI_API HRESULT WINAPI AITAPI_Get_IVideoProcAmp_Interface(AitDeviceHandle DevHandle,void** pIVideoProcAmp);
#endif 

//for AUDIO devices
typedef void* AitAudioDeviceHandle;
EXTERN_C AITUVCEXTAPI_API HRESULT WINAPI AITAPI_AudioOpenDeviceByPath(LPCTSTR DevPath,AitAudioDeviceHandle* pDevHandle);
EXTERN_C AITUVCEXTAPI_API void AITAPI_AudioCloseDevice(AitAudioDeviceHandle* pDevHandle);
EXTERN_C AITUVCEXTAPI_API HRESULT WINAPI AITAPI_AudioSetVolume(AitAudioDeviceHandle DevHandle,int volume);

/** @brief Commit multicast video format
@param[in] handle Api handle from by AITAPI_OpenDevice.
@param[in] StreamType image format
@param[in] Width image width
@param[in] Height image height
@param[in] Fps 
@param[in] Bps stream bitrate in 100kbps
@remark Stream types
\n1:H264
\n2:MJPG
\n3:YUY2
\n4:NV12
\n5:NV21
\n6:Y(Gray)
\n7:M2TS
\n8:RAW(8 bits)
\n9:RAW(10 bits)
@retval HRESULT
*/
EXTERN_C AITUVCEXTAPI_API HRESULT WINAPI AITAPI_MulticastCommit(AitDeviceHandle DevHandle,BYTE StreamType,USHORT Width,USHORT Height,BYTE Fps,USHORT Bps);

/** @brief Select desired stream id, 
@param[in] handle Api handle from by AITAPI_OpenDevice.
@param[in] bStreamID StreamID indicates which stream all XU stream specific controls are manipulating.
@retval HRESULT
*/
EXTERN_C AITUVCEXTAPI_API HRESULT WINAPI AITAPI_MulticastStreamID(AitDeviceHandle DevHandle,BYTE StreamID);

/** @brief Toggle stream, Dynamic start/stop multicast stream 
@param[in] handle Api handle from by AITAPI_OpenDevice.
@param[in] Enable Control starts or stops the currently selected multicast stream designated by the current value of StreamID (set by the StreamID control) if supported
@retval HRESULT
*/
EXTERN_C AITUVCEXTAPI_API HRESULT WINAPI AITAPI_MulticasToggleLayer(AitDeviceHandle DevHandle,BYTE Enable);
#endif
