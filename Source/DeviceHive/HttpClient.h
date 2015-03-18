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

#include "HttpTask.h"
#include "Connection.h"
#include "ConnectionGuard.h"
#include "NonCopyable.h"

#include "Namespace.h"

namespace HTTP
{
  const char cCRLF[]              = "\r\n";
  const char cCRLFx2[]            = "\r\n\r\n";   
  const char cHTTP_1_1[]          = "HTTP/1.1";
  const char cDefaultProto[]      = "http";     

  const char Host[]               = "Host: ";
  const char Allow[]              = "Allow: ";
  const char Accept[]             = "Accept: ";
  const char Authorization[]      = "Authorization: ";  
  const char Connection[]         = "Connection: ";
  const char Content_Encoding[]   = "Content-Encoding: ";
  const char Content_Length[]     = "Content-Length: ";
  const char Content_Type[]       = "Content-Type: ";
  const char Expires[]            = "Expires: ";
  const char Last_Modified[]      = "Last-Modified: ";
  const char User_Agent[]         = "User-Agent: ";
  const char Location[]           = "Location: ";
  const char Upgrade[]            = "Upgrade: ";
    
  const char AuthDeviceKey[]      = "Auth-DeviceKey: ";
  
  const char AuthDeviceID[]       = "Auth-DeviceID: ";
  const char AuthDeviceMac[]      = "Auth-DeviceMac: ";  
  const char AuthDeviceMacSig[]   = "Auth-DeviceMacSig: ";  
  const char AuthNonce[]          = "Auth-Nonce: ";    
  const char AuthSignature[]      = "Auth-Signature: ";
  
  
  // This namespace contains definition of common status codes from HTTP/1.0 and HTTP/1.1.
  namespace Status
  {
     //Internal (0xx) codes.
     enum Internal
     {
        UNKNOWN = 0
     };
     
     //Informational (1xx) codes.
     enum Informational
     {
        CONTINUE = 100
     };
     
     //Success (2xx) codes.
     enum Success
     {
        OK         = 200,
        CREATED    = 201,
        ACCEPTED   = 202,
        NO_CONTENT = 204
     };
     
     //Redirection (3xx) codes.
     enum Redirection
     {
        MOVED_PERMANENTLY = 301,
        MOVED_TEMPORARILY = 302,
        NOT_MODIFIED      = 304
     };
     
     //Client Error (4xx) codes.
     enum ClientError
     {
        BAD_REQUEST             = 400,
        UNAUTHORIZED            = 401,
        FORBIDDEN               = 403,
        NOT_FOUND               = 404,
        
        AUTHENTICATION_TIMEOUT  = 419        
     };
     
     //Server Error (5xx) codes.
     enum ServerError
     {
        INTERNAL_SERVER_ERROR = 500,
        NOT_IMPLEMENTED       = 501,
        BAD_GATEWAY           = 502,
        SERVICE_UNAVAILABLE   = 503,
        GATEWAY_TIMEOUT       = 504
     };     
  } // Status namespace  
}

struct Endpoint
{
   char HostName[96];
   _ip_address IP;
};

//class CRestProtocol;
class CHttpClient : CConnectionGuard::GuardListener
                  , CHttpTask::TaskListener
                  , private NonCopyable
{ 
public:   
   static const UInt8 cCacheSize = 5;
   
private:
   CHttpClient();
   
public:   
      // delivers the singleton instance
   static CHttpClient* GetInstance();   
   void Stop();   
   
public:      
   virtual ~CHttpClient();
   
public:
   CHttpTask* NewPOST(const char * url);
   CHttpTask* NewGET(const char * url);     
   CHttpTask* NewPUT(const char * url);    
   CHttpTask* NewDELETE(const char * url);    
   
   ResultDH AsyncSend(CHttpTask* task, TaskListener* listener,
                    TaskListener::TaskCallback Callback, UInt32 timeoutMs,
                    const char* taskName = 0);
   
   ResultDH AsyncSendWithObject(CHttpTask* task, TaskListener* listener, 
                              TaskListener::TaskWithObjectCallback Callback,
                              UInt32 timeoutMs, void* objectToPostProcess,
                              const char* taskName = 0);   

   bool ReleaseTask(CHttpTask* task);   

private:
   CHttpTask* CreateTask();
   ResultDH PrepareTask(CHttpTask* task);   
   ResultDH GetIPForHostName(const char* hostName, _ip_address& ipDest);   

private:   
   void OnConnectionExpired(UInt32 result, CConnectionGuard* pGuard);

private:
   CHttpTask* m_ActiveTasks[cCacheSize];
   CConnectionGuard* m_ConnectionCache[cCacheSize];
   Endpoint* m_ResolverCache[cCacheSize];
   
   UInt8 m_TasksCount;
   UInt8 m_ConnectionsCount;   
   
   Tools::CCriticalSection m_ConnCacheMutex;
   
   static const UInt32 cIdleConnectionTimeMs = 1000 * 60 * 9;
   
private:
   static CHttpClient* m_pSingletone;     
};


#include "NamespaceEnd.h"