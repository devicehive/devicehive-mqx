This is an example of a device, driven by Freescale MQX RTOS 4.1.1 (http://www.freescale.com/mqx). The example demonstrates how the device can register itself in the DeviceHive cloud, receive commands form the server and send notifications. 
This example was originally developed for a custom board with Freescale Kinetis K70 processor as a core and requires IAR Embedded Workbench for ARM v. 7.30 or higher installed.
For now the only available workspace configuration is Debug. The Release configuration will be available soon.

1.	Clone the source code
2.	Install Freescale MQX RTOS 4.1.1 into externals\MQX_4_1 subfolder of the project.
3.	CyaSSL library is bound as a submodule for the project and in order to make it to build successfully, apply a patch (externals\SSL\CyaSSL-patch.diff) for CyaSSL: from within externals\SSL\CyaSSL subfolder type 
		git apply --ignore-space-change --ignore-whitespace ../CyaSSL-patch.diff and press enter. 
	This action will modify the project files for building CyaSSL library and library’s settings to fit the requirements of the whole workspace.
4.	Follow this link http://devicehive.com/user/register to register, get a playground and get an appropriate URL to work with.
5.	Open IAR workspace: IDE\DeviceHive-MQX.eww
6.	Press Ctrl+Shift+F in order to find “nnXXXX.pg.devicehive.com” string in the source code and change this URL to the one, you got on step 4 (do not forget to add the beginning of the URL path, e.g. /api/).
7.	Make board-specific and project-specific changes:
	a.	Change required MQX source files according to MQX BSP modification and other instructions. 
		i.	You may skip this step, but you may want to use externals\MQX-patch.diff patch to make all the necessary changes in MQX RTOS sources for running this example on the custom board this example was originally developed for (to apply the patch type git apply --ignore-space-change --ignore-whitespace externals/ MQX-patch.diff from within the root folder of the project and press enter). This is only illustrational action and you must not expect these changes will fit your particular board requirement!
	b.	Modify TestPin_think.c and TestPin_think.h files in Peripherals subfolder to get LEDs and Push button of your board properly controllable. 
	c.	Set up the Debugger in DeviceHive-MQX->Debugger project’s options.
8.	In the IAR IDE go to Project->Batch build… menu, choose the Debug configuration and press Make button.
