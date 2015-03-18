/*-----------------------------------------------------------------------------
Autor                  Datum
E. Fitze               14.06.2013

-- BESCHREIBUNG ---------------------------------------------------------------


-- AENDERUNGEN ----------------------------------------------------------------

Autor                  Datum

-----------------------------------------------------------------------------*/
#include "Logger.h"

#include "string.h"

#include "OS.h"

using namespace Tools;

//-----------------------------------------------------------------------------
const UInt32 Logger::c_LogToConsole  = 0x001;
const UInt32 Logger::c_LogToFile     = 0x002;
const UInt32 Logger::c_LogToDebugger = 0x004;
const UInt32 Logger::c_LogToEthernet = 0x008;

UInt32 Logger::s_loggerMask = 0;
UInt32 Logger::s_loggerOutput = 0;

//ILogger * Logger::pLoggerIF = 0;

// Critical section for access to single instance
MUTEX_STRUCT   Logger::mtxLogger;

char* Logger::logBuffer=NULL;
char* Logger::tmpLoggerBuffer=NULL;


//-----------------------------------------------------------------------------
Logger::Logger()
//-----------------------------------------------------------------------------
{
}
//-----------------------------------------------------------------------------
Logger::~Logger()
//-----------------------------------------------------------------------------
{
}

//-----------------------------------------------------------------------------
void Logger::InitLogger()
//-----------------------------------------------------------------------------
{
   _mutex_init(&mtxLogger, NULL);
   tmpLoggerBuffer = (char*)OS::AllocateSRAM(512);
   logBuffer       = (char*)OS::AllocateSRAM(512);
}

//-----------------------------------------------------------------------------
void Logger::SetLoggerMask(UInt32 mask)
//-----------------------------------------------------------------------------
{
   s_loggerMask = mask;
}

//-----------------------------------------------------------------------------
void Logger::SetLoggerOutput(UInt32 mask)
//-----------------------------------------------------------------------------
{
   s_loggerOutput = mask;
}
//-----------------------------------------------------------------------------
void Logger::EnableLoggerOutput(UInt32 mask)
//-----------------------------------------------------------------------------
{
   s_loggerOutput |= mask;
}
//-----------------------------------------------------------------------------
void Logger::DisableLoggerOutput(UInt32 mask)
//-----------------------------------------------------------------------------
{
   s_loggerOutput &= ~mask;
}




//extern Bool SendLoggerData(UInt8* pData, UInt32 len);

//-----------------------------------------------------------------------------
void Logger::Log(UInt32 mask, const char* str, UInt32 lineNumber, char* sourceFilename)
//-----------------------------------------------------------------------------
{
   TIME_STRUCT     time;
   UInt32          len;

   {  // find den letzten backslash in sourceFilename.
      len = strlen(sourceFilename);
      while(len)
      {
         len--;
         if (sourceFilename[len] == '\\')
         {
            sourceFilename = &sourceFilename[len+1];
            break;
         }
      }
   }
   UInt32 bitNumber, msk;
   msk = mask;
   msk |= 0x80;
   bitNumber = 0;
   while ((msk & 1) == 0)
   {
      msk >>= 1;
      bitNumber++;
   }

   _time_get_elapsed(&time);
   _io_sprintf(logBuffer, "[%X] %3d.%03d %s(%d): %s\n", /*bitNumber+1*/_task_get_id(), time.SECONDS, time.MILLISECONDS, sourceFilename, lineNumber, str);

   // write out the Logger text...
   if (s_loggerOutput & c_LogToConsole)
   {
      _io_printf(logBuffer);
   }
   
   /*
   if (s_loggerOutput & c_LogToEthernet)
   {
      if (!(mask & lmModACS))
      {
         SendLoggerData((UInt8*)logBuffer, strlen(logBuffer));
      }
   }
   */
}


extern"C"
{
   //-----------------------------------------------------------------------------
   void LogTxt(UInt32 mask, const char* txt, UInt32 lineNumber, char* sourceFilename)
   {
      Logger::Log(mask, txt, lineNumber, sourceFilename);
   }

   UInt32 GetLoggerMask()
   {
      return Logger::GetLoggerMask();
   }


   MUTEX_STRUCT* GetLogMutext()
   {
      return &Logger::mtxLogger;
   }

   char* GetLogBuffer()
   {
      return Logger::tmpLoggerBuffer;
   }

}
