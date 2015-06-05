/*-----------------------------------------------------------------------------

Autor                                                           Datum
Nikolay Peganov <Nikolay.Peganov@dataart.com>                   04.12.2014

-- BESCHREIBUNG ---------------------------------------------------------------


-- AENDERUNGEN ----------------------------------------------------------------

Autor                  Datum

-----------------------------------------------------------------------------*/

#include "rtcs.h"
#include "ConnectionGuard.h"

using namespace DH;

// Construction
//-----------------------------------------------------------------------------
CConnectionGuard::CConnectionGuard()
: m_pListener(0)
, m_pExpirationCallback(0)
, m_pConn(0)
, m_ExpirationTimeMs(0)
, m_Interrupted(false)
//-----------------------------------------------------------------------------
{
   LOG1(Tools::Logger::lmInfo, "CConnectionGuard{0x%X} created", this);
}

//-----------------------------------------------------------------------------
CConnectionGuard::~CConnectionGuard()
//-----------------------------------------------------------------------------
{
   Stop();
   LOG1(Tools::Logger::lmInfo, "CConnectionGuard{0x%X} destroyed", this);
}

//-----------------------------------------------------------------------------
void CConnectionGuard::Stop()
//-----------------------------------------------------------------------------
{
   LOG1(Tools::Logger::lmInfo, "Stopping CConnectionGuard{0x%X}", this);

   if (m_pConn)
   {
      delete m_pConn;
      m_pConn = 0;
   }
}

//-----------------------------------------------------------------------------
void CConnectionGuard::Guard(CConnection* pConn, UInt32 expirationTimeMs,
                             GuardListener* pListener,
                             GuardListener::ExpirationCallback expirationCallback,
                             const char* taskName)
//-----------------------------------------------------------------------------
{
   m_pListener = pListener;
   m_pExpirationCallback = expirationCallback;
   m_pConn = pConn;
   m_ExpirationTimeMs = expirationTimeMs;
   m_Started = GetTickCount();

   LOG3(Tools::Logger::lmInfo, "Starting a ConnectionGuard{0x%X}, on Conn{0x%X} with %d ms timeout.",
        this, pConn, m_ExpirationTimeMs);

   if (!taskName)
   {
      strcpy(m_TaskName, "ConnectionGuard");
   }
   else
   {
      strcpy(m_TaskName, taskName);
   }

   CreateThread(m_TaskName, m_cTaskPrio, m_cTaskStackSize);
}

//-----------------------------------------------------------------------------
UInt32 CConnectionGuard::Run()
//-----------------------------------------------------------------------------
{
   LOG2(Tools::Logger::lmInfo, "Running a ConnectionGuard{0x%X}, on Conn{0x%X}",
        this, m_pConn);
   if (m_pConn)
      return m_pConn->Wait(m_ExpirationTimeMs);
   else
      return RTCS_OK;

}

//-----------------------------------------------------------------------------
CConnection* CConnectionGuard::TakeConnection()
//-----------------------------------------------------------------------------
{
   CConnection* result = m_pConn;
   m_pConn = 0;
   m_Interrupted = true;
   return result;
}

//-----------------------------------------------------------------------------
bool CConnectionGuard::Match(_ip_address ip, UInt16 port, bool needSecure)
//-----------------------------------------------------------------------------
{
   if(m_pConn)
   {
      return (ip == m_pConn->GetIP()) &&
               (port == m_pConn->GetPort()) &&
               (needSecure == m_pConn->IsSecure());
   }
   else
      return false;
}

//-----------------------------------------------------------------------------
void CConnectionGuard::ExitInstance(UInt32 result)
//-----------------------------------------------------------------------------
{
   if (result == RTCS_OK)
   {
      LOG1(Tools::Logger::lmInfo, "ConnectionGuard{0x%X} has been interrupted", this);
   }
   else
   {
      if (result == RTCSERR_TCP_TIMED_OUT)
      {
         LOG1(Tools::Logger::lmInfo, "ConnectionGuard{0x%X} has expired", this);
      }
      else
      {
         LOG2(Tools::Logger::lmError, "ConnectionGuard{0x%X} has completed with error, code: 0x%05X", this, result);
      }

      if (m_pConn)
      {
         delete m_pConn;
         m_pConn = 0;
      }
   }

   float lifeTime = (GetTickCount() - m_Started) / 1000;
   LOG2(Tools::Logger::lmInfo, "ConnectionGuard{0x%X} has lived %3.3f seconds",
        this, lifeTime);

   UInt32 ThreadID = GetCurrentThreadId();

   if (m_pListener && m_pExpirationCallback)
   {
      GuardListener* L = m_pListener;
      GuardListener::ExpirationCallback cb = m_pExpirationCallback;

      m_pListener = 0;
      m_pExpirationCallback = 0;

      (L->*cb)(result, this); // WARN: Do not refer to any task member after calling this callback!!!
                              // The task is not valid (destroyed) since this moment!
   }

   LOG1(Tools::Logger::lmTrace, "ConnectionGuard{0x%X}: Finishing...", this);
}
