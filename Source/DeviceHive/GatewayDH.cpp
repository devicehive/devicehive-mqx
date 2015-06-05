/*-----------------------------------------------------------------------------

Autor                                                           Datum
Nikolay Peganov <Nikolay.Peganov@dataart.com>                   17.11.2014

-- BESCHREIBUNG ---------------------------------------------------------------


-- AENDERUNGEN ----------------------------------------------------------------

Autor                  Datum

-----------------------------------------------------------------------------*/
#include "MQX.h"
#include "rtcs.h"

#include <cyassl/ssl.h>

extern "C"
{
#include <ipcfg.h>
}

#include "Application.h"
#include "GatewayDH.h"
using namespace DH;

CGateway* CGateway::m_pSingletone = 0;

//-----------------------------------------------------------------------------
CGateway* CGateway::GetInstance()
//-----------------------------------------------------------------------------
{
   if (0 == m_pSingletone)
   {  // Create instance
      m_pSingletone = new CGateway;
   }

   // deliver reference to instance
   return m_pSingletone;
}

//-----------------------------------------------------------------------------
CGateway::CGateway()
//-----------------------------------------------------------------------------
: m_pApp(0)
, m_AppGwStartCallback(0)
, m_AppGotServerInfoCallback(0)
, m_InRegisterCallback(0)
, m_InCommandAckCallback(0)
, m_InNotifyCallback(0)

, m_pServerUrl(0)

, m_Started(false)
, m_PollingIsStarted(false)

, m_ipAdr(IPADDR(192, 168, 10, 10))
, m_ipMask(IPADDR(192, 168, 10,  1))
, m_ipGateway(IPADDR(192, 168, 10,  1))

, m_DHCPMode(Enable)

, m_NotificationIsInProgress(false)
, m_AcknowledgementIsInProgress(false)   

, m_RestProto(CRestProtocol::GetInstance())
//-----------------------------------------------------------------------------
{
   _mem_zero(m_LastCommandTS, cLastCommandTSLen);
   
   m_NotificationCompletedEvent.Create();   
   m_AcknowledgementCompletedEvent.Create();   
}

//-----------------------------------------------------------------------------
void CGateway::Init(CApplication* pApp, DHCPModeEnum DHCPMode)
//-----------------------------------------------------------------------------
{
   m_pApp = pApp;
   m_DHCPMode = DHCPMode;
}

//-----------------------------------------------------------------------------
CGateway::~CGateway()
//-----------------------------------------------------------------------------
{
   if (m_RestProto)
   {
      delete m_RestProto;
      m_RestProto = 0;
   }

   if (m_pServerUrl)
   {
      delete [] m_pServerUrl;
      m_pServerUrl = 0;
   }
}

void CGateway::AsyncStart(GatewayListener::instrOnGwStartedCB gwStartCallback)
//-----------------------------------------------------------------------------
{
   LOG(Tools::Logger::lmInfo, "Starting the Gateway");
   m_AppGwStartCallback = gwStartCallback;
   CreateThread("GatewayStart", m_cGwStartPrio, m_cGwStartStackSize);
}

void CGateway::AsyncGetServerInfo(GatewayListener::instrOnGotServerInfoCB gwServerInfoCallback)
//-----------------------------------------------------------------------------
{
   LOG1(Tools::Logger::lmTrace, "Gateway{0x%X} is getting ServerInfo", this);
   m_AppGotServerInfoCallback = gwServerInfoCallback;
   m_RestProto->AsyncGetServerInfo((CRestProtocol::RestListener::gwOnServerInfoCB)&CGateway::OnGotServerInfo);
}

