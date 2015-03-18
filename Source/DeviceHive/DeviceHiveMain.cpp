/*-----------------------------------------------------------------------------
Autor                                                           Datum
Nikolay Peganov <Nikolay.Peganov@dataart.com>                   17.11.2014

-- BESCHREIBUNG ---------------------------------------------------------------


-- AENDERUNGEN ----------------------------------------------------------------

Autor                  Datum

-----------------------------------------------------------------------------*/

#include "ToolsLib.h"
#include "..\DeviceHive\Application.h"

DH::CApplication* g_pApp = 0;

//-----------------------------------------------------------------------------
extern"C"
{
   //-----------------------------------------------------------------------------
   void DeviceHiveMain()
   //-----------------------------------------------------------------------------
   {

      Tools::Logger::InitLogger();
      Tools::Logger::SetLoggerOutput(Tools::Logger::c_LogToConsole);
      Tools::Logger::SetLoggerMask(Tools::Logger::lmError | Tools::Logger::lmWarning |
                                 Tools::Logger::lmTrace | Tools::Logger::lmInfo);
      LOG(Tools::Logger::lmInfo, "Got started!"); 

      g_pApp = DH::CApplication::GetInstance();
      g_pApp->Start();
   }
}

