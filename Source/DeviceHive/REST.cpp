/*-----------------------------------------------------------------------------

Autor                                                           Datum
Nikolay Peganov <Nikolay.Peganov@dataart.com>                   17.11.2014

-- BESCHREIBUNG ---------------------------------------------------------------


-- AENDERUNGEN ----------------------------------------------------------------

Autor                  Datum

-----------------------------------------------------------------------------*/
#include "REST.h"
#include "Connection.h"
#include "GatewayDH.h"
#include "string.h"

using namespace DH;

CRestProtocol* CRestProtocol::m_pSingletone = 0;

//-----------------------------------------------------------------------------
CRestProtocol* CRestProtocol::GetInstance()
//-----------------------------------------------------------------------------
{
   if (0 == m_pSingletone)
   {  // Create instance
      m_pSingletone = new CRestProtocol;
   }

   // deliver reference to instance
   return m_pSingletone;
}

//-----------------------------------------------------------------------------
CRestProtocol::CRestProtocol()
//-----------------------------------------------------------------------------
: m_pGateway(0)
, m_Http(CHttpClient::GetInstance())
, m_BaseUrl(0)
, m_RegistrationIsInProgress(false)

, m_OnServerInfoCallback(0)
, m_OnRegisterCallback(0)
, m_OnPollCommandsCallback(0)
, m_OnCommandAckCallback(0)
, m_OnNotifyCallback(0)
//-----------------------------------------------------------------------------
{
}

//-----------------------------------------------------------------------------
void CRestProtocol::Init(CGateway* pGateway, const char* pServerUrl)
//-----------------------------------------------------------------------------
{
   m_pGateway = pGateway;
   m_BaseUrl = (char*)pServerUrl;
}

//-----------------------------------------------------------------------------
CRestProtocol::~CRestProtocol()
//-----------------------------------------------------------------------------
{
   /*
   // m_BaseUrl is a pointer to a Gateway's member, containing ServerURL
   if (m_BaseUrl)
   {
      delete [] m_BaseUrl;
      m_BaseUrl = 0;
   }
   */

   if (m_Http)
   {
      delete m_Http;
      m_Http = 0;
   }
}

//-----------------------------------------------------------------------------
void CRestProtocol::AsyncGetServerInfo(RestListener::gwOnServerInfoCB Callback)
//-----------------------------------------------------------------------------
{
   m_RegistrationIsInProgress = true;
   m_RegistrationCompletedEvent.Reset(); 

   m_OnServerInfoCallback = Callback;

   CHttpTask* pHttpTask = WaitForFreeTask(RT_GET);
   LOG1(Tools::Logger::lmInfo, "Getting Server Info, Task{0x%X}", pHttpTask);

   pHttpTask->AddPath("info");
   pHttpTask->FinishUrl();
   pHttpTask->AddHeader(HTTP::Accept, "*/*");
   pHttpTask->AddHeader(HTTP::Connection, "keep-alive");
   pHttpTask->Finalize();

   const UInt32 cTimeoutMs = cDefaultTimeoutMs;

   ResultDH result = m_Http->AsyncSend(pHttpTask, this,
                                       (CHttpTask::TaskListener::TaskCallback)&CRestProtocol::OnGotServerInfo,
                                       cTimeoutMs, "Getting ServerInfo");

   if (result != DH_NO_ERROR)
   {
      m_pGateway->NotifyAboutError(result);// Calls to CApplication::HandleError() method
      return;
      // You may want to call m_OnServerInfoCallback here like this:
      /*
      if (m_OnServerInfoCallback)
      {
         RestListener::gwOnServerInfoCB cb = m_OnServerInfoCallback;
         m_OnServerInfoCallback = 0;

         ((CRestProtocol::RestListener*)(m_pGateway)->*cb)(result, NULL);
      }
      */

      // But be carefull with multithreading in this case. It is highly desirable
      // to call this callback from within a dedicated thread
      // Also pay attantion to a  callback parameter value (NULL, in this case)
      // This approach needs to be carefully tested!!!
   }
}

