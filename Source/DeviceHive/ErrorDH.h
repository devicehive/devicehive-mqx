/*-----------------------------------------------------------------------------

Autor                                                           Datum
Nikolay Peganov <Nikolay.Peganov@dataart.com>                   25.12.2014

-- BESCHREIBUNG ---------------------------------------------------------------


-- AENDERUNGEN ----------------------------------------------------------------

Autor                  Datum

-----------------------------------------------------------------------------*/
#pragma once
#include "ToolsLib.h"

#include "NonCopyable.h"
#include "Namespace.h"

static const UInt32 DH_NO_ERROR = MQX_OK;  
class ErrorDH : private NonCopyable
{
public:
   enum ErrorType
   {
      ET_AppSpecific = 0,
      ET_RTCS,           
      ET_IPCFG,     
      ET_SYASSL,            
         
      ET_HttpStatus,             
      ET_InvalidResponse      
   };

   enum ErrorPlace
   {
      EP_GwNetworkSetup = 0,      
      EP_GwStartup,          
      
      EP_SocketConnect,
      EP_SocketWtiteSome,
      EP_SocketReadSome,      
      EP_SslSocketConnect,
      EP_SslSocketWtiteSome,
      EP_SslSocketReadSome,   
      
      EP_SetAtomicReadTimeout,
      EP_SetAtomicWriteTimeout,      
      
      EP_WtiteRequest,
      EP_ReadResponse,
      
      EP_RestRegister,      
      EP_RestWaitRegistrationComplete,            
      
      EP_RestOnGotServerInfo,
      EP_RestOnGotRegistered,         
      EP_RestOnPollCommands,      
      EP_RestOnNoify,            
      EP_RestOnAcknowledged,

      EP_GwSendingNoification,            
      EP_GwSendingAcknowledgement,
      
      EP_GwOnGotServerInfo,    
   
      EP_DnsResolving                  
   };   
    
   static const UInt32 cAppSpecificErrCodeBase = 0x0A0A0000;
   
   enum AppSpecificErrors
   {
      EC_FailedToCreateSocket = cAppSpecificErrCodeBase | 0x01,
      EC_FailedToCreateSSLSocket,
      EC_Interrupted,
      EC_TimedOut,
      
      EC_ContentIsExpected,
      EC_NoTimeStamp,      
      
      EC_DnsResolverError,
      
      EC_GwFailedToGetSerial,
      EC_GwFailedToGetMac,      
      
      EC_GaveUpWaitingForRegistration,
      
      EC_GaveUpWaitingForNotification,   
      EC_GaveUpWaitingForAcknowledgement,     
   };
   
   
private:   
   ErrorDH();
   
public:
   explicit ErrorDH(UInt32 code, ErrorType type, ErrorPlace place,
                    const char* what = "", UInt32 lineNumber = __LINE__,
                    char* sourceFilename = __FILE__);   
   virtual ~ErrorDH();
   
   bool Interrupted();  
   const char* What();
   
protected: // Constans   
   UInt32 m_Code;
   ErrorType m_Type;
   ErrorPlace m_Place;   
   char* m_pWhat;
   char* m_pFile;
   size_t line;
};

typedef ErrorDH* ResultDH;  

#define FormatError(code, type, place, what) new ErrorDH(code, type, place, what, __LINE__, __FILE__);
      
#include "NamespaceEnd.h"