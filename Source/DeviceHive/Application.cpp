/*-----------------------------------------------------------------------------

Autor                                                           Datum
Nikolay Peganov <Nikolay.Peganov@dataart.com>                   17.11.2014

-- BESCHREIBUNG ---------------------------------------------------------------


-- AENDERUNGEN ----------------------------------------------------------------

Autor                  Datum

-----------------------------------------------------------------------------*/
extern "C"
{
#include "TestPin_think.h"
}

#include "RetryableObject.h"
#include "Application.h"

using namespace DH;

CApplication* CApplication::m_pSingletone = 0;
bool CApplication::m_Terminated = false;

CApplication* CApplication::GetInstance()
{
   if (0 == m_pSingletone)
   {  // Create instance
      m_pSingletone = new CApplication;
   }

   // deliver reference to instance
   return m_pSingletone;
}

// Construction / Destruction
//-----------------------------------------------------------------------------
CApplication::CApplication()
: m_ButtonChecker(CButtonChecker(this))
, m_ButtonCheckerStarted(false)
, m_pGateway(0)
//-----------------------------------------------------------------------------
{
   strcpy(cServerUrl, "http://nnXXXX.pg.devicehive.com/api");
   if (strstr(cServerUrl, "nnXXXX.pg.devicehive.com"))
   {
      LOG(Tools::Logger::lmInfo, "HALTING!");          
      LOG(Tools::Logger::lmInfo, "Please, visit");
      LOG(Tools::Logger::lmInfo, "\"http://devicehive.com/user/register\"");          
      LOG(Tools::Logger::lmInfo, "to create a playground or change cServerUrl to an appropriate value.");      
      while (true)
         Sleep(1000);
   }
      

}

//-----------------------------------------------------------------------------
CApplication::~CApplication()
//-----------------------------------------------------------------------------
{
   if (m_pGateway)
   {
      delete m_pGateway;
      m_pGateway = 0;
   }
}

void CApplication::Start()
//-----------------------------------------------------------------------------
{
   LOG(Tools::Logger::lmInfo, "Starting the Application");
   CreateThread("ApplicationStart", m_cAppStartPrio, m_cAppStartStackSize);
}

//-----------------------------------------------------------------------------

UInt32 CApplication::Run()
//-----------------------------------------------------------------------------
{
   LOG(Tools::Logger::lmInfo, "Running the Application");
   m_pGateway = CGateway::GetInstance();
   m_pGateway->Init(this);

   m_pGateway->AsyncStart((CGateway::GatewayListener::instrOnGwStartedCB)&CApplication::OnGatewayStarted);
   return MQX_OK;
}

//-----------------------------------------------------------------------------
void CApplication::ExitInstance()
//-----------------------------------------------------------------------------
{
}

//-----------------------------------------------------------------------------
void CApplication::ExitInstance(UInt32 result)
//-----------------------------------------------------------------------------
{
   if (result == MQX_OK)
   {
      LOG(Tools::Logger::lmInfo, "The Application has been started successfully");
   }
   else
   {
      LOG(Tools::Logger::lmInfo, "The Application has failed to start");
   }
}

//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
void CApplication::HandleError(ResultDH error)
//-----------------------------------------------------------------------------
{
   if(error)
   {
      LOG1(Tools::Logger::lmError, "An error has occurred: %s", error->What());
      delete error;
   }
   else
   {
      LOG(Tools::Logger::lmWarning, "HandleError() method has been called without an error.");
   }
}

//-----------------------------------------------------------------------------
void CApplication::HandleCommand(cJSON* jCommand)
//-----------------------------------------------------------------------------
{
   // TODO: Implement Device-specific command handling here.
   // Below is just a possible example.

   // CCommandExecutor is an example of an asynchronous command executor.
   // Of course, you may handle commands synchronously (directly in HandleCommand() method),
   // but, I think, asynchronous handling is better way for doing this.

   CCommandExecutor* pExecutor = 0;
   cJSON* jCommandName = cJSON_GetObjectItem(jCommand, "command");
   LOG1(Tools::Logger::lmInfo, "Got comand %s", jCommandName->valuestring);

   if (!strncmp(jCommandName->valuestring, "RedLED", strlen("RedLED")))
   {
      pExecutor = new CRedLedExecutor();
   }
   else if (!strncmp(jCommandName->valuestring, "GreenLED", strlen("GreenLED")))
   {
      pExecutor = new CGreenLedExecutor();
   }
   else
   {
      cJSON* jStatus = cJSON_CreateString("Failed");

      cJSON* jResult = cJSON_CreateObject();
      cJSON_AddItemToObject(jResult, "Command",
                            cJSON_CreateString(jCommandName->valuestring));
      cJSON_AddItemToObject(jResult, "Status",
                            cJSON_CreateString("Command unknown"));

      cJSON_ReplaceItemInObject(jCommand, "result", jResult);
      cJSON_ReplaceItemInObject(jCommand, "status", jStatus);

      AsyncAcknowledgeCommand(jCommand);
   }

   if (pExecutor)
   {
      pExecutor->AsyncExecute(this, (CCommandExecutor::ExecutorListener::ExecutionCallback)&CApplication::OnCommandExecuted, jCommand);;
   }
}

//-----------------------------------------------------------------------------
void CApplication::AsyncSendNotification(const char* pNotificationName,
                                         cJSON* jNotificationData)
//-----------------------------------------------------------------------------
{
   cJSON* jNotification = cJSON_CreateObject();
   cJSON_AddItemToObject(jNotification, "notification",
                         cJSON_CreateString(pNotificationName));
   cJSON_AddItemToObject(jNotification, "parameters", jNotificationData);

   char TimeStamp[32 + 1];
   m_pGateway->GetTimeStamp(TimeStamp);
   cJSON_AddItemToObject(jNotification, "localTimestamp", cJSON_CreateString(TimeStamp));

   // You may use cJSON_Duplicate to copy jNotificationData into jNotification
   // and release jNotificationData right here or after the call to
   // Application::AsyncSendNotification() method.

   RetryableObject* pNotification = new RetryableObject(jNotification);

   m_pGateway->AsyncSendNotification(pNotification,
      (CGateway::GatewayListener::instrOnNotifyCB)&CApplication::OnNotificationSent);
}


//-----------------------------------------------------------------------------
void CApplication::AsyncAcknowledgeCommand(cJSON* jCommand)
//-----------------------------------------------------------------------------
{
   RetryableObject* pCommand = new RetryableObject(jCommand);
   LOG1(Tools::Logger::lmInfo, "Acknowledging a Command{0x%X}", pCommand);

   m_pGateway->AsyncAcknowledgeCommand(pCommand,
      (CGateway::GatewayListener::instrOnCommandAckCB)&CApplication::OnCommandAcknowledged);
}

//-----------------------------------------------------------------------------
void CApplication::OnNotificationSent(ResultDH result, cJSON* jServerNotification,
                                      RetryableObject* pSentNotification)
//-----------------------------------------------------------------------------
{
   // jServerNotification is a copy of a notification, created on the server.
   // I do not know what will you do with jServerNotification object, so, releasing it right here.
   cJSON_Delete(jServerNotification);

   if (result == DH_NO_ERROR)
   {
      LOG1(Tools::Logger::lmInfo, "The App has sucessfully sent a Notification{0x%X}!",
          pSentNotification);

      if (pSentNotification)
      {
         delete pSentNotification;
      }
   }
   else
   {
      bool isInterrupted = result->Interrupted();
      LOG1(Tools::Logger::lmInfo, "Failed to sent a Notification{0x%X}.", pSentNotification);
      HandleError(result);  // deletes result
      
      if (!isInterrupted) // retry
      {
         // You can make several additional attempts to send a notification before giving up
         if (pSentNotification)
         {
            UInt8 RetryCount = pSentNotification->GetRetryCount();
            if (RetryCount++ < cRetryFailCount)
            {
               pSentNotification->SetRetryCount(RetryCount);
               Sleep(1000); // TODO: make the delayed call asynchronously

               LOG2(Tools::Logger::lmInfo, "Sending a Notification{0x%X}, Attempt: %d",
                    pSentNotification, RetryCount + 1);

               m_pGateway->AsyncSendNotification(pSentNotification,
                                                 (CGateway::GatewayListener::instrOnNotifyCB)&CApplication::OnNotificationSent);
            }
            else
            {
               delete pSentNotification;
            }
         }
      }
      else
      {
         LOG1(Tools::Logger::lmInfo, "Failed to sent a Notification{0x%X}: interrupted",
              pSentNotification);

         if (pSentNotification)
         {
            delete pSentNotification;
         }
      }
   }
}


//-----------------------------------------------------------------------------
void CApplication::OnGatewayStarted(ResultDH result)
//-----------------------------------------------------------------------------
{
   if (result == DH_NO_ERROR)
   {
      LOG(Tools::Logger::lmInfo, "The Gateway has sucessfully started!");
      m_pGateway->AsyncGetServerInfo((CGateway::GatewayListener::instrOnGotServerInfoCB)&CApplication::OnGotSerevrInfo);
   }
   else
   {
      LOG(Tools::Logger::lmInfo, "The Gateway has failed to start");
      HandleError(result);
      Sleep(1000); // TODO: make the delayed call asynchronously
      m_pGateway->AsyncStart((CGateway::GatewayListener::instrOnGwStartedCB)&CApplication::OnGotSerevrInfo);
   }
}

//-----------------------------------------------------------------------------
void CApplication::OnGotSerevrInfo(ResultDH result)
//-----------------------------------------------------------------------------
{
   if (result == DH_NO_ERROR)
   {
      LOG(Tools::Logger::lmInfo, "The Gateway has sucessfully got ServerInfo!");
      m_pGateway->AsyncRegister((CGateway::GatewayListener::instrOnRegisterCB)&CApplication::OnRegistered);
   }
   else
   {
      bool isInterrupted = result->Interrupted();
      LOG(Tools::Logger::lmInfo, "The Gateway has failed to get server info.");
      HandleError(result);

      if (!isInterrupted) // retry
      {
         Sleep(1000 * 1);// TODO: make the delayed call asynchronously
         // You can change this time to 1 minute according to FS-ID 5052
         m_pGateway->AsyncGetServerInfo((CGateway::GatewayListener::instrOnGotServerInfoCB)&CApplication::OnGotSerevrInfo);
      }
      else
      {
         LOG(Tools::Logger::lmInfo, "OnGotSerevrInfo: interrupted.");
      }
   }
}

//-----------------------------------------------------------------------------
void CApplication::OnRegistered(ResultDH result, cJSON* jRegistrationInfo)
//-----------------------------------------------------------------------------
{
   if (result == DH_NO_ERROR)
   {
      LOG(Tools::Logger::lmInfo, "The Device has been successfully registered!");

      m_pGateway->AsyncPollCommands(m_pGateway->GetLastCommandTimestamp());

      if (!m_ButtonCheckerStarted)
      {
         m_ButtonChecker.CreateThread("ButtonChecker", 10, 3000);
         m_ButtonCheckerStarted = true;
      }
   }
   else
   {
      bool isInterrupted = result->Interrupted();
      LOG(Tools::Logger::lmInfo, "Failed to register the Device.");
      HandleError(result);

      if (!isInterrupted) // retry
      {
         Sleep(1000 * 1);// TODO: make the delayed call asynchronously
         // You can change this time to 1 minute according to FS-ID 5052
         m_pGateway->AsyncGetServerInfo((CGateway::GatewayListener::instrOnGotServerInfoCB)&CApplication::OnGotSerevrInfo);
      }
      else
      {
         LOG(Tools::Logger::lmInfo, "OnRegistered: interrupted.");
      }
   }

   cJSON_Delete(jRegistrationInfo);
}

//-----------------------------------------------------------------------------
void CApplication::OnCommandExecuted(UInt32 result, cJSON* jCommand, CCommandExecutor* pExecutor)
//-----------------------------------------------------------------------------
{
   if (result == MQX_OK)
   {
      LOG(Tools::Logger::lmInfo, "A command has been executed successfully.");
   }
   else
   {
      LOG1(Tools::Logger::lmInfo, "Failed to execute command, code: %d", result);
   }

   if (pExecutor)
   {
      delete pExecutor;
      pExecutor = 0;
   }

   AsyncAcknowledgeCommand(jCommand);

   // WARN: Please, note, that this jCommand object is normally deletted in CApplication::OnCommandAcknowledged
   // (or CGateway::OnCommandAcknowledged or CRestProtocol::OnCommandAcknowledged
   // in the case if no callbacks were provided).
}

//-----------------------------------------------------------------------------
void CApplication::OnCommandAcknowledged(ResultDH result, RetryableObject* pCommand)
//-----------------------------------------------------------------------------
{
   LOG(Tools::Logger::lmInfo, "OnCommandAcknowledged");
   if (result == DH_NO_ERROR)
   {
      LOG2(Tools::Logger::lmInfo, "CApplication{0x%X} has successfully acknowledged Command{0x%X}",
           this, pCommand);
      delete pCommand;
   }
   else
   {
      bool isInterrupted = result->Interrupted();
      LOG1(Tools::Logger::lmError, "Failed to acknowledge Command{0x%X}.", pCommand);
      HandleError(result);

      if (!isInterrupted) // retry
      {
         if (pCommand)
         {
            UInt8 RetryCount = pCommand->GetRetryCount();
            if (RetryCount++ < cRetryFailCount)
            {
               pCommand->SetRetryCount(RetryCount);
               Sleep(1 * 1000); // TODO: make the delayed call asynchronously

               LOG2(Tools::Logger::lmInfo, "Acknowledging a Command{0x%X}, Attempt: %d",
                    pCommand, RetryCount + 1);

               m_pGateway->AsyncAcknowledgeCommand(pCommand,
                  (CGateway::GatewayListener::instrOnCommandAckCB)&CApplication::OnCommandAcknowledged);
            }
            else
            {
               delete pCommand;
            }
         }
      }
      else
      {
         LOG1(Tools::Logger::lmInfo, "Failed to acknowledge a Command{0x%X}: interrupted",
              pCommand);

         if (pCommand)
         {
            delete pCommand;
         }
      }
   }
}

//-----------------------------------------------------------------------------
bool CApplication::GetMacAddress(UInt8* macDest)
//-----------------------------------------------------------------------------
{
   macDest[0] = 0xAA;
   macDest[1] = 0xBB;
   macDest[2] = 0xCC;
   macDest[3] = 0xDD;
   macDest[4] = 0xEE;
   macDest[5] = 0xFF;
   
   return true;
}

//-----------------------------------------------------------------------------
bool CApplication::GetSerial(char* serialDest, UInt8 destCapacity)
//-----------------------------------------------------------------------------
{
   Tools::OS::CopyBytes((PVOID)"4e2b005e-AAAA-BBBB-CCCC-19c085d755c1", serialDest, destCapacity);
   return true;
}

//-----------------------------------------------------------------------------
const char* CApplication::GetServerUrl()
//-----------------------------------------------------------------------------
{
   return cServerUrl;
}

//-----------------------------------------------------------------------------
UInt32 CApplication::CRedLedExecutor::Run()
//-----------------------------------------------------------------------------
{
   LOG(Tools::Logger::lmInfo, "Running RedLed executor.");
   cJSON* jCommandParams = cJSON_GetObjectItem(m_jCommand, "parameters");
   cJSON* jOperation = cJSON_GetObjectItem(jCommandParams, "operation");

   cJSON* jResult = cJSON_CreateObject();
   cJSON* jStatus = cJSON_CreateObject();

   if (jOperation)
   {
      if (!strncmp(jOperation->valuestring, "on", strlen("on")))
      {
         SetLED_Err();
         cJSON_AddItemToObject(jResult, "NewLEDState",
                               cJSON_CreateString(jOperation->valuestring));
         jStatus = cJSON_CreateString("Success");
      }
      else if (!strncmp(jOperation->valuestring, "off", strlen("off")))
      {
         ClearLED_Err();
         cJSON_AddItemToObject(jResult, "NewLEDState",
                               cJSON_CreateString(jOperation->valuestring));
         jStatus = cJSON_CreateString("Success");
      }
      else if (!strncmp(jOperation->valuestring, "toggle", strlen("toggle")))
      {
         ToggleLED_Err();
         jResult = cJSON_CreateString("Red LED has been toggled");
         jStatus = cJSON_CreateString("Success");
      }
      else
      {
         cJSON_AddItemToObject(jResult, "Operation",
                               cJSON_CreateString(jOperation->valuestring));
         cJSON_AddItemToObject(jResult, "Status",
                               cJSON_CreateString("Not supported"));

         jStatus = cJSON_CreateString("Failed");
      }
   }
   else
   {
      jResult = cJSON_CreateString("UnknownParameter");
      jStatus = cJSON_CreateString("Failed");
   }

   cJSON_ReplaceItemInObject(m_jCommand, "result", jResult);
   cJSON_ReplaceItemInObject(m_jCommand, "status", jStatus);

   return MQX_OK;
}

//-----------------------------------------------------------------------------
UInt32 CApplication::CGreenLedExecutor::Run()
//-----------------------------------------------------------------------------
{
   LOG(Tools::Logger::lmInfo, "Running GreenLed executor.");
   cJSON* jCommandParams = cJSON_GetObjectItem(m_jCommand, "parameters");
   cJSON* jOperation = cJSON_GetObjectItem(jCommandParams, "operation");

   cJSON* jResult = cJSON_CreateObject();
   cJSON* jStatus = cJSON_CreateObject();

   if (jOperation)
   {
      if (!strncmp(jOperation->valuestring, "on", strlen("on")))
      {
         SetLED_FW();
         cJSON_AddItemToObject(jResult, "NewLEDState",
                               cJSON_CreateString(jOperation->valuestring));
         jStatus = cJSON_CreateString("Success");
      }
      else if (!strncmp(jOperation->valuestring, "off", strlen("off")))
      {
         ClearLED_FW();
         cJSON_AddItemToObject(jResult, "NewLEDState",
                               cJSON_CreateString(jOperation->valuestring));
         jStatus = cJSON_CreateString("Success");
      }
      else if (!strncmp(jOperation->valuestring, "toggle", strlen("toggle")))
      {
         ToggleLED_FW();
         jResult = cJSON_CreateString("Green LED has been toggled");
         jStatus = cJSON_CreateString("Success");
      }
      else
      {
         cJSON_AddItemToObject(jResult, "Operation",
                               cJSON_CreateString(jOperation->valuestring));
         cJSON_AddItemToObject(jResult, "Status",
                               cJSON_CreateString("Not supported"));

         jStatus = cJSON_CreateString("Failed");
      }
   }
   else
   {
      jResult = cJSON_CreateString("UnknownParameter");
      jStatus = cJSON_CreateString("Failed");
   }

   cJSON_ReplaceItemInObject(m_jCommand, "result", jResult);
   cJSON_ReplaceItemInObject(m_jCommand, "status", jStatus);

   return MQX_OK;
}

//-----------------------------------------------------------------------------
CApplication::CButtonChecker::CButtonChecker(CApplication* pApp)
//-----------------------------------------------------------------------------
: m_pApp(pApp)
{}

//-----------------------------------------------------------------------------
UInt32 CApplication::CButtonChecker::Run()
//-----------------------------------------------------------------------------
{
   do
   {
      Sleep(50);
      if (GetPushButton())
      {
         if (1)
         {
            cJSON* jNotificationData = cJSON_CreateObject();
            cJSON_AddItemToObject(jNotificationData, "Event",
                                  cJSON_CreateString("Pushed"));

            m_pApp->AsyncSendNotification("ButtonNotification", jNotificationData);
            Sleep(1000);
         }
      }
   }while(!m_Terminated);

   return MQX_OK;
}