//-----------------------------------------------------------------------------
void CGateway::AsyncRegister(GatewayListener::instrOnRegisterCB inRegisterCallback)
//-----------------------------------------------------------------------------
{
   // Here is an example of the Device meta data. For now it is just dummy data,
   // except of MAC address and Serial number, which are taken from the Application layer

   LOG(Tools::Logger::lmInfo, "Registering the Device");
   m_InRegisterCallback = inRegisterCallback;

/* Here is the structure (see http://devicehive.com/restful#Reference/Device)
   please note, that most of the fields are optional and can be omitted!!!
{
    "id": {string},
    "key": {string},
    "name": {string},
    "status": {string},
    "data": {object},
    "network": {
        "id": {integer},
        "key": {string},
        "name": {string},
        "description": {string}
    },
    "deviceClass": {
        "id": {integer},
        "name": {string},
        "version": {string},
        "isPermanent": {boolean},
        "offlineTimeout": {integer},
        "data": {object},
        "equipment": [
            {
                "id": {integer},
                "name": {string},
                "code": {string},
                "type": {string},
                "data": {object}
            }
        ]
    }
    "timestamp": {timestamp}:
}
*/

   // Most of the fields are optional and can be omitted!!!
   cJSON *jDevice = cJSON_CreateObject();

   cJSON *jDeviceNetwork = cJSON_CreateObject();
   cJSON *jDeviceClass = cJSON_CreateObject();
   //cJSON *jDeviceEquipment = cJSON_CreateArray();
   //cJSON *jEquipment1 = cJSON_CreateObject();
   //cJSON *jEquipment2 = cJSON_CreateObject();
   cJSON *jDeviceData = cJSON_CreateObject();
   cJSON *jClassData = cJSON_CreateObject();
   //cJSON *jEquipmentData = cJSON_CreateObject();

   /*
   cJSON_AddItemToObject(jEquipmentData,"SomeDataIsHere",cJSON_CreateString("SomeEquipmentData"));

   cJSON_AddItemToObject(jEquipment1,"id",cJSON_CreateNumber(1001));
   cJSON_AddItemToObject(jEquipment1,"name",cJSON_CreateString("Equipment1"));
   cJSON_AddItemToObject(jEquipment1,"code",cJSON_CreateString("code1"));
   cJSON_AddItemToObject(jEquipment1,"type",cJSON_CreateString("Equipment type1"));
   cJSON_AddItemToObject(jEquipment1,"data",jEquipmentData);

   cJSON_AddItemToObject(jEquipment2,"id",cJSON_CreateNumber(1002));
   cJSON_AddItemToObject(jEquipment2,"name",cJSON_CreateString("Equipment2"));
   cJSON_AddItemToObject(jEquipment2,"code",cJSON_CreateString("code2"));
   cJSON_AddItemToObject(jEquipment2,"type",cJSON_CreateString("Equipment type2"));
   cJSON_AddItemToObject(jEquipment2,"data",jEquipmentData);

   cJSON_AddItemToArray(jDeviceEquipment, jEquipment1);
   cJSON_AddItemToArray(jDeviceEquipment, jEquipment2);
   */

   cJSON_AddItemToObject(jClassData,"SomeDataIsHere",cJSON_CreateString("SomeClassData"));
   cJSON_AddItemToObject(jDeviceClass,"id",cJSON_CreateNumber(2001));
   cJSON_AddItemToObject(jDeviceClass,"name",cJSON_CreateString("Device class"));
   cJSON_AddItemToObject(jDeviceClass,"version",cJSON_CreateString("0.01"));
   cJSON_AddItemToObject(jDeviceClass,"isPermanent",cJSON_CreateBool(1));
   cJSON_AddItemToObject(jDeviceClass,"offlineTimeout",cJSON_CreateNumber(60 * 30));
   cJSON_AddItemToObject(jDeviceClass,"data",jClassData);
   //cJSON_AddItemToObject(jDeviceClass,"equipment",jDeviceEquipment);

   cJSON_AddItemToObject(jDeviceNetwork,"id",cJSON_CreateNumber(1003));
   cJSON_AddItemToObject(jDeviceNetwork,"key",cJSON_CreateString("NetworkKey"));
   cJSON_AddItemToObject(jDeviceNetwork,"name",cJSON_CreateString("Network"));
   cJSON_AddItemToObject(jDeviceNetwork,"description",cJSON_CreateString("Device Network"));

   cJSON_AddItemToObject(jDeviceData,"SomeDataIsHere",cJSON_CreateString("SomeDeviceData"));
   cJSON_AddItemToObject(jDevice,"id", cJSON_CreateString(m_Device.m_Serial));                  // <=== Serial
   cJSON_AddItemToObject(jDevice,"key", cJSON_CreateString(m_Device.m_MacAdressStr));           // <=== MAC
   cJSON_AddItemToObject(jDevice,"name",cJSON_CreateString("MQX DEVICE"));
   cJSON_AddItemToObject(jDevice,"status",cJSON_CreateString("Online"));
   cJSON_AddItemToObject(jDevice,"data",jDeviceData);
   cJSON_AddItemToObject(jDevice,"network",jDeviceNetwork);
   cJSON_AddItemToObject(jDevice,"deviceClass",jDeviceClass);

   char TimeStamp[32 + 1];
   GetTimeStamp(TimeStamp);
   cJSON_AddItemToObject(jDevice, "timestamp", cJSON_CreateString(TimeStamp));                      // <=== TimeStamp

   m_RestProto->AsyncRegister(jDevice, (CRestProtocol::RestListener::gwOnRegisterCB)&CGateway::OnRegistered);

   cJSON_Delete(jDevice);
}