//-----------------------------------------------------------------------------
void CRestProtocol::OnGotServerInfo(ResultDH result, CHttpTask* task)
//-----------------------------------------------------------------------------
{
   LOG1(Tools::Logger::lmInfo, "OnGotServerInfo, Task{0x%X}", task);
   cJSON* pServerInfoJson = 0;
   bool interrupted = false;

   if (result == DH_NO_ERROR)
   {
      if (task->ResponseIsComplited() && task->HttpStatusIsSuccessfull())
      {
         LOG(Tools::Logger::lmInfo, "Successfully got ServerInfo");
         char* pContent = (char*)task->GetResponseContent();
         if(pContent != 0)
         {
            pServerInfoJson = cJSON_Parse(pContent);
         }
         else
         {
            LOG1(Tools::Logger::lmWarning, "No content in response, Task{0x%X}", task);

            result = FormatError(ErrorDH::EC_ContentIsExpected, ErrorDH::ET_AppSpecific,
                                 ErrorDH::EP_RestOnGotServerInfo,
                                 "OnGotServerInfo: Response content is expected");
         }
      }
      else if (!task->ResponseIsComplited())
      {
         LOG2(Tools::Logger::lmError, "Failed to get ServerInfo, Task{0x%X}, ResponseState: %d",
              task, task->GetResponseState());

         result = FormatError(task->GetResponseState(), ErrorDH::ET_InvalidResponse,
                              ErrorDH::EP_RestOnGotServerInfo,
                              "Failed to get ServerInfo: Invalid response");
      }
      else
      {
         LOG2(Tools::Logger::lmError, "Failed to get ServerInfo, Task{0x%X}, HttpStatusCode: %d",
              task, task->GetHttpStatus());
         result = FormatError(task->GetHttpStatus(), ErrorDH::ET_HttpStatus,
                              ErrorDH::EP_RestOnGotServerInfo,
                              "Failed to get ServerInfo: Invalid HTTP status");
      }
   }
   else
   {
      LOG1(Tools::Logger::lmError, "Failed to get ServerInfo, Task{0x%X}. Connection error", task);
      interrupted = result->Interrupted();
   }

   ReleaseTask(task);

   if (m_OnServerInfoCallback)
   {
      RestListener::gwOnServerInfoCB cb = m_OnServerInfoCallback;
      m_OnServerInfoCallback = 0;

      ((CRestProtocol::RestListener*)(m_pGateway)->*cb)(result, pServerInfoJson);
   }
   else //No callback from the Gateway -> Register device on our own accord
   {
      if (result)
      {
         delete result;
         if (!interrupted)
            m_pGateway->AsyncGetServerInfo();
      }
      else
         m_pGateway->AsyncRegister();
   }
}


