/*-----------------------------------------------------------------------------

Autor                                                           Datum
Nikolay Peganov <Nikolay.Peganov@dataart.com>                   05.12.2014

-- BESCHREIBUNG ---------------------------------------------------------------


-- AENDERUNGEN ----------------------------------------------------------------

Autor                  Datum

-----------------------------------------------------------------------------*/
#pragma once
#include "MQX.h"
#include "ToolsLib.h"

#include "Connection.h"
#include "NonCopyable.h"
#include "Namespace.h"

class CConnectionGuard : public Tools::CThread
                       , private NonCopyable
{
public:
   class GuardListener
   {
   public:
      typedef void (GuardListener::* ExpirationCallback)(UInt32 result, CConnectionGuard* guard);               
   }; 

public:
   CConnectionGuard();      
   virtual ~CConnectionGuard();
   void Stop();
   
public:
   void Guard(CConnection* pConn, UInt32 expirationTimeMs, 
              GuardListener* pListener, 
              GuardListener::ExpirationCallback expirationCallback,
              const char* taskName = 0);
   bool Match(_ip_address ip, UInt16 port, bool needSecure);
   CConnection* TakeConnection();     
   
private:  
   // Thread Functions
   virtual UInt32 Run();
   virtual void ExitInstance(){};
   virtual void ExitInstance(UInt32 result); 
   
private:  
   static const UInt32 m_cTaskPrio = 10;
   static const UInt32 m_cTaskStackSize = 4096;     
   
private: 
   GuardListener* m_pListener;   
   GuardListener::ExpirationCallback m_pExpirationCallback; 
   CConnection* m_pConn;
   UInt32 m_ExpirationTimeMs;
   bool m_Interrupted;
   UInt32 m_Started;
};

#include "NamespaceEnd.h"