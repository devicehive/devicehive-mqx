/*-----------------------------------------------------------------------------

Autor                                                           Datum
Nikolay Peganov <Nikolay.Peganov@dataart.com>                   17.11.2014

-- BESCHREIBUNG ---------------------------------------------------------------
The CApplication is a base class for a real firmware application. The firmware 
application is assumed to be derived from this class. It may have special 
implementation for each public method or use existing ones. CApplication has 
an access to a CGateway entity via m_pGateway member, which represents a 
DeviceHive gateway. 

-- AENDERUNGEN ----------------------------------------------------------------

Autor                  Datum

-----------------------------------------------------------------------------*/
#pragma once

#include "MQX.h"
#include "ToolsLib.h"
#include "GatewayDH.h"
#include "CommandExecutor.h"
#include "NonCopyable.h"

#include "Namespace.h"
//-----------------------------------------------------------------------------
class CApplication : public Tools::CThread // Thread implementation
                   , public CGateway::GatewayListener   // Gateway callback types
                   , public CCommandExecutor::ExecutorListener  // CommandExecutor callback types
                   , private NonCopyable // Prevent copying
{
private:
   // Construction 
   CApplication();   
   
public:
   // Destruction
   virtual ~CApplication();
   
public:   
      // delivers the singleton instance
   static CApplication* GetInstance();
     
public:
   // Start application
   virtual void Start(); // Starts the gateway

public:   
   virtual void HandleError(ResultDH error);  // Handels an error
   virtual void HandleCommand(cJSON* jCommand);  // Handels a command   
   // Sends a notification with a custom payload in the form of JSON    
   virtual void AsyncSendNotification(const char* pNotificationName, cJSON* jNotificationData); 
   virtual void AsyncAcknowledgeCommand(cJSON* jCommand); // Acknowledges a command
      
   
protected: // Callback methods 
   // This method is called after the Gateway sets up network (regardless successfully or not).
   // In the case of success, the Gateway will be attempting to connect to a server and,
   // finally, will call OnRegistered callback after Device registration.
   virtual void OnGatewayStarted(ResultDH result); 
   
   // (names are mostly self explanatory)  
   virtual void OnGotSerevrInfo(ResultDH result);    
   virtual void OnRegistered(ResultDH result, cJSON* jRegistrationInfo);            
   virtual void OnCommandExecuted(UInt32 result, cJSON* jCommand, CCommandExecutor* pExecutor); 
   virtual void OnCommandAcknowledged(ResultDH result, RetryableObject* pCommand);     
   virtual void OnNotificationSent(ResultDH result, cJSON* jServerNotification, RetryableObject* pSentNotification);    
   
public: // Public methods, used by the Gateway
   bool GetMacAddress(UInt8* macDest);
   bool GetSerial(char* serialDest, UInt8 destCapacity);   
   const char* GetServerUrl();     
   
protected:     // Thread Functions
   virtual UInt32 Run();
   virtual void ExitInstance();
   virtual void ExitInstance(UInt32 result);  
   
protected:
   // CCommandExecutor is an example of an asynchronous command executor.
   // Of course, you may handle commands synchronously (directly in HandleCommand() method),
   // but, I think, asynchronous handling is better way for doing this.
   class CRedLedExecutor : public CCommandExecutor
   {
      virtual UInt32 Run();
   };
   
   class CGreenLedExecutor : public CCommandExecutor
   {
      virtual UInt32 Run();
   };
   
   // Simple button checker. Can be used to fire notifications, or other simulations.
   class CButtonChecker : public Tools::CThread
   {
   private:
      CButtonChecker();
   public:
      explicit CButtonChecker(CApplication* pApp);
      virtual UInt32 Run();   
   private:
      CApplication* m_pApp;
   } m_ButtonChecker;
   bool m_ButtonCheckerStarted;
   
protected:
   CGateway* m_pGateway;
   static const UInt8 m_cAppStartPrio = 10;
   static const UInt32 m_cAppStartStackSize = 1000;
   char cServerUrl[128 + 1];   
   
   // The number of attempts to send a Notification/Acknowledgement before failure.
   static const UInt32 cRetryFailCount = 1;  
   
   static bool m_Terminated;
   
   static CApplication* m_pSingletone;
};

#include "NamespaceEnd.h"