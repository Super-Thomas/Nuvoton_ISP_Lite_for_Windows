# Nuvoton_ISP_Lite_for_Windows

## Intro
This is the Simple version of NuMicro ISP Programming Tool. NuMicro ISP Programming Tool is very good but it is complicated because there have many functions. This program only supports UART connection and programming to APROM. And supports only one device. So this source code is simple. You can study Nuvoton ISP programming process with this program. Also, You can development advanced program include ISP function. This program works base on Windows platform. If you searching for NuMicro ISP App on the android platform, Please refer [Using_Nuvoton_ISP_on_the_Android](https://github.com/Super-Thomas/Using_Nuvoton_ISP_on_the_Android).

## Development environment
|N|Name|Description|Note|
|---|---|---|---|
|1|Windows OS|Windows 10 Pro 19045.3324||
|2|Visual Studio|Visual Studio 2022 17.2.6|IDE|

## System Overview
![image](https://github.com/Super-Thomas/Nuvoton_ISP_Lite_for_Windows/assets/99227045/aedf9635-7c6d-4d5a-8d17-db95655eb58a)<br />
As shown in the figure above, it is connected to the target board with Nuvoton MCU through UART by using USB to UART bridge on Windows PC.

## How to ISP Firmware update
![image](https://github.com/Super-Thomas/Nuvoton_ISP_Lite_for_Windows/assets/99227045/3d1d7d66-0d00-4cf3-bad9-fcf188133010)<br />
This program is same as Nuvoton's ISP Tool. If you want know details on how to use it, Please refer this [documents](https://github.com/OpenNuvoton/ISPTool/tree/master/Documents).

## ISP update process
### Connect
![image](https://github.com/Super-Thomas/Nuvoton_ISP_Lite_for_Windows/assets/99227045/dcd88e41-ed5f-4e46-a46c-932b1e2f9ed5)<br />
### Programming
![image](https://github.com/Super-Thomas/Nuvoton_ISP_Lite_for_Windows/assets/99227045/84276e9c-7abb-4c1c-81bc-1873e2fd4e4d)<br />

## Packet Description
Packet size is 64 Byte. There have 4 Byte of command list and 4 Byte of command index. You can ignore command index in the packet.<br />
![image](https://user-images.githubusercontent.com/99227045/187109385-1f5d628d-3b3e-4147-8a75-4212bc504247.png)

Please refer table below for command list.
|Command|Value(Hex/4 Byte)|Description|Note|
|---|---|---|---|
|CMD_UPDATE_APROM|0x000000A0|Update data to the internal flash of target board.||
|CMD_CONNECT|0x000000AE|Try connect to target board.||
|CMD_GET_DEVICEID|0x000000B1|Request device ID to target board.||

### CMD_CONNECT
Try connect to target board. You must send this packet to target board every few millisecond during reset for target board. Target board must be received this packet when boot-up.<br />
Host -> Target board<br />
![image](https://user-images.githubusercontent.com/99227045/187111015-b0d0b4f9-41f3-4b6d-ad1d-0f6625513afc.png)

If target board received this packet from Host, Target board will send packet same as CMD_CONNECT packet to Host.<br />
Target board -> Host<br />
![image](https://user-images.githubusercontent.com/99227045/187111015-b0d0b4f9-41f3-4b6d-ad1d-0f6625513afc.png)

### CMD_GET_DEVICEID
Request device ID to target board.<br />
Host -> Target board<br />
![image](https://user-images.githubusercontent.com/99227045/187118255-9322f814-989c-419e-869b-21a086fa748b.png)

If target board received this packet from Host, Target board will send packet include device ID to Host. You can refer to device ID for each device [here](https://github.com/OpenNuvoton/ISPTool/blob/master/NuvoISP/DataBase/PartNumID.cpp). <br />
Target board -> Host<br />
![image](https://user-images.githubusercontent.com/99227045/187118659-7463d6d0-6e21-4e1c-9325-b3697b31cb6a.png)

### CMD_UPDATE_APROM
Update data to the internal flash of target board. There have two kind of packet. If you send CMD_UPDATE_APROM packet to target board for first time, You must send packet include Start address and Total length like as picture below.<br />
Host -> Target board<br />
![image](https://user-images.githubusercontent.com/99227045/187119655-311e2f82-5130-4420-bb5e-7e56a664be7a.png)<br />
Start Address: This is start address that where you need write buffer data.<br />
Total Length: This is total length for buffer you needs write to flash.<br /><br />
You can send following packet after sent packet for first time.<br />
Host -> Target board<br />
![image](https://user-images.githubusercontent.com/99227045/194185321-6a384b81-fe77-48d3-b3fe-4a0fb63326c8.png)

If target board received this packet from Host, Target board will send packet include CRC for received packet to Host.<br />
Target board -> Host<br />
![image](https://user-images.githubusercontent.com/99227045/187121331-1cfb4a61-0a57-45a2-97bc-07ef03d73421.png)

## Software Description
![image](https://github.com/Super-Thomas/Nuvoton_ISP_Lite_for_Windows/assets/99227045/e357c0f5-c38c-4c95-b617-26a20f800477)<br />

## Thanks to
https://github.com/OpenNuvoton/ISPTool<br />
https://www.codeproject.com/Articles/992/Serial-library-for-C