//-----------------------------------------------------------------------------
void CRestProtocol::AsyncRegister(cJSON* jDevice, RestListener::gwOnRegisterCB Callback)
//-----------------------------------------------------------------------------
{
   m_OnRegisterCallback = Callback;
   CHttpTask* pHttpTask = WaitForFreeTask(RT_PUT);
   LOG1(Tools::Logger::lmInfo, "Registering the Device, Task{0x%X}", pHttpTask);

   pHttpTask->AddPath("device");
   pHttpTask->AddPath(m_pGateway->GetDeviceID());
   pHttpTask->FinishUrl();
   pHttpTask->AddHeader(HTTP::Content_Type, "application/json");
   pHttpTask->AddHeader(HTTP::Accept, "application/json");
   pHttpTask->AddHeader(HTTP::Connection, "keep-alive");

   pHttpTask->AddHeader(HTTP::AuthDeviceID, m_pGateway->GetDeviceID());
   pHttpTask->AddHeader(HTTP::AuthDeviceKey, m_pGateway->GetDeviceMAC());      

   if (jDevice)
   {
      char* jsonStr = cJSON_PrintUnformatted(jDevice);
      pHttpTask->SetContent(jsonStr);
      if (jsonStr)
      {
         delete [] jsonStr;
         jsonStr = 0;
      }
   }

   const UInt32 cTimeoutMs = cDefaultTimeoutMs;
   ResultDH result = m_Http->AsyncSend(pHttpTask, this,
                        (CHttpTask::TaskListener::TaskCallback)&CRestProtocol::OnGotRegistered,
                        cTimeoutMs, "Registering");

   if (result != DH_NO_ERROR)
   {
      m_pGateway->NotifyAboutError(result);// Calls to CApplication::HandleError() method
      return;
      // You may want to call m_OnRegisterCallback here like this:
      /*
      if (m_OnRegisterCallback)
      {
         RestListener::gwOnRegisterCB cb = m_OnRegisterCallback;
         m_OnRegisterCallback = 0;

         ((CRestProtocol::RestListener*)(m_pGateway)->*cb)(result, NULL);
      }
      */

      // But be carefull with multithreading in this case. It is highly desirable
      // to call this callback from within a dedicated thread
      // Also pay attantion to a  callback parameter value (NULL, in this case)
      // This approach needs to be carefully tested!!!
   }
}

//-----------------------------------------------------------------------------
void CRestProtocol::OnGotRegistered(ResultDH result, CHttpTask* task)
//-----------------------------------------------------------------------------
{
   LOG(Tools::Logger::lmInfo, "OnGotRegistered");
   cJSON* pRegisterInfoJson = 0;
   bool interrupted = false;
   bool retry = false;

   if (result == DH_NO_ERROR)
   {
      if (task->ResponseIsComplited() && task->HttpStatusIsSuccessfull())
      {
         LOG(Tools::Logger::lmInfo, "Successfully got Registered");
         char* pContent = (char*)task->GetResponseContent();
         if(pContent != 0)
         {
            pRegisterInfoJson = cJSON_Parse(pContent);
         }
         else
         {
            LOG1(Tools::Logger::lmWarning, "No content in response, Task{0x%X}", task);
            result = FormatError(ErrorDH::EC_ContentIsExpected, ErrorDH::ET_AppSpecific,
                                 ErrorDH::EP_RestOnGotRegistered,
                                 "OnGotRegistered: Response content is expected");
         }
      }
      else if (!task->ResponseIsComplited())
      {
         LOG2(Tools::Logger::lmError, "Failed to Register, Task{0x%X}, ResponseState: %d",
              task, task->GetResponseState());

         result = FormatError(task->GetResponseState(), ErrorDH::ET_InvalidResponse,
                              ErrorDH::EP_RestOnGotRegistered,
                              "Failed to Register: Invalid response");
      }
      else
      {
         LOG2(Tools::Logger::lmError, "Failed to Register, Task{0x%X} HttpStatusCode: %d",
              task, task->GetHttpStatus());

         result = FormatError(task->GetHttpStatus(), ErrorDH::ET_HttpStatus,
                              ErrorDH::EP_RestOnGotRegistered,
                              "Failed to Register: Invalid HTTP status");
      }
   }
   else
   {
      LOG1(Tools::Logger::lmError, "Failed to Register, Task{0x%X}. Connection error", task);
      interrupted = result->Interrupted();
   }

   ReleaseTask(task);

   if (!retry && m_OnRegisterCallback) // This Gateway's callback always exists
                                       // when the Gateway calls CRestProtocol::AsyncRegister
   {
      m_RegistrationIsInProgress = false;
      m_RegistrationCompletedEvent.Set();

      RestListener::gwOnRegisterCB cb = m_OnRegisterCallback;
      m_OnRegisterCallback = 0;

      ((CRestProtocol::RestListener*)(m_pGateway)->*cb)(result, pRegisterInfoJson);
   }
   else if (retry)
   {
      if (result)
         delete result;

      if (!interrupted)
         AsyncGetServerInfo();
   }
}
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
void CRestProtocol::AsyncPollCommands(const char* pLastCommandTimestamp,
                                      RestListener::gwOnPollCommandsCB Callback,
                                      RetryableObject* p419RetryCounter)