//-----------------------------------------------------------------------------
void CGateway::AsyncPollCommands(const char* pLastCommandTimestamp)// Poll commands
//-----------------------------------------------------------------------------
{
   if (!m_PollingIsStarted)
   {
      LOG(Tools::Logger::lmTrace, "Commands polling IS NOT started, got it started!");
      m_PollingIsStarted = true;
   }

   LOG1(Tools::Logger::lmInfo, "Gateway {0x%X} is polling commands", this);
   m_RestProto->AsyncPollCommands(pLastCommandTimestamp,
                                  (CRestProtocol::RestListener::gwOnPollCommandsCB)&CGateway::OnPollCommands);
}

//-----------------------------------------------------------------------------
void CGateway::AsyncAcknowledgeCommand(RetryableObject* pCommand,
                                       GatewayListener::instrOnCommandAckCB inCommandAckCallback)// Acknowledge command
//-----------------------------------------------------------------------------
{
   if (m_AcknowledgementIsInProgress)
   {
      if(!m_AcknowledgementCompletedEvent.Wait(m_RestProto->GetTaskCompletionTime()))
      {
         ResultDH result = FormatError(ErrorDH::EC_GaveUpWaitingForAcknowledgement,
                                       ErrorDH::ET_AppSpecific,
                                       ErrorDH::EP_GwSendingAcknowledgement,
                                       "Given up waiting for acknowledgement completed");

         NotifyAboutError(result);// Calls to CApplication::HandleError() method
         LOG1(Tools::Logger::lmWarning, "Rest{0x%X}: given up waiting for acknowledgement completed", this);         
         return;      
      }
   }  

   m_AcknowledgementIsInProgress = true; 
   m_AcknowledgementCompletedEvent.Reset();
   
   // WARN: Please, note, that this jCommand object is normally deleted in CApplication::OnCommandAcknowledged
   // (or CGateway::OnCommandAcknowledged or CRestProtocol::OnCommandAcknowledged
   // in the case if no callbacks were provided).

   m_InCommandAckCallback = inCommandAckCallback;
   LOG3(Tools::Logger::lmInfo, "Gateway {0x%X} is acknowledging Command{0x%X} with callback {0x%X}",
        this, pCommand, m_InCommandAckCallback);

   m_RestProto->AsyncAcknowledgeCommand(pCommand,
       (CRestProtocol::RestListener::gwOnCommandAckCB)&CGateway::OnCommandAcknowledged);
}

//-----------------------------------------------------------------------------
void CGateway::AsyncSendNotification(RetryableObject* pNotification,
                              GatewayListener::instrOnNotifyCB inNotifyCallback)
