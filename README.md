# DeviceHive C++ framework

This is MQX RTOS C++ framework for DeviceHive. For more information please see corresponding documentation and the description below.
## About DeviceHive

DeviceHive turns any connected device into the part of Internet of Things. It provides the communication layer, control software and multi-platform libraries to bootstrap development of smart energy, home automation, remote sensing, telemetry, remote control and monitoring software and much more. Connect embedded Linux using Python or C++ libraries and JSON protocol or connect AVR, Microchip devices using lightweight C libraries and BINARY protocol. Develop client applications using HTML5/JavaScript, iOS and Android libraries. For solutions involving gateways, there is also gateway middleware that allows to interface with devices connected to it. Leave communications to DeviceHive and focus on actual product and innovation.
## DeviceHive license

DeviceHive is developed by DataArt Apps and distributed under Open Source MIT license. This basically means you can do whatever you want with the software as long as the copyright notice is included. This also means you don't have to contribute the end product or modified sources back to Open Source, but if you feel like sharing, you are highly encouraged to do so!

Copyright (C) 2012 DataArt Apps

All Rights Reserved
## Description

This is an example of a device, driven by Freescale MQX RTOS 4.1.1 ([http://www.freescale.com/mqx](http://www.freescale.com/mqx)). The example demonstrates how the device can register itself in the DeviceHive cloud, receive commands form the server and send notifications. **This example was originally developed for a custom board with Freescale Kinetis K70 processor as a core** and requires IAR Embedded Workbench for ARM v. 7.30 or higher installed. For now the only available workspace configuration is Debug. The Release configuration will be available soon.

1.     Clone the source code
2.     Install Freescale MQX RTOS 4.1.1 into _externals\MQX_4_1_ subfolder of the project.
3.     CyaSSL library is bound as a submodule for the project and in order to make it to build successfully, apply a patch (_externals\SSL\CyaSSL-patch.diff_) for CyaSSL: from within _externals\SSL\CyaSSL_ subfolder type `git apply --ignore-space-change --ignore-whitespace ../CyaSSL-patch.diff` and press enter. This action will modify the project files for building CyaSSL library and library’s settings to fit the requirements of the whole workspace.
4.     Follow this link [http://devicehive.com/user/register](http://devicehive.com/user/register) to register, get a playground and get an appropriate URL to work with.
5.     Open IAR workspace: _IDE\DeviceHive-MQX.eww_
6.     Press Ctrl+Shift+F in order to find “`nnXXXX.pg.devicehive.com`” string in the source code and change this URL to the one, you got on step 4 (do not forget to add the beginning of the URL path, e.g. _/api/_).
7.     Make board-specific and project-specific changes:  
     7.1. **Optional.** Change required MQX source files according to MQX BSP modification and other instructions. You may skip this step, but you may want to use _externals\MQX-patch.diff_ patch to make all the necessary changes in MQX RTOS sources for running this example on the custom board this example was originally developed for (to apply the patch type `git apply --ignore-space-change --ignore-whitespace externals/ MQX-patch.diff` from within the root folder of the project and press enter). This is only illustrational action and you must not expect these changes will fit your particular board requirement!  
     7.2. Modify TestPin_think.c and TestPin_think.h files in Peripherals subfolder to get LEDs and Push button of your board properly controllable.  
     7.3. Set up the Debugger in DeviceHive-MQX->Debugger project’s options.
8.     In the IAR IDE go to Project->Batch build… menu, choose the Debug configuration and press Make button.