//-----------------------------------------------------------------------------
{
   // Please note, that p419RetryCounter object keeps no sensible data,
   // except fail retry counter for 419 http status code

   m_OnPollCommandsCallback = Callback;
   LOG2(Tools::Logger::lmInfo, "Rest{0x%X} is about to poll commands with callback {0x%X}",
        this, m_OnPollCommandsCallback);

   WaitForRegistrationCompleted();
   CHttpTask* pHttpTask = WaitForFreeTask(RT_GET);

   pHttpTask->AddPath("device");
   pHttpTask->AddPath(m_pGateway->GetDeviceID());
   pHttpTask->AddPath("command");
   pHttpTask->AddPath("poll");

   pHttpTask->QueryPath("timestamp", pLastCommandTimestamp);
   char strTimeout[16];
   sprintf(strTimeout, "%d", cCommandPollTimeoutMs / 1000);
   pHttpTask->QueryPath("waitTimeout", strTimeout);

   pHttpTask->FinishUrl();

   pHttpTask->AddHeader(HTTP::Content_Type, "application/json");
   pHttpTask->AddHeader(HTTP::Accept, "application/json");
   pHttpTask->AddHeader(HTTP::Connection, "keep-alive");
   
   pHttpTask->AddHeader(HTTP::AuthDeviceID, m_pGateway->GetDeviceID());
   pHttpTask->AddHeader(HTTP::AuthDeviceKey, m_pGateway->GetDeviceMAC());    
   
   pHttpTask->Finalize();

   const UInt32 cTimeoutMs = cCommandPollTimeoutMs + cDefaultTimeoutMs;

   ResultDH result = m_Http->AsyncSendWithObject(pHttpTask, this,
                         (CHttpTask::TaskListener::TaskWithObjectCallback)&CRestProtocol::OnPollCommands,
                         cTimeoutMs, (void*)p419RetryCounter, "PollCommands");

   if (result != DH_NO_ERROR)
   {
      delete p419RetryCounter;
      p419RetryCounter = 0;

      m_pGateway->NotifyAboutError(result);// Calls to CApplication::HandleError() method
      return;
      // You may want to call m_OnPollCommandsCallback here like this:
      /*
      if (m_OnPollCommandsCallback)
      {
         RestListener::gwOnPollCommandsCB cb = m_OnPollCommandsCallback;
         m_OnPollCommandsCallback = 0;

         ((CRestProtocol::RestListener*)(m_pGateway)->*cb)(result, NULL);
      }
      */

      // But be carefull with multithreading in this case. It is highly desirable
      // to call this callback from within a dedicated thread
      // Also pay attantion to a  callback parameter value (NULL, in this case)
      // This approach needs to be carefully tested!!!
   }
}

//-----------------------------------------------------------------------------
void CRestProtocol::OnPollCommands(ResultDH result, CHttpTask* task,
                                   RetryableObject* p419RetryCounter)
