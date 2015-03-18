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

#include "REST.h"
#include "RetryableObject.h"
#include "NonCopyable.h"
#include "Namespace.h"

class CApplication;
//-----------------------------------------------------------------------------
class CGateway : public Tools::CThread
               , public CRestProtocol::RestListener
               , private NonCopyable
{ 
public:
   class GatewayListener
   {
   public:
      typedef void (GatewayListener::* instrOnGwStartedCB)(ResultDH result);      
      typedef void (GatewayListener::* instrOnGotServerInfoCB)(ResultDH result);           
      
      typedef void (GatewayListener::* instrOnRegisterCB)(ResultDH result, 
                                                          cJSON* jRegistrationInfo);   
      
      typedef void (GatewayListener::* instrOnCommandAckCB)(ResultDH result, RetryableObject* pCommand);        
      
      typedef void (GatewayListener::* instrOnNotifyCB)(ResultDH result, 
                                                        cJSON* jServerNotification, 
                                                        RetryableObject* pSentNotification);  
   };
   
   enum DHCPModeEnum
   {
      Disable = 0,
      Enable,
      Service
   };
     
private:
  static const UInt8 cMacLen = 6;    
  static const UInt8 cStrMacLen = (cMacLen * 2) + 1;
  static const UInt8 cSerialLen = 36 + 1;      
  static const UInt8 cLastCommandTSLen = 32 + 1;    
  
public:
  class CDevice
  {
  public:
    char m_MacAdressStr[cStrMacLen];  
    char m_Serial[cSerialLen];    
  };
   
private:
   //Default constructor is not supported
   CGateway(); 
   
public:   
      // delivers the singleton instance
   static CGateway* GetInstance();   
   
   void Init(CApplication* pApp, DHCPModeEnum DHCPMode = Enable);
   void UpdateServerUrl(const char* newServerUrl);
   void Stop();
   virtual void StartClock(); //WARN: for debug purpose only! Calls _time_set()!;  
   void GetTimeStamp(char* dest);   
   void NotifyAboutError(ResultDH error);   
   bool IsRegistrationInProcess() const
   {return m_RestProto->GetRegistrationStatus();}      
   
public:
   virtual ~CGateway();
   
public:
   void AsyncStart(GatewayListener::instrOnGwStartedCB gwStartCallback);// Start the gateway
   void AsyncGetServerInfo(GatewayListener::instrOnGotServerInfoCB gwServerInfoCallback = 0);   
   void AsyncRegister(GatewayListener::instrOnRegisterCB inRegisterCallback = 0);// Register the Device
   void AsyncPollCommands(const char* pLastCommandTimestamp);// Poll commands
   void AsyncAcknowledgeCommand(RetryableObject* pCommand,// Acknowledge command
                                GatewayListener::instrOnCommandAckCB inCommandAckCallback);
   void AsyncSendNotification(RetryableObject* pNotification,// Send a notification      
                              GatewayListener::instrOnNotifyCB inNotifyCallback);     

private:
   ResultDH SetupNetwork(UInt8* pMacAddress, DHCPModeEnum DHCPMode);  
   
private:
   // Gateway's callback methods
   void OnGotServerInfo(ResultDH result, cJSON* jServerInfo);   
   void OnRegistered(ResultDH result, cJSON* jRegistrationInfo); 
   void OnPollCommands(ResultDH result, cJSON* jCommands);   
   void OnCommandAcknowledged(ResultDH result, RetryableObject* pCommand);   
   void OnNotify(ResultDH result, cJSON* jServerNotification, RetryableObject* pNotification);  
   
public:
   const char* GetServerUrl();
   const char* GetDeviceID();
   const char* GetDeviceMAC();
   const char* GetLastCommandTimestamp();   
   
private:  
   // Thread Functions
   virtual UInt32 Run();
   virtual void ExitInstance();
   virtual void ExitInstance(UInt32 result);  
   
private:
   CApplication* m_pApp;
   
   // Application's callback pointers 
   GatewayListener::instrOnGwStartedCB m_AppGwStartCallback;
   GatewayListener::instrOnGotServerInfoCB m_AppGotServerInfoCallback;   
   GatewayListener::instrOnRegisterCB m_InRegisterCallback;   
   GatewayListener::instrOnCommandAckCB m_InCommandAckCallback;     
   GatewayListener::instrOnNotifyCB m_InNotifyCallback;     
   
   char* m_pServerUrl;
   
   static const UInt8 m_cGwStartPrio = 10;  
   static const UInt32 m_cGwStartStackSize = 15000; // RSA encryption needs more stack

   bool m_Started;
   bool m_PollingIsStarted;
   
   UInt32 m_ipAdr;
   UInt32 m_ipMask;
   UInt32 m_ipGateway;  
   
   DHCPModeEnum m_DHCPMode;
   
   CRestProtocol* m_RestProto;
   
private: // DH-device interface of the Device   
   char m_LastCommandTS[cLastCommandTSLen];
   CDevice m_Device;  
   
private:
   Tools::CEvent m_NotificationCompletedEvent;   
   Tools::CEvent m_AcknowledgementCompletedEvent;   
   
   bool m_NotificationIsInProgress;
   bool m_AcknowledgementIsInProgress;   
   
private:
   static CGateway* m_pSingletone;      
};


#include "NamespaceEnd.h"