//-----------------------------------------------------------------------------
{
   if (m_NotificationIsInProgress)
   {
      if(!m_NotificationCompletedEvent.Wait(m_RestProto->GetTaskCompletionTime()))
      {
         ResultDH result = FormatError(ErrorDH::EC_GaveUpWaitingForNotification,
                                       ErrorDH::ET_AppSpecific,
                                       ErrorDH::EP_GwSendingNoification,
                                       "Given up waiting for notification completed");

         NotifyAboutError(result);// Calls to CApplication::HandleError() method
         LOG1(Tools::Logger::lmWarning, "Rest{0x%X}: given up waiting for notification completed", this);         
         return;      
      }
   }  

   m_NotificationIsInProgress = true;      
   m_NotificationCompletedEvent.Reset();   

   
   // WARN: Please, note, that this jNotification object is normally deleted in CApplication::OnNotificationSent
   // (or CGateway::OnNotify or CRestProtocol::jSentNotif
   // in the case if no callbacks were provided).

   m_InNotifyCallback = inNotifyCallback;
   LOG3(Tools::Logger::lmInfo, "Gateway{0x%X} is sending a Notif{0x%X} with Callback{0x%X}",
        this, pNotification, m_InNotifyCallback);

   m_RestProto->AsyncSendNotification(pNotification,
       (CRestProtocol::RestListener::gwOnNotifyCB)&CGateway::OnNotify);
}

//-----------------------------------------------------------------------------

UInt32 CGateway::Run()
//-----------------------------------------------------------------------------
{
   LOG(Tools::Logger::lmInfo, "Running the Gateway");
   UInt8 macAdress[cMacLen];
   ResultDH result = DH_NO_ERROR;

   _mem_zero(m_Device.m_MacAdressStr, cStrMacLen);
   _mem_zero(m_Device.m_Serial, cSerialLen);

   if (m_pApp->GetMacAddress(macAdress))
   {
      sprintf(m_Device.m_MacAdressStr, "%02X%02X%02X%02X%02X%02X",
              macAdress[0],
              macAdress[1],
              macAdress[2],
              macAdress[3],
              macAdress[4],
              macAdress[5]);
      LOG1(Tools::Logger::lmTrace, "The Gateway got MAC: %s", m_Device.m_MacAdressStr);

      result = SetupNetwork(macAdress, Enable);
      if (result != DH_NO_ERROR)
        return (UInt32)result;

      if (!m_pApp->GetSerial(m_Device.m_Serial, cSerialLen))
      {
         return (UInt32)FormatError(ErrorDH::EC_GwFailedToGetSerial,
                                    ErrorDH::ET_AppSpecific, ErrorDH::EP_GwStartup,
                                    "Failed to obtain serial");
      }

      int res = CyaSSL_Init();
      if (res != SSL_SUCCESS)
      {
         return (UInt32)FormatError(res, ErrorDH::ET_SYASSL, ErrorDH::EP_GwStartup,
                                    "CyaSSL_Init() failed");
      }

      UpdateServerUrl(m_pApp->GetServerUrl());
   }
   else
   {
      return (UInt32)FormatError(ErrorDH::EC_GwFailedToGetMac,
                                 ErrorDH::ET_AppSpecific, ErrorDH::EP_GwStartup,
                                 "Failed to obtain MAC address");
   }

   return (UInt32)result;
}

//-----------------------------------------------------------------------------
void CGateway::ExitInstance()
//-----------------------------------------------------------------------------
{
}

//-----------------------------------------------------------------------------
void CGateway::UpdateServerUrl(const char* newServerUrl)
//-----------------------------------------------------------------------------
{
   if (m_pServerUrl)
   {
      delete [] m_pServerUrl;
      m_pServerUrl = 0;
   }

   m_pServerUrl = new char[strlen(newServerUrl) + 1];
   strcpy(m_pServerUrl, newServerUrl);

   if (m_RestProto)
   {
      m_RestProto->Init(this, m_pServerUrl);
   }
}

//-----------------------------------------------------------------------------
void CGateway::Stop()
//-----------------------------------------------------------------------------
{
   LOG(Tools::Logger::lmInfo, "Stopping The Gateway");
   m_Started = false;
   m_PollingIsStarted = false;

   if (m_RestProto)
   {
      m_RestProto->Stop();
   }
}