//-----------------------------------------------------------------------------
{
   LOG(Tools::Logger::lmInfo, "OnPollCommands");
   bool retry = false;

   cJSON* pCommandsJson = 0;

   if (result == DH_NO_ERROR)
   {
      if (task->ResponseIsComplited() && task->HttpStatusIsSuccessfull())
      {
         LOG1(Tools::Logger::lmInfo, "Got Poll Commands response, Task{0x%X}", task);
         char* pContent = (char*)task->GetResponseContent();
         if(pContent != 0)
         {
            pCommandsJson = cJSON_Parse(pContent);
         }
         else
         {
            LOG1(Tools::Logger::lmWarning, "No content in response, Task{0x%X}", task);
            result = FormatError(ErrorDH::EC_ContentIsExpected, ErrorDH::ET_AppSpecific,
                                 ErrorDH::EP_RestOnPollCommands,
                                 "OnPollCommands: Response content is expected");
         }
      }
      else if (!task->ResponseIsComplited())
      {
         LOG2(Tools::Logger::lmError, "Failed to get commands, Task{0x%X}, ResponseState: %d",
              task, task->GetResponseState());

         result = FormatError(task->GetResponseState(), ErrorDH::ET_InvalidResponse,
                              ErrorDH::EP_RestOnPollCommands,
                              "Failed to get commands: Invalid response");
      }
      else
      {
         LOG2(Tools::Logger::lmError, "Failed to get commands, Task{0x%X} HttpStatusCode: %d",
              task, task->GetHttpStatus());

         result = FormatError(task->GetHttpStatus(), ErrorDH::ET_HttpStatus,
                              ErrorDH::EP_RestOnPollCommands,
                              "Failed to get commands: Invalid HTTP status");
      }
   }
   else
   {
      LOG1(Tools::Logger::lmError, "Failed to get commands, Task{0x%X}. Connection error", task);
   }

   ReleaseTask(task);

   if (!retry && m_OnPollCommandsCallback)
   {
      if (p419RetryCounter)
      {
         delete p419RetryCounter;
         p419RetryCounter = 0;
      }

      LOG2(Tools::Logger::lmInfo, "Rest {0x%X} has got commands and calling callback {0x%X}",
           this, m_OnPollCommandsCallback);

      RestListener::gwOnPollCommandsCB cb = m_OnPollCommandsCallback;
      m_OnPollCommandsCallback = 0;
      ((CRestProtocol::RestListener*)(m_pGateway)->*cb)(result, pCommandsJson);
   }
   else
   {
      if (!retry && p419RetryCounter)
      {
         delete p419RetryCounter;
         p419RetryCounter = 0;
      }

      cJSON_Delete(pCommandsJson);
      delete result;
   }
}
//-----------------------------------------------------------------------------


//-----------------------------------------------------------------------------
void CRestProtocol::AsyncAcknowledgeCommand(RetryableObject* pCommand,
                                            RestListener::gwOnCommandAckCB Callback)
//-----------------------------------------------------------------------------
{
   m_OnCommandAckCallback = Callback;
   LOG3(Tools::Logger::lmInfo, "Rest {0x%X} is acknowledging Command{0x%X} with callback {0x%X}",
        this, pCommand, m_OnCommandAckCallback);

   WaitForRegistrationCompleted();
   CHttpTask* pHttpTask = WaitForFreeTask(RT_PUT);

   char CommandIdStr[32];CommandIdStr[0] = 0;
   UInt32 CommandId = 0;
   if (pCommand)
   {
      CommandId = cJSON_GetObjectItem((cJSON*)pCommand->GetObject(), "id")->valueint;
   }
   sprintf(CommandIdStr, "%d", CommandId);

   pHttpTask->AddPath("device");
   pHttpTask->AddPath(m_pGateway->GetDeviceID());
   pHttpTask->AddPath("command");
   pHttpTask->AddPath(CommandIdStr);
   pHttpTask->FinishUrl();

   pHttpTask->AddHeader(HTTP::Content_Type, "application/json");
   pHttpTask->AddHeader(HTTP::Accept, "application/json");
   pHttpTask->AddHeader(HTTP::Connection, "keep-alive");
   pHttpTask->AddHeader(HTTP::AuthDeviceID, m_pGateway->GetDeviceID());
   pHttpTask->AddHeader(HTTP::AuthDeviceKey, m_pGateway->GetDeviceMAC());   

   char* jsonStr = cJSON_PrintUnformatted((cJSON*)pCommand->GetObject());
   pHttpTask->SetContent(jsonStr);
   if (jsonStr)
   {
      delete [] jsonStr;
      jsonStr = 0;
   }

   const UInt32 cTimeoutMs = cDefaultTimeoutMs;
   char TaskName[32 + 1];
   sprintf(TaskName, "Acknowledging{0x%X}", pCommand);

   ResultDH result = m_Http->AsyncSendWithObject(pHttpTask, this,
                      (CHttpTask::TaskListener::TaskWithObjectCallback)&CRestProtocol::OnCommandAcknowledged,
                      cTimeoutMs, (void*)pCommand, TaskName);

   if (result != DH_NO_ERROR)
   {
      m_pGateway->NotifyAboutError(result);// Calls to CApplication::HandleError() method
      return;
      // You may want to call m_OnCommandAckCallback here like this:
      /*
      if (m_OnCommandAckCallback)
      {
         RestListener::gwOnCommandAckCB cb = m_OnCommandAckCallback;
         m_OnCommandAckCallback = 0;

         ((CRestProtocol::RestListener*)(m_pGateway)->*cb)(result, NULL);
      }
      */

      // But be carefull with multithreading in this case. It is highly desirable
      // to call this callback from within a dedicated thread
      // Also pay attantion to a  callback parameter value (NULL, in this case)
      // This approach needs to be carefully tested!!!
   }
}


//-----------------------------------------------------------------------------
void CRestProtocol::OnCommandAcknowledged(ResultDH result, CHttpTask* task,
                                          RetryableObject* pCommand)
//-----------------------------------------------------------------------------
{
   LOG(Tools::Logger::lmInfo, "OnCommandAcknowledged");
   bool retry = false;

   if (result == DH_NO_ERROR)
   {
      if (task->ResponseIsComplited() && task->HttpStatusIsSuccessfull())
      {
         LOG3(Tools::Logger::lmInfo, "Rest{0x%X} has successfully acknowledged Command{0x%X}, Task{0x%X}",
              this, pCommand, task);
      }
      else if (!task->ResponseIsComplited())
      {
         LOG3(Tools::Logger::lmError, "Failed to acknowledge Command{0x%X}, Task{0x%X}, ResponseState: %d",
              pCommand, task, task->GetResponseState());

         result = FormatError(task->GetResponseState(), ErrorDH::ET_InvalidResponse,
                              ErrorDH::EP_RestOnAcknowledged,
                              "Failed to acknowledge Command: Invalid response");
      }
      else
      {
         LOG3(Tools::Logger::lmError, "Failed to acknowledge Command{0x%X}, Task{0x%X} HttpStatusCode: %d",
              pCommand, task, task->GetHttpStatus());

         result = FormatError(task->GetHttpStatus(), ErrorDH::ET_HttpStatus,
                              ErrorDH::EP_RestOnAcknowledged,
                              "Failed to acknowledge Command: Invalid HTTP status");
      }
   }
   else
   {
      LOG2(Tools::Logger::lmError, "Failed to acknowledge Command{0x%X}, Task{0x%X}. Connection error",
           pCommand, task);
   }

   ReleaseTask(task);

   if (!retry && m_OnCommandAckCallback)
   {
         LOG3(Tools::Logger::lmInfo, "Rest{0x%X} has acknowledged Command{0x%X} and calling callback {0x%X}",
              this, pCommand, m_OnCommandAckCallback);

         RestListener::gwOnCommandAckCB cb = m_OnCommandAckCallback;
         m_OnCommandAckCallback = 0;
         ((CRestProtocol::RestListener*)(m_pGateway)->*cb)(result, pCommand);
   }
   else
   {
      if (!retry && pCommand)
      {
         delete pCommand;
         pCommand = 0;
      }

      delete result;
   }
}
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
void CRestProtocol::AsyncSendNotification(const RetryableObject* pNotification,
                                          RestListener::gwOnNotifyCB Callback)
