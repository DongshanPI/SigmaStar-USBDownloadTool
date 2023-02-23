Upgrade firwmare Flow:

1. Make sure device (chip boot in ROM USB mode) is ready
2. Make sure PC & device are connected with USB cable
3. Make sure the usb_updater.bin and u-boot.bin are in the same folder as USBDownloadTool.exe
4. Make sure the images binaries & scripts for u-boot upgrading are ready for upgrading
5. Execute USBDownloadTool.exe, click "Upgrade Firmware"

Upgrade firwmare flow in UVC device:
1. Make sure device run in UVC function, and support XU command for setting u-boot env ota_upgrade_status
2. Other steps refer to above steps 2 ~ 5.

编译说明
1. 使用Visual Studio打开USBDownloadTool下的工程, 选择Release发布
2. 将生成的USBDownloadTool.exe放在AitUVCExtApi.dll同一路径下才能运行