/*-----------------------------------------------------------------------------
Autor                  Datum
E. Fitze               14.06.2013

-- BESCHREIBUNG ---------------------------------------------------------------


-- AENDERUNGEN ----------------------------------------------------------------

Autor                  Datum

-----------------------------------------------------------------------------*/
#pragma once

#include <mqx.h>
#include <mutex.h>
#include "fio.h"

#include "CommonTypes.h"

#if defined(__cplusplus)
#include "Namespace.h"

//-----------------------------------------------------------------------------
class Logger
{
public:
   // constants for logger mask
   static const UInt32 lmError          =     0x0001;
   static const UInt32 lmWarning        =     0x0002;
   static const UInt32 lmInfo           =     0x0004;
   static const UInt32 lmTrace          =     0x0008;
   static const UInt32 lmData1          =     0x0010;
   static const UInt32 lmData2          =     0x0020;
   static const UInt32 lmFunction       =     0x0040;
   static const UInt32 lmPerformance    =     0x0080;

   static const UInt32 lmShowLevel      =     0x0100;
   static const UInt32 lmModACS         =     0x0200;  // Bei allen AppComService Logger Einträgen ist das Bit gesetzt.

   static const UInt32 lmShowParameter  =   0x010000;

public:
   static void InitLogger();

   static void SetLoggerMask(UInt32 mask);
   static UInt32 GetLoggerMask() {return s_loggerMask;}

   static void SetLoggerOutput(UInt32 mask);
   static void EnableLoggerOutput(UInt32 mask);
   static void DisableLoggerOutput(UInt32 mask);

   //static void InstallLoggerListener(ILogger * pListener);

   static const UInt32 c_LogToConsole;
   static const UInt32 c_LogToFile;
   static const UInt32 c_LogToDebugger;
   static const UInt32 c_LogToEthernet;

   static void Log(UInt32 mask, const char*, UInt32 lineNumber, char* sourceFilename);

public:
private: // static class do not support constructor...
   /** Constructor */
   Logger();

   /** Destructor */
   virtual ~Logger();

private:
   // flag to select logger regions
   static UInt32 s_loggerMask;

   // flag defines logging output.
   static UInt32 s_loggerOutput;

   //static ILogger    * pLoggerIF;

   static char* logBuffer;

public:
   /** MT synchronisation */
   static MUTEX_STRUCT mtxLogger;

   static char* tmpLoggerBuffer;

};



//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
#ifdef BSL_NO_LOGGER
#define LOG(msk, p)
#define LOG1(msk, p, p1)
#define LOG2(msk, p, p1, p2)
#define LOG3(msk, p, p1, p2, p3)
#define LOG4(msk, p, p1, p2, p3, p4)
#define LOG5(msk, p, p1, p2, p3, p4, p5)
#define LOG6(msk, p, p1, p2, p3, p4, p5, p6)
#define LOG7(msk, p, p1, p2, p3, p4, p5, p6, p7)

#else

#ifdef MCU_MVF50GS10MK50
#define NOT_IN_IRQ     !(__get_interrupt_state() & 0x80)
#else
#define NOT_IN_IRQ 1
#endif

#define LOG(msk, p)  {  if (msk & Tools::Logger::GetLoggerMask() && NOT_IN_IRQ)   \
   {  _mutex_lock(&Tools::Logger::mtxLogger); \
      Tools::Logger::Log(msk, p, __LINE__, __FILE__); \
      _mutex_unlock(&Tools::Logger::mtxLogger); } }

#define LOG1(msk, p, p1)  {  if ((msk) & Tools::Logger::GetLoggerMask() && NOT_IN_IRQ)   \
   {  _mutex_lock(&Tools::Logger::mtxLogger); \
      _io_sprintf(Tools::Logger::tmpLoggerBuffer, p, p1); \
      Tools::Logger::Log(msk, Tools::Logger::tmpLoggerBuffer, __LINE__, __FILE__); \
      _mutex_unlock(&Tools::Logger::mtxLogger);  } }
#define LOG2(msk, p, p1, p2)  {  if ((msk) & Tools::Logger::GetLoggerMask() && NOT_IN_IRQ)   \
   {  _mutex_lock(&Tools::Logger::mtxLogger); \
      _io_sprintf(Tools::Logger::tmpLoggerBuffer, p, p1, p2); \
      Tools::Logger::Log(msk, Tools::Logger::tmpLoggerBuffer, __LINE__, __FILE__); \
      _mutex_unlock(&Tools::Logger::mtxLogger);  } }
#define LOG3(msk, p, p1, p2, p3)  {  if ((msk) & Tools::Logger::GetLoggerMask() && NOT_IN_IRQ)   \
   {  _mutex_lock(&Tools::Logger::mtxLogger); \
      _io_sprintf(Tools::Logger::tmpLoggerBuffer, p, p1, p2, p3); \
      Tools::Logger::Log(msk, Tools::Logger::tmpLoggerBuffer, __LINE__, __FILE__); \
      _mutex_unlock(&Tools::Logger::mtxLogger);  } }
#define LOG4(msk, p, p1, p2, p3, p4)  {  if ((msk) & Tools::Logger::GetLoggerMask() && NOT_IN_IRQ)   \
   {  _mutex_lock(&Tools::Logger::mtxLogger); \
      _io_sprintf(Tools::Logger::tmpLoggerBuffer, p, p1, p2, p3, p4); \
      Tools::Logger::Log(msk, Tools::Logger::tmpLoggerBuffer, __LINE__, __FILE__); \
      _mutex_unlock(&Tools::Logger::mtxLogger);  } }
#define LOG5(msk, p, p1, p2, p3, p4, p5)  {  if ((msk) & Tools::Logger::GetLoggerMask() && NOT_IN_IRQ)   \
   {  _mutex_lock(&Tools::Logger::mtxLogger); \
      _io_sprintf(Tools::Logger::tmpLoggerBuffer, p, p1, p2, p3, p4, p5); \
      Tools::Logger::Log(msk, Tools::Logger::tmpLoggerBuffer, __LINE__, __FILE__); \
      _mutex_unlock(&Tools::Logger::mtxLogger);  } }
#define LOG6(msk, p, p1, p2, p3, p4, p5, p6)  {  if ((msk) & Tools::Logger::GetLoggerMask() && NOT_IN_IRQ)   \
   {  _mutex_lock(&Tools::Logger::mtxLogger); \
      _io_sprintf(Tools::Logger::tmpLoggerBuffer, p, p1, p2, p3, p4, p5, p6); \
      Tools::Logger::Log(msk, Tools::Logger::tmpLoggerBuffer, __LINE__, __FILE__); \
      _mutex_unlock(&Tools::Logger::mtxLogger);  } }
#define LOG7(msk, p, p1, p2, p3, p4, p5, p6, p7)  {  if ((msk) & Tools::Logger::GetLoggerMask() && NOT_IN_IRQ)   \
   {  _mutex_lock(&Tools::Logger::mtxLogger); \
      _io_sprintf(Tools::Logger::tmpLoggerBuffer, p, p1, p2, p3, p4, p5, p6, p7); \
      Tools::Logger::Log(msk, Tools::Logger::tmpLoggerBuffer, __LINE__, __FILE__); \
      _mutex_unlock(&Tools::Logger::mtxLogger);  } }


#endif

//-----------------------------------------------------------------------------

#include "NamespaceEnd.h"
#endif

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
#if !defined(__cplusplus)
#define lmError         0x0001
#define lmWarning       0x0002
#define lmInfo          0x0004
#define lmTrace         0x0008
#define lmData1         0x0010
#define lmData2         0x0020
#define lmFunction      0x0040
#define lmPerformance   0x0080


//-----------------------------------------------------------------------------
#define LOG(msk, p)  {  if (msk & GetLoggerMask())   \
   {  _mutex_lock(GetLogMutext());  \
      LogTxt(msk, p, __LINE__, __FILE__); \
      _mutex_unlock(GetLogMutext());  } }

#define LOG1(msk, p, p1)  {  if ((msk) & GetLoggerMask())   \
   {  _mutex_lock(GetLogMutext()); \
      _io_sprintf(GetLogBuffer(), p, p1); \
      LogTxt(msk, GetLogBuffer(), __LINE__, __FILE__); \
      _mutex_unlock(GetLogMutext());  } }

#define LOG2(msk, p, p1, p2)  {  if ((msk) & GetLoggerMask())   \
   {  _mutex_lock(GetLogMutext()); \
      _io_sprintf(GetLogBuffer(), p, p1, p2); \
      LogTxt(msk, GetLogBuffer(), __LINE__, __FILE__); \
      _mutex_unlock(GetLogMutext());  } }
#define LOG3(msk, p, p1, p2, p3)  {  if ((msk) & GetLoggerMask())   \
   {  _mutex_lock(GetLogMutext()); \
      _io_sprintf(GetLogBuffer(), p, p1, p2, p3); \
      LogTxt(msk, GetLogBuffer(), __LINE__, __FILE__); \
      _mutex_unlock(GetLogMutext());  } }
#define LOG4(msk, p, p1, p2, p3, p4)  {  if ((msk) & GetLoggerMask())   \
   {  _mutex_lock(GetLogMutext()); \
      _io_sprintf(GetLogBuffer(), p, p1, p2, p3, p4); \
      LogTxt(msk, GetLogBuffer(), __LINE__, __FILE__); \
      _mutex_unlock(GetLogMutext());  } }
#define LOG5(msk, p, p1, p2, p3, p4, p5)  {  if ((msk) & GetLoggerMask())   \
   {  _mutex_lock(GetLogMutext()); \
      _io_sprintf(GetLogBuffer(), p, p1, p2, p3, p4, p5); \
      LogTxt(msk, GetLogBuffer(), __LINE__, __FILE__); \
      _mutex_unlock(GetLogMutext());  } }
#define LOG6(msk, p, p1, p2, p3, p4, p5, p6)  {  if ((msk) & GetLoggerMask())   \
   {  _mutex_lock(GetLogMutext()); \
      _io_sprintf(GetLogBuffer(), p, p1, p2, p3, p4, p5, p6); \
      LogTxt(msk, GetLogBuffer(), __LINE__, __FILE__); \
      _mutex_unlock(GetLogMutext());  } }
#define LOG7(msk, p, p1, p2, p3, p4, p5, p6, p7)  {  if ((msk) & GetLoggerMask())   \
   {  _mutex_lock(GetLogMutext()); \
      _io_sprintf(GetLogBuffer(), p, p1, p2, p3, p4, p5, p6, p7); \
      LogTxt(msk, GetLogBuffer(), __LINE__, __FILE__); \
      _mutex_unlock(GetLogMutext());  } }


//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
void LogTxt(UInt32 mask, const char* txt, UInt32 lineNumber, char* sourceFilename);
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
UInt32 GetLoggerMask();
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
MUTEX_STRUCT* GetLogMutext();
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
char* GetLogBuffer();
//-----------------------------------------------------------------------------

#endif