//-----------------------------------------------------------------------------
{
   m_OnNotifyCallback = Callback;
   LOG3(Tools::Logger::lmInfo, "Rest {0x%X} is sending Notification{0x%X} with callback {0x%X}",
        this, pNotification, m_OnPollCommandsCallback);

   WaitForRegistrationCompleted();
   CHttpTask* pHttpTask = WaitForFreeTask(RT_POST);

   pHttpTask->AddPath("device");
   pHttpTask->AddPath(m_pGateway->GetDeviceID());
   pHttpTask->AddPath("notification");

   pHttpTask->FinishUrl();

   pHttpTask->AddHeader(HTTP::Content_Type, "application/json");
   pHttpTask->AddHeader(HTTP::Accept, "application/json");
   pHttpTask->AddHeader(HTTP::Connection, "keep-alive");
   pHttpTask->AddHeader(HTTP::AuthDeviceID, m_pGateway->GetDeviceID());
   pHttpTask->AddHeader(HTTP::AuthDeviceKey, m_pGateway->GetDeviceMAC());   

   cJSON* jNotification = (cJSON*)pNotification->GetObject();

   if (jNotification)
   {
      char* jsonStr = cJSON_PrintUnformatted((cJSON*)jNotification);
      pHttpTask->SetContent(jsonStr);
      if (jsonStr)
      {
         delete [] jsonStr;
         jsonStr = 0;
      }
   }

   const UInt32 cTimeoutMs = cDefaultTimeoutMs;
   char TaskName[32 + 1];
   sprintf(TaskName, "Sending Notif{0x%X}", pNotification);

   ResultDH result = m_Http->AsyncSendWithObject(pHttpTask, this,
                         (CHttpTask::TaskListener::TaskWithObjectCallback)&CRestProtocol::OnNotify,
                         cTimeoutMs, (void*)pNotification, TaskName);

   if (result != DH_NO_ERROR)
   {
      m_pGateway->NotifyAboutError(result);// Calls to CApplication::HandleError() method
      return;
      // You may want to call m_OnNotifyCallback here like this:
      /*
      if (m_OnNotifyCallback)
      {
         RestListener::gwOnNotifyCB cb = m_OnNotifyCallback;
         m_OnNotifyCallback = 0;

         ((CRestProtocol::RestListener*)(m_pGateway)->*cb)(result, NULL, NULL);
      }
      */

      // But be carefull with multithreading in this case. It is highly desirable
      // to call this callback from within a dedicated thread
      // Also pay attantion to a  callback parameter value (NULLs, in this case)
      // This approach needs to be carefully tested!!!
   }
}

