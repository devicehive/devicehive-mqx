/*-----------------------------------------------------------------------------

Autor                                                           Datum
Nikolay Peganov <Nikolay.Peganov@dataart.com>                   17.11.2014

-- BESCHREIBUNG ---------------------------------------------------------------


-- AENDERUNGEN ----------------------------------------------------------------

Autor                  Datum

-----------------------------------------------------------------------------*/
#pragma once

#include "MQX.h"
#include "ToolsLib.h"
extern "C"
{
#include "cJSON.h"
}

#include "RetryableObject.h"
#include "HttpClient.h"
#include "HttpTask.h"
#include "NonCopyable.h"

#include "Namespace.h"

//-----------------------------------------------------------------------------

class CGateway;
class CRestProtocol : CHttpTask::TaskListener   
                    , private NonCopyable
{
public:
   class RestListener
   {
   public:
      typedef void (RestListener::* gwOnServerInfoCB)(ResultDH result, cJSON* jServerInfo);    
      typedef void (RestListener::* gwOnRegisterCB)(ResultDH result, cJSON* jRegistrationInfo); 
      typedef void (RestListener::* gwOnPollCommandsCB)(ResultDH result, cJSON* jCommands);    
      typedef void (RestListener::* gwOnCommandAckCB)(ResultDH result, RetryableObject* pCommand);      
      typedef void (RestListener::* gwOnNotifyCB)(ResultDH result, cJSON* jSerevrNotif, 
                                                  RetryableObject* pSentNotif);                                                    
   };
  
private:
   CRestProtocol();   
   
public:   
      // delivers the singleton instance
   static CRestProtocol* GetInstance();   
   void Init(CGateway* pGateway, const char* pServerUrl);
   void Stop();     
   
public:
   virtual ~CRestProtocol();

public:
   void SetUrl(const char* url);
   bool GetRegistrationStatus() const
   {return m_RegistrationIsInProgress == true;}
   size_t GetTaskCompletionTime() const
   {
      if (GetRegistrationStatus())
      {
         return cDefaultTimeoutMs + cRegistrationTime;
      }
      else
      {
         return cDefaultTimeoutMs;      
      }   
   }
   
public: // DH protocol methods   
   void AsyncGetServerInfo(RestListener::gwOnServerInfoCB Callback = 0);
   void AsyncRegister(cJSON* jDevice, RestListener::gwOnRegisterCB Callback = 0);  
  
   void AsyncPollCommands(const char* pLastCommandTimestamp, RestListener::gwOnPollCommandsCB Callback,
                          RetryableObject* p419RetryCounter = new RetryableObject());      
   
   void AsyncAcknowledgeCommand(RetryableObject* pCommand, RestListener::gwOnCommandAckCB Callback);    
   void AsyncSendNotification(const RetryableObject* pNotification, RestListener::gwOnNotifyCB Callback);               
   
private: // RestProtocol's collback methods
   void OnGotServerInfo(ResultDH result, CHttpTask* task);   
   void OnGotRegistered(ResultDH result, CHttpTask* task);    
   void OnPollCommands(ResultDH result, CHttpTask* task, RetryableObject* p419RetryCounter);     
   void OnCommandAcknowledged(ResultDH result, CHttpTask* task, RetryableObject* pCommand);
   void OnNotify(ResultDH result, CHttpTask* task, RetryableObject* pSentNotif);         

private:   
   void WaitForRegistrationCompleted();
   CHttpTask* WaitForFreeTask(ReqTypeEnum Type);
   
private: 
   CGateway* m_pGateway;
   CHttpClient* m_Http;     
   char* m_BaseUrl;   
   bool m_RegistrationIsInProgress;
   
   Tools::CEvent m_RegistrationCompletedEvent;
   static const UInt32 cRegistrationTime = 1000 * 20; // 20 seconds to complete registration
   Tools::CEvent m_FreeTaskEvent;   
   static const UInt32 cTaskFreeTime = 1000 * 20; // 20 seconds to complete get a free task
   void ReleaseTask(CHttpTask* task);
   
   // Gateway's callback pointers    
   RestListener::gwOnServerInfoCB m_OnServerInfoCallback;   
   RestListener::gwOnRegisterCB m_OnRegisterCallback;      
   RestListener::gwOnPollCommandsCB m_OnPollCommandsCallback;      
   RestListener::gwOnCommandAckCB m_OnCommandAckCallback;      
   RestListener::gwOnNotifyCB m_OnNotifyCallback;       
     
private:
   static CRestProtocol* m_pSingletone;       
   static const UInt32 cDefaultTimeoutMs = 1000 * 7;  
   static const UInt32 cCommandPollTimeoutMs = 1000 * 60 * 1;   
};


#include "NamespaceEnd.h"