/**********************************************************************
* FileName: Main.c
* Version :
* Date    : 17.11.2014
*
* Comments:
*
* 
*
*END************************************************************************/
#include <mqx.h>
#include <bsp.h>

#include "DeviceHive\DeviceHiveMain.h"
#include "Peripherals\TestPin_think.h"

//-------------------------------------------------------------------------------------------------

/* Task IDs */
#define MAIN_TASK 1

#if MQX_VERSION < 411
#define UInt32mqx uint_32
#else
#define UInt32mqx UInt32
#endif

extern void Main_task(UInt32mqx);


//-------------------------------------------------------------------------------------------------
const TASK_TEMPLATE_STRUCT  MQX_template_list[] =
{
   // Task Index,   Function,      Stack,  Priority,   Name,   Attributes,          Param, Time Slice
    { MAIN_TASK,    Main_task,     5000,  13,          "main",      MQX_AUTO_START_TASK, 0,   0 },
    { 0 }
};

//-------------------------------------------------------------------------------------------------
void MCU_Init(void)
//-------------------------------------------------------------------------------------------------
{
   volatile int fmc_pfb01cr_register, fmc_pfb23cr_register;

   //wdog_setTimeoutValue(500, 0, 1);    // 500 ms

   // turn code cache on
   LMEM_PCCCR = (LMEM_PCCCR_GO_MASK | LMEM_PCCCR_INVW1_MASK | LMEM_PCCCR_INVW0_MASK | LMEM_PCCCR_ENWRBUF_MASK | LMEM_PCCCR_ENCACHE_MASK);
   while( LMEM_PCCCR & LMEM_PCCCR_GO_MASK ){};

   DDR_CR41 = 0x20;  // Priorität des LCD-Controllers für Zugriff auf das DRAM erhöhe.
 }

//-------------------------------------------------------------------------------------------------
void Main_task(UInt32mqx initial_data)
//-------------------------------------------------------------------------------------------------
{
   //MCU_Init();
   InitLED_FW();
   InitPeripherals();   
   ClearLED_FW();
   SetLED_Err();
   ClearLED_Err();
  
   DeviceHiveMain();
}