//-----------------------------------------------------------------------------
void CRestProtocol::OnNotify(ResultDH result, CHttpTask* task, RetryableObject* pSentNotif)
//-----------------------------------------------------------------------------
{
   LOG(Tools::Logger::lmInfo, "OnNotify");
   cJSON* pServerNotification = 0;
   bool retry = false;

   if (result == DH_NO_ERROR)
   {
      if (task->ResponseIsComplited() && task->HttpStatusIsSuccessfull())
      {
         LOG2(Tools::Logger::lmInfo, "A Notif{0x%X} has been sent successfully, Task{0x%X}", pSentNotif, task);
         char* pContent = (char*)task->GetResponseContent();
         if(pContent != 0)
         {
            pServerNotification = cJSON_Parse(pContent);
         }
         else
         {
            // Since I do not know what to do with this notifcation, I do not rise an error here
         }
      }
      else if (!task->ResponseIsComplited())
      {
         LOG3(Tools::Logger::lmError, "Failed to send Notif{0x%X}, Task{0x%X}}, ResponseState: %d",
              pSentNotif, task, task->GetResponseState());

         result = FormatError(task->GetResponseState(), ErrorDH::ET_InvalidResponse,
                              ErrorDH::EP_RestOnNoify,
                              "Failed to get commands: Invalid response");
      }
      else
      {
         LOG3(Tools::Logger::lmError, "Failed to send Notif{0x%X}, Task{0x%X}, HttpStatusCode: %d",
              task, pSentNotif, task->GetHttpStatus());

         result = FormatError(task->GetHttpStatus(), ErrorDH::ET_HttpStatus,
                              ErrorDH::EP_RestOnNoify,
                              "Failed to send a Notif: Invalid HTTP status");
      }
   }
   else
   {
      LOG2(Tools::Logger::lmError, "Failed to send Notif{0x%X}, Task{0x%X}. Connection error",
           pSentNotif, task);
   }

   ReleaseTask(task);

   if (!retry &&m_OnNotifyCallback)
   {
      LOG3(Tools::Logger::lmInfo, "Rest {0x%X} has sent a Notif{0x%X} and calling callback {0x%X}",
           this, pSentNotif, m_OnNotifyCallback);

      RestListener::gwOnNotifyCB cb = m_OnNotifyCallback;
      m_OnNotifyCallback = 0;
      ((CRestProtocol::RestListener*)(m_pGateway)->*cb)(result, pServerNotification, pSentNotif);
   }
   else
   {
      if(!retry && pSentNotif)
      {
         delete pSentNotif;
      }

      cJSON_Delete(pServerNotification);
      delete result;
   }
}
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
void CRestProtocol::WaitForRegistrationCompleted()
//-----------------------------------------------------------------------------
{
   if (m_RegistrationIsInProgress)
   {
      LOG1(Tools::Logger::lmInfo, "Rest{0x%X}: Registration is in process. Waiting.", this);
      if (!m_RegistrationCompletedEvent.Wait(cRegistrationTime))
      {
         // TODO: Pass an error notification to the APP.
         // Anyway, moving forvard, some pearticular error will be risen
         // (by server, probably) and will be escalated to the App
         ResultDH result = FormatError(ErrorDH::EC_GaveUpWaitingForRegistration,
                                       ErrorDH::ET_AppSpecific,
                                       ErrorDH::EP_RestWaitRegistrationComplete,
                                       "Given up waiting for registration complete");

         m_pGateway->NotifyAboutError(result);// Calls to CApplication::HandleError() method
         LOG1(Tools::Logger::lmWarning, "Rest{0x%X: given up waiting for registration completed", this);
      }
      else
      {
         LOG1(Tools::Logger::lmInfo, "Rest{0x%X}: Registration seems to be completed", this);
      }
   }
}

//-----------------------------------------------------------------------------
CHttpTask* CRestProtocol::WaitForFreeTask(ReqTypeEnum Type)
//-----------------------------------------------------------------------------
{
   CHttpTask* pHttpTask = 0;

   do
   {
      switch (Type)
      {
         case RT_POST:
            pHttpTask = m_Http->NewPOST(m_BaseUrl);
            break;
         case RT_GET:
            pHttpTask = m_Http->NewGET(m_BaseUrl);
            break;
         case RT_PUT:
            pHttpTask = m_Http->NewPUT(m_BaseUrl);
            break;
         case RT_DELETE:
            pHttpTask = m_Http->NewDELETE(m_BaseUrl);
            break;
      }

      if (!pHttpTask)
      {
         LOG1(Tools::Logger::lmInfo, "Rest{0x%X}: No free task. Waiting.", this);
         if (!m_FreeTaskEvent.Wait(cTaskFreeTime))
         {
            LOG1(Tools::Logger::lmWarning, "Rest{0x%X}: gave up waiting for free task. Will wait again.", this);
         }
         else
         {
            LOG1(Tools::Logger::lmInfo, "Rest{0x%X}: Got free task!", this);
         }
      }
   }while (!pHttpTask);

   LOG2(Tools::Logger::lmInfo, "Rest{0x%X}: Got free Task{0x%X}!", this, pHttpTask);
   return pHttpTask;
}

//-----------------------------------------------------------------------------
void CRestProtocol::ReleaseTask(CHttpTask* task)
//-----------------------------------------------------------------------------
{
   if(m_Http->ReleaseTask(task))
      m_FreeTaskEvent.Set();
}

//-----------------------------------------------------------------------------
void CRestProtocol::Stop()
//-----------------------------------------------------------------------------
{
   LOG1(Tools::Logger::lmInfo, "Rest{0x%X}: Stopping all activity", this);
   if(m_Http)
   {
      m_Http->Stop();
   }
}