//-----------------------------------------------------------------------------
void CGateway::ExitInstance(UInt32 result)
//-----------------------------------------------------------------------------
{
   bool success = false;
   if (result == DH_NO_ERROR)
   {
      LOG(Tools::Logger::lmInfo, "The Gateway has been started successfully");
      success = true;
   }

   if(m_AppGwStartCallback)
   {
      GatewayListener::instrOnGwStartedCB cb = m_AppGwStartCallback;
      m_AppGwStartCallback = 0;

      (m_pApp->*cb)((ResultDH)result);  // App is responsible for calling AsyncGetServerInfo()
   }
   else if (success)
   {
      AsyncGetServerInfo();
   }
   else
   {
      LOG(Tools::Logger::lmError, "The Gateway has failed to start");
      m_pApp->HandleError((ResultDH)result);
   }

}// After calling _task_abort(), THIS line is unreachable!!!

//-----------------------------------------------------------------------------


//-----------------------------------------------------------------------------
void CGateway::NotifyAboutError(ResultDH error)
//-----------------------------------------------------------------------------
{
   m_pApp->HandleError(error);
}

//-----------------------------------------------------------------------------
ResultDH CGateway::SetupNetwork(UInt8* pMacAddress, DHCPModeEnum DHCPMode)
//-----------------------------------------------------------------------------
{
   ResultDH result = DH_NO_ERROR;
   IPCFG_IP_ADDRESS_DATA autoIPdata;
   _enet_address enetAddress;
   autoIPdata.ip = m_ipAdr;
   autoIPdata.mask = m_ipMask;
   autoIPdata.gateway = m_ipGateway;

   UInt32 res = RTCS_create();
   if (res != RTCS_OK)
   {
      return FormatError(res, ErrorDH::ET_RTCS, ErrorDH::EP_GwNetworkSetup,
                         "RTCS_create() failed");
   }

   if (DHCPMode == Service)
   {  // Servicemode
      DHCPMode = Disable;
      m_ipAdr = IPADDR(192, 168, 10, 10);
      m_ipMask = IPADDR(255, 255, 255, 0);
      m_ipGateway = IPADDR(192, 168, 10, 1);
   }

   //ENET_get_mac_address (BSP_DEFAULT_ENET_DEVICE, 0xF10000 + m_ipAddresMask, enetAddress);
   Tools::OS::CopyBytes(pMacAddress, enetAddress, cMacLen);
   res = ipcfg_init_device(BSP_DEFAULT_ENET_DEVICE, enetAddress);
   if (res != IPCFG_OK)
   {
      return FormatError(res, ErrorDH::ET_IPCFG, ErrorDH::EP_GwNetworkSetup,
                         "ipcfg_init_device() failed");
   }

   if (DHCPMode == Enable)
   {
      //Sleep(12000);
     Sleep(2000);
   }

   if (DHCPMode == Enable) //#ifdef NET_DHCP_ENABLE
   {
      memset(&autoIPdata, 0, sizeof(autoIPdata));
      res = ipcfg_bind_dhcp_wait(BSP_DEFAULT_ENET_DEVICE, TRUE, &autoIPdata);
   }
   else
   {
      res = ipcfg_bind_staticip(BSP_DEFAULT_ENET_DEVICE, &autoIPdata);
   }

   if (res == IPCFG_OK)
   {
      bool res;
      res = ipcfg_get_ip(BSP_DEFAULT_ENET_DEVICE, &autoIPdata);
      if (res)
      {
         LOG4(Tools::Logger::lmTrace, "Got IP: %d.%d.%d.%d",
              (autoIPdata.ip >> 24) & 0xFF,
              (autoIPdata.ip >> 16) & 0xFF,
              (autoIPdata.ip >> 8) & 0xFF,
              autoIPdata.ip & 0xFF);

         if (Enable == Enable)
         {
            m_ipAdr = autoIPdata.ip;
            m_ipGateway = autoIPdata.gateway;
            m_ipMask = autoIPdata.mask;
         }
      }
      else
      {
         return FormatError(0, ErrorDH::ET_IPCFG, ErrorDH::EP_GwNetworkSetup,
                            "ipcfg_get_ip() failed");
      }
   }
   else
   {
      return FormatError(res, ErrorDH::ET_IPCFG, ErrorDH::EP_GwNetworkSetup,
                         "ipcfg_bind_dhcp_wait() or ipcfg_bind_staticip failed");
   }

   res = DNS_init();
   if (res != IPCFG_OK)
   {
      return FormatError(res, ErrorDH::ET_RTCS, ErrorDH::EP_GwNetworkSetup,
                         "DNS_init() failed");
   }

   return result;
}


