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

#include <cyassl/ssl.h>

extern "C"
{
#include <ipcfg.h>
}

#include "ErrorDH.h"
#include "NonCopyable.h"
#include "Namespace.h"
//-----------------------------------------------------------------------------
class CConnection : private NonCopyable
{
public:
   // Construction / Destruction   
    CConnection(_ip_address IpAddr, UInt16 Port);
    
    virtual ~CConnection();

public:
   virtual ResultDH Connect(UInt32 timeoutMs);
   
   virtual ResultDH WriteSome(const UInt8* pBuf, UInt32 bytesToWrite, UInt32& bytesWritten) = 0;
   virtual ResultDH ReadSome(UInt8* pBuf, UInt32 bufCapacity, UInt32& bytesRead) = 0;     
   
   virtual UInt32 Shutdown(); 

public:   
   bool IsConnected(){return m_isConnected == true;}
   bool IsSecure(){return m_isSecure == true;}     
   
   _ip_address GetIP(){return m_IP;}   
   UInt32 GetPort(){return m_port;}  
   
public:
   ResultDH SetAtomicReadTimeout(UInt32 timeoutMs);
   ResultDH SetAtomicWriteTimeout(UInt32 timeoutMs);   
   UInt32 Wait(UInt32 timeoutMs);   
   
protected:
   UInt32 m_sock; 
   _ip_address m_IP;
   UInt16 m_port;  
   bool m_isConnected;  
   bool m_isSecure;
};
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
class SimpleConnection : public CConnection
{
   typedef CConnection Base;   
public:   
   SimpleConnection(_ip_address IpAddr, UInt16 Port);
   virtual ~SimpleConnection();   
   
   virtual ResultDH Connect(UInt32 timeoutMs);
   
   virtual ResultDH WriteSome(const UInt8* pBuf, UInt32 bytesToWrite, UInt32& bytesWritten);
   virtual ResultDH ReadSome(UInt8* pBuf, UInt32 bufCapacity, UInt32& bytesRead);     
   
   virtual UInt32 Shutdown();  
};
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
class SecureConnection : public CConnection
{
   typedef CConnection Base;      
public:      
   SecureConnection(_ip_address IpAddr, UInt16 Port);   
   virtual ~SecureConnection();      
   
   virtual ResultDH Connect(UInt32 timeoutMs);
   
   virtual ResultDH WriteSome(const UInt8* pBuf, UInt32 bytesToWrite, UInt32& bytesWritten);
   virtual ResultDH ReadSome(UInt8* pBuf, UInt32 bufCapacity, UInt32& bytesRead);     
   
   virtual UInt32 Shutdown();     
      
private:
   CYASSL*        m_sockssl;
   CYASSL_CTX*    m_ctx;
};
//-----------------------------------------------------------------------------

#include "NamespaceEnd.h"