//-----------------------------------------------------------------------------
const char* CGateway::GetServerUrl()
//-----------------------------------------------------------------------------
{
   return m_pServerUrl;
}

//-----------------------------------------------------------------------------
const char* CGateway::GetDeviceID()
//-----------------------------------------------------------------------------
{
   return m_Device.m_Serial;
}

//-----------------------------------------------------------------------------
const char* CGateway::GetDeviceMAC()
//-----------------------------------------------------------------------------
{
   return m_Device.m_MacAdressStr;
}

//-----------------------------------------------------------------------------
const char* CGateway::GetLastCommandTimestamp()
//-----------------------------------------------------------------------------
{
   return m_LastCommandTS;
}

//-----------------------------------------------------------------------------
void CGateway::OnGotServerInfo(ResultDH result, cJSON* jServerInfo)
//-----------------------------------------------------------------------------
{
   LOG(Tools::Logger::lmInfo, "OnGotServerInfo");
   bool success = false;
   if (result == DH_NO_ERROR)
   {
      LOG(Tools::Logger::lmInfo, "Got ServerInfo");

      cJSON* jServerTimeStamp = cJSON_GetObjectItem(jServerInfo, "serverTimestamp");
      if (jServerTimeStamp)
      {
         if (strlen(m_LastCommandTS) == 0)
         {
            strcpy(m_LastCommandTS, jServerTimeStamp->valuestring);
            LOG1(Tools::Logger::lmInfo, "Got Server Timestamp: %s", m_LastCommandTS);

            StartClock(); //WARN: for debug purpose only! Calls _time_set()!;
         }

         m_Started = true;
         success = true;
      }
      else
      {
         result = FormatError(ErrorDH::EC_NoTimeStamp, ErrorDH::ET_AppSpecific,
                              ErrorDH::EP_GwOnGotServerInfo,
                              "No serverTimestamp in ServerInfo response");
      }
   }

   cJSON_Delete(jServerInfo);

   if(m_AppGotServerInfoCallback)
   {
      LOG2(Tools::Logger::lmInfo, "Gateway{0x%X} has got ServerInfo and calling callback {0x%X}",
           this, m_AppGotServerInfoCallback);

      GatewayListener::instrOnGotServerInfoCB cb = m_AppGotServerInfoCallback;
      m_AppGotServerInfoCallback = 0;
      (m_pApp->*cb)(result); // The Application is responsible for calling
                                     // AsyncRegister(), result deletion and so on!!!
   }
   else
   {
      if(success)
      {
         AsyncRegister();
      }
      else
      {
         if (!result->Interrupted())
         {
            LOG1(Tools::Logger::lmError, "Failed to get ServerInfo: %s", result->What());
            m_pApp->HandleError(result);

            Sleep(1000 * 3);// TODO: make the delayed call asynchronously
            // You can change this time to 1 minute according to FS-ID 5052
            AsyncGetServerInfo();
         }
         else
         {
            LOG(Tools::Logger::lmInfo, "OnGotServerInfo: interrupted");
            delete result;
         }
      }
   }
}

//-----------------------------------------------------------------------------
void CGateway::OnRegistered(ResultDH result, cJSON* jRegisterInfo)
//-----------------------------------------------------------------------------
{
   LOG(Tools::Logger::lmInfo, "OnRegistered");
   bool success = false;
   if (result == DH_NO_ERROR)
   {
      LOG(Tools::Logger::lmInfo, "Got Device registered!");
      success = true;
   }

   if(m_InRegisterCallback)
   {
      LOG2(Tools::Logger::lmInfo, "Gateway{0x%X} has Registered Device and calling callback {0x%X}",
           this, m_InRegisterCallback);

      GatewayListener::instrOnRegisterCB cb = m_InRegisterCallback;
      m_InRegisterCallback = 0;
      (m_pApp->*cb)(result, jRegisterInfo); // The Application is responsible for calling
                                                    // AsyncPollCommands(), result deletion and so on!!!
   }
   else
   {
      if(success)
      {
         if (!m_PollingIsStarted)
         {
            AsyncPollCommands(GetLastCommandTimestamp());
         }
         else
         {
            LOG(Tools::Logger::lmTrace, "Commands polling HAS AlREADY BEEN started!");
         }
      }
      else
      {
         if (!result->Interrupted())
         {
            LOG1(Tools::Logger::lmTrace, "Failed to get Register: %s", result->What());
            m_pApp->HandleError(result);

            Sleep(1000 * 3);// TODO: make the delayed call asynchronously
            // You can change this time to 1 minute according to FS-ID 5052
            AsyncGetServerInfo();
         }
         else
         {
            LOG(Tools::Logger::lmInfo, "OnRegistered: interrupted");
            delete result;
         }
      }
   }
}

//-----------------------------------------------------------------------------
void CGateway::OnPollCommands(ResultDH result, cJSON* jCommands)
//-----------------------------------------------------------------------------
{
   LOG(Tools::Logger::lmInfo, "OnPollCommands");

   if (result == DH_NO_ERROR)
   {
      size_t commandsSize = cJSON_GetArraySize(jCommands);
      LOG1(Tools::Logger::lmInfo, "Got %d Commands in response!", commandsSize);

      for(size_t i = 0; i < commandsSize; ++i)
      {
         // WARN: Please, note, that this jCommand object is normally deleted in CApplication::OnCommandAcknowledged
         // (or CGateway::OnCommandAcknowledged or CRestProtocol::OnCommandAcknowledged
         // in the case if no callbacks were provided).
         cJSON* jCommand = cJSON_DetachItemFromArray(jCommands, 0);
         cJSON* jTimestamp = cJSON_GetObjectItem(jCommand, "timestamp");
         if (jTimestamp)
         {
            strcpy(m_LastCommandTS, jTimestamp->valuestring);
         }

         m_pApp->HandleCommand(jCommand);
      }

      cJSON_Delete(jCommands);

      m_RestProto->AsyncPollCommands(m_LastCommandTS,
          (CRestProtocol::RestListener::gwOnPollCommandsCB)&CGateway::OnPollCommands);
   }
   else if (result->Interrupted())// if interrupted
   {
      LOG(Tools::Logger::lmInfo, "OnPollCommands: interrupted");
      m_PollingIsStarted = false;
      delete result;
   }
   else
   {
      LOG1(Tools::Logger::lmError, "Failed to poll commands: %s", result->What());
      m_pApp->HandleError(result);

      Sleep(3 * 1000); // TODO: make the delayed call asynchronously
      m_RestProto->AsyncPollCommands(m_LastCommandTS,
                                 (CRestProtocol::RestListener::gwOnPollCommandsCB)&CGateway::OnPollCommands);
   }
}

//-----------------------------------------------------------------------------
void CGateway::OnNotify(ResultDH result, cJSON* jServerNotification, RetryableObject* pSentNotification)
//-----------------------------------------------------------------------------
{
   LOG(Tools::Logger::lmInfo, "OnNotify");
   bool success = false;
   if (result == DH_NO_ERROR)
   {
      LOG1(Tools::Logger::lmInfo, "A Notif{0x%X} has been sent successfully", pSentNotification);
      success = true;
   }

   if(m_InNotifyCallback)
   {
      LOG3(Tools::Logger::lmInfo, "Gateway{0x%X} has sent a Notif{0x%X} and calling callback {0x%X}",
           this, pSentNotification, m_InNotifyCallback);

      GatewayListener::instrOnNotifyCB cb = m_InNotifyCallback;
      m_InNotifyCallback = 0;
      (m_pApp->*cb)(result, jServerNotification, pSentNotification);
      // The Application is responsible for pSentNotification and jServerNotification delition,
      // result deletion, retry attempts and so on!!!!
   }
   else
   {
      if(!success)
      {
         if (!result->Interrupted())
         {
            LOG2(Tools::Logger::lmTrace, "Failed to send a Notif{0x%X}: %s",
                 pSentNotification, result->What());
            m_pApp->HandleError(result);
         }
         else
         {
            LOG(Tools::Logger::lmInfo, "OnCommandAcknowledged: interrupted");
            delete result;
         }
      }

      cJSON_Delete(jServerNotification);
      if(pSentNotification)
      {
         delete pSentNotification;
      }
   }
   
   m_NotificationCompletedEvent.Set();
   m_NotificationIsInProgress = false;
}

//-----------------------------------------------------------------------------
void CGateway::OnCommandAcknowledged(ResultDH result, RetryableObject* pCommand)
//-----------------------------------------------------------------------------
{
   LOG(Tools::Logger::lmInfo, "OnCommandAcknowledged");
   bool success = false;
   if (result == DH_NO_ERROR)
   {
      LOG2(Tools::Logger::lmInfo, "Gateway{0x%X} has successfully acknowledged Command{0x%X}",
           this, pCommand);
      success = true;
   }

   if(m_InCommandAckCallback)
   {
      LOG3(Tools::Logger::lmInfo, "Gateway{0x%X} has acknowledged Command{0x%X} and calling callback {0x%X}",
           this, pCommand, m_InCommandAckCallback);

      GatewayListener::instrOnCommandAckCB cb = m_InCommandAckCallback;
      m_InCommandAckCallback = 0;
      (m_pApp->*cb)(result, pCommand); // The Application is responsible for jCommand delition,
                                       // result deletion, retry attempts and so on!!!!
   }
   else
   {
      if(!success)
      {
         if (!result->Interrupted())
         {
            LOG2(Tools::Logger::lmTrace, "Failed to acknowledged Command{0x%X}: %s", pCommand, result->What());
            m_pApp->HandleError(result);
         }
         else
         {
            LOG(Tools::Logger::lmInfo, "OnCommandAcknowledged: interrupted");
            delete result;
         }
      }

      if (pCommand)
      {
         delete pCommand;
         pCommand = 0;
      }
   }
   
   m_AcknowledgementCompletedEvent.Set();
   m_AcknowledgementIsInProgress = false;
}
//-----------------------------------------------------------------------------

void CGateway::StartClock()
//-----------------------------------------------------------------------------
{
   TIME_STRUCT time;
   DATE_STRUCT CurTime;

   //m_LastCommandTS Time format: 2015-01-13T16:41:58.989000
   sscanf(m_LastCommandTS, "%04d-%02d-%02dT%02d:%02d:%02d.%03d000",
          &CurTime.YEAR, &CurTime.MONTH, &CurTime.DAY,
          &CurTime.HOUR, &CurTime.MINUTE, &CurTime.SECOND, &CurTime.MILLISEC);

   bool res = _time_from_date(&CurTime, &time);
   _time_set(&time);
}

//-----------------------------------------------------------------------------
void CGateway::GetTimeStamp(char* dest)
//-----------------------------------------------------------------------------
{
   if (dest)
   {
      TIME_STRUCT time;
      _time_get(&time);
      DATE_STRUCT CurTime;
      bool res = _time_to_date(&time, &CurTime);

      //TimeStamp format: 2015-01-13T16:41:58.989000
      sprintf(dest, "%04d-%02d-%02dT%02d:%02d:%02d.%03d000",
              CurTime.YEAR, CurTime.MONTH, CurTime.DAY,
              CurTime.HOUR, CurTime.MINUTE, CurTime.SECOND, CurTime.MILLISEC);
   }
}

