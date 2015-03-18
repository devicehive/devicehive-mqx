/*-----------------------------------------------------------------------------

Autor                                                           Datum
Nikolay Peganov <Nikolay.Peganov@dataart.com>                   17.11.2014

-- BESCHREIBUNG ---------------------------------------------------------------


-- AENDERUNGEN ----------------------------------------------------------------

Autor                  Datum

-----------------------------------------------------------------------------*/
#include "MQX.h"
#include "rtcs.h"
extern "C"
{
#include <ipcfg.h>
}


#include "Connection.h"
using namespace DH;

// Construction / Destruction
//-----------------------------------------------------------------------------
CConnection::CConnection(_ip_address IpAddr, UInt16 Port)
//-----------------------------------------------------------------------------
: m_sock(0)
, m_IP(IpAddr)
, m_port(Port)
, m_isConnected(0)
{
   LOG1(Tools::Logger::lmTrace, "Conn{0x%X} created", this);
}

//-----------------------------------------------------------------------------
CConnection::~CConnection()
//-----------------------------------------------------------------------------
{
   LOG1(Tools::Logger::lmTrace, "Conn{0x%X} destroyed", this);
}

//-----------------------------------------------------------------------------
ResultDH CConnection::Connect(UInt32 timeoutMs)
//-----------------------------------------------------------------------------
{
   sockaddr_in addr;

   //Create the stream socket for the TCP connection
   m_sock = socket(PF_INET, SOCK_STREAM, 0);
   if (m_sock == RTCS_SOCKET_ERROR)
   {
      LOG(Tools::Logger::lmWarning, "Failed to open soket");

      Shutdown();

      return FormatError(ErrorDH::EC_FailedToCreateSocket,
                         ErrorDH::ET_AppSpecific,
                         ErrorDH::EP_SocketConnect,
                         "Failed to open socket");
   }

   //Set socket properties
   addr.sin_family      = AF_INET;
   addr.sin_port        = m_port;
   addr.sin_addr.s_addr = m_IP;

   UInt32 SockOptValue;

   // Attempt establishing connection during timeoutMs seconds
   UInt32 result = setsockopt(m_sock, SOL_TCP, OPT_CONNECT_TIMEOUT, &timeoutMs, sizeof(UInt32));
   if (result != RTCS_OK)
   {
      Shutdown();
      return FormatError(result, ErrorDH::ET_RTCS, ErrorDH::EP_SocketConnect,
                         "Failed to set OPT_CONNECT_TIMEOUT socket option");
   }

   // periodically probe the remote endpoint, whether the remote endpoint is still present
   SockOptValue = 1; // 1 minute
   result = setsockopt(m_sock, SOL_TCP, OPT_KEEPALIVE, &SockOptValue, sizeof(UInt32));
   if (result != RTCS_OK)
   {
      Shutdown();
      return FormatError(result, ErrorDH::ET_RTCS, ErrorDH::EP_SocketConnect,
                         "Failed to set OPT_KEEPALIVE socket option");
   }

   // When the socket is bound, RTCS allocates a send buffer of the
   // specified number of bytes, which controls how much sent data RTCS
   // can buffer for the socket.
   SockOptValue = 4380 * 3; // bytes
   result = setsockopt(m_sock, SOL_TCP, OPT_TBSIZE, &SockOptValue, sizeof(UInt32));
   if (result != RTCS_OK)
   {
      Shutdown();
      return FormatError(result, ErrorDH::ET_RTCS, ErrorDH::EP_SocketConnect,
                         "Failed to set OPT_TBSIZE socket option");
   }

   // When the socket is bound, RTCS allocates a receive buffer of the
   // specified number of bytes, which controls how much received data
   // RTCS can buffer for the socket.
   SockOptValue = 4380 * 3; // bytes
   result = setsockopt(m_sock, SOL_TCP, OPT_RBSIZE, &SockOptValue, sizeof(UInt32));
   if (result != RTCS_OK)
   {
      Shutdown();
      return FormatError(result, ErrorDH::ET_RTCS, ErrorDH::EP_SocketConnect,
                         "Failed to set OPT_RBSIZE socket option");
   }

   // connect to server
   LOG(Tools::Logger::lmTrace, "Connecting...");
   result = connect(m_sock, &addr, sizeof(addr));
   if (result != RTCS_OK)
   {
      LOG5(Tools::Logger::lmWarning, "Failed to connect to %d.%d.%d.%d:%d",
           m_IP >> 24, (m_IP >> 16) & 0xFF, (m_IP >> 8) & 0xFF, m_IP & 0xFF, m_port);

      Shutdown();
      return FormatError(result, ErrorDH::ET_RTCS, ErrorDH::EP_SocketConnect,
                         "Failed to connect to a server");
   }
   else
   {
     LOG6(Tools::Logger::lmInfo, "Socket{0x%X} connected to %d.%d.%d.%d:%d",
       m_sock, m_IP >> 24, (m_IP >> 16) & 0xFF, (m_IP >> 8) & 0xFF, m_IP & 0xFF, m_port);
   }

   return DH_NO_ERROR;
}


//-----------------------------------------------------------------------------
UInt32 CConnection::Shutdown()
//-----------------------------------------------------------------------------
{
   UInt32 result;

   if (m_sock != 0)
   {
      result = shutdown(m_sock, FLAG_CLOSE_TX);
      m_sock = 0;
   }

   LOG1(Tools::Logger::lmTrace, "Conn{0x%X} Shutdown", this);
   return result;
}

//-----------------------------------------------------------------------------
ResultDH CConnection::SetAtomicReadTimeout(UInt32 timeoutMs)
//-----------------------------------------------------------------------------
{
   // When the timeout expires, recv() returns with whatever data that has been received.
   UInt32 res = setsockopt(m_sock, SOL_TCP, OPT_RECEIVE_TIMEOUT, &timeoutMs, sizeof(UInt32));
   if (res != RTCS_OK)
   {
      return FormatError(res, ErrorDH::ET_RTCS,
                         ErrorDH::EP_SetAtomicWriteTimeout,
                         "Failed to set OPT_RECEIVE_TIMEOUT socket option");

   }
   else
   {
      return DH_NO_ERROR;
   }
}

//-----------------------------------------------------------------------------
ResultDH CConnection::SetAtomicWriteTimeout(UInt32 timeoutMs)
//-----------------------------------------------------------------------------
{
   // When the timeout expires, send() returns with whatever data that has been received.
   UInt32 res = setsockopt(m_sock, SOL_TCP, OPT_SEND_TIMEOUT, &timeoutMs, sizeof(UInt32));
   if (res != RTCS_OK)
   {
      return FormatError(res, ErrorDH::ET_RTCS,
                         ErrorDH::EP_SetAtomicWriteTimeout,
                         "Failed to set OPT_RECEIVE_TIMEOUT socket option");

   }
   else
   {
      return DH_NO_ERROR;
   }
}

//-----------------------------------------------------------------------------
UInt32 CConnection::Wait(UInt32 timeoutMs)
//-----------------------------------------------------------------------------
{
   UInt32 result = RTCS_selectset(&m_sock, 1, timeoutMs);
   if (result == m_sock) // The connection is used by some task
   {
      result = RTCS_OK;
      LOG1(Tools::Logger::lmTrace, "Conn{0x%X}: Got signal => obtained by some HttpTask", this);
   }
   else if (result == 0) // The connection has expired
   {
      result = RTCSERR_TCP_TIMED_OUT;
      LOG1(Tools::Logger::lmTrace, "Conn{0x%X}: Got signal. Timeout => Idle connection has expired", this);
   }
   else if (result == RTCS_SOCKET_ERROR) // An Error has oqured or server has closed a connection.
   {
      result = RTCS_geterror(m_sock);
      LOG2(Tools::Logger::lmTrace, "Conn{0x%X}: Got ERROR signal. Error{0x%X}.", this, result);
      if (result == RTCSERR_TCP_TIMED_OUT)
      {
         result = RTCSERR_TCP_CONN_RLSD; // Just an appropriate error code.
      }
   }

   return result;
}


//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------


//-----------------------------------------------------------------------------
SimpleConnection::SimpleConnection(_ip_address IpAddr, UInt16 Port)
//-----------------------------------------------------------------------------
: CConnection(IpAddr, Port)
{
   LOG1(Tools::Logger::lmTrace, "SimpleConn{0x%X} created", this);
   m_isSecure = false;
}

//-----------------------------------------------------------------------------
SimpleConnection::~SimpleConnection()
//-----------------------------------------------------------------------------
{
   Shutdown();
   LOG1(Tools::Logger::lmTrace, "SimpleConn{0x%X} destroyed", this);
}

//-----------------------------------------------------------------------------
ResultDH SimpleConnection::Connect(UInt32 timeoutMs)
//-----------------------------------------------------------------------------
{
   LOG5(Tools::Logger::lmTrace, "Establishing simple connection to %d.%d.%d.%d:%d",
       m_IP >> 24, (m_IP >> 16) & 0xFF, (m_IP >> 8) & 0xFF, m_IP & 0xFF, m_port);

   ResultDH result = Base::Connect(timeoutMs);

   if(result != DH_NO_ERROR)
   {
      return result;
   }

   m_isConnected = true;
   return result;
}

//-----------------------------------------------------------------------------
ResultDH SimpleConnection::WriteSome(const UInt8* pBuf, UInt32 bytesToWrite, UInt32& bytesWritten)
//-----------------------------------------------------------------------------
{
   ResultDH result = DH_NO_ERROR;
   UInt32 res = send(m_sock,(void*)pBuf, bytesToWrite, 0);

   if (res != RTCS_ERROR)
   {
      bytesWritten = res;
      LOG3(Tools::Logger::lmTrace, "Conn{0x%X}: %d bytes have been written into Socket{0x%X}.", this, bytesWritten, m_sock);
   }
   else
   {
      UInt32 err = RTCS_geterror(m_sock);
      if (err != RTCSERR_TCP_TIMED_OUT)
      {
         result = FormatError(RTCS_geterror(m_sock), ErrorDH::ET_RTCS,
                           ErrorDH::EP_SocketWtiteSome,
                           "Failed to write data into a socket");
      }
   }

   return result;
}

//-----------------------------------------------------------------------------
ResultDH SimpleConnection::ReadSome(UInt8* pBuf, UInt32 bufCapacity, UInt32& bytesRead)
//-----------------------------------------------------------------------------
{
   ResultDH result = DH_NO_ERROR;
   UInt32 res = recv(m_sock, (void*)pBuf, bufCapacity, 0);

   if (res != RTCS_ERROR)
   {
      bytesRead = res;
      LOG3(Tools::Logger::lmTrace, "Conn{0x%X}: %d bytes have been read from Socket{0x%X}.", this, bytesRead, m_sock);
   }
   else
   {
      UInt32 err = RTCS_geterror(m_sock);
      if (err != RTCSERR_TCP_TIMED_OUT)
      {
         result = FormatError(RTCS_geterror(m_sock), ErrorDH::ET_RTCS,
                           ErrorDH::EP_SocketReadSome,
                           "Failed to read data from a socket");
      }
   }
   return result;
}

//-----------------------------------------------------------------------------
UInt32 SimpleConnection::Shutdown()
//-----------------------------------------------------------------------------
{
   UInt32 result = Base::Shutdown();
   m_isConnected = false;
   LOG1(Tools::Logger::lmTrace, "SimpleConn{0x%X} shutdown", this);
   return result;
}


//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------


//-----------------------------------------------------------------------------
SecureConnection::SecureConnection(_ip_address IpAddr, UInt16 Port)
//-----------------------------------------------------------------------------
: Base(IpAddr, Port)
, m_sockssl(0)
, m_ctx(0)
{
   m_isSecure = true;
   LOG1(Tools::Logger::lmTrace, "SecureConn{0x%X} created", this);
}

//-----------------------------------------------------------------------------
SecureConnection::~SecureConnection()
//-----------------------------------------------------------------------------
{
   Shutdown();
   LOG1(Tools::Logger::lmTrace, "SecureConn{0x%X} destroyed", this);
}

//-----------------------------------------------------------------------------
ResultDH SecureConnection::Connect(UInt32 timeoutMs)
//-----------------------------------------------------------------------------
{
   LOG5(Tools::Logger::lmTrace, "Establishing SSL connection to %d.%d.%d.%d:%d",
       m_IP >> 24, (m_IP >> 16) & 0xFF, (m_IP >> 8) & 0xFF, m_IP & 0xFF, m_port);

   ResultDH res = Base::Connect(timeoutMs);

   if(res != DH_NO_ERROR)
   {
      return res;
   }

   LOG(Tools::Logger::lmTrace, "Getting SSL context");
   m_ctx = CyaSSL_CTX_new(CyaSSLv23_client_method());

   LOG(Tools::Logger::lmTrace, "Getting SSL object");
   if ((m_sockssl = CyaSSL_new(m_ctx)) == NULL)
   {
      LOG(Tools::Logger::lmWarning, "CyaSSL_new failed");
      Shutdown();
      return FormatError(ErrorDH::EC_FailedToCreateSSLSocket,
                               ErrorDH::ET_AppSpecific, ErrorDH::EP_SslSocketConnect,
                               "CyaSSL_new failed");
   }

   LOG2(Tools::Logger::lmTrace, "Creating SSL Socket{0x%X} on the Socket{0x%X}", m_sockssl, m_sock);

   UInt32 result = CyaSSL_set_fd(m_sockssl, m_sock);
   if (result != SSL_SUCCESS)
   {
      Int32 err;
      char err_desc[64];
      err = CyaSSL_get_error(m_sockssl, result);
      sprintf(err_desc, "%s", CyaSSL_ERR_error_string(err, err_desc));

      LOG2(Tools::Logger::lmWarning, "Failed to attach SSL to the socket: code %d, %s", err, err_desc);
      Shutdown();

      return FormatError(err, ErrorDH::ET_SYASSL, ErrorDH::EP_SslSocketConnect,
                         err_desc);
   }

   CyaSSL_set_verify(m_sockssl, SSL_VERIFY_NONE, 0);

   LOG(Tools::Logger::lmTrace, "Performing a handshake.");
   result = CyaSSL_connect(m_sockssl);
   if (result != SSL_SUCCESS)
   {
      Int32 err;
      char err_desc[64];
      err = CyaSSL_get_error(m_sockssl, result);
      sprintf(err_desc, "%s", CyaSSL_ERR_error_string(err, err_desc));

      LOG2(Tools::Logger::lmWarning, "Handshake error: code %d, %s", err, err_desc);
      Shutdown();
      return FormatError(err, ErrorDH::ET_SYASSL, ErrorDH::EP_SslSocketConnect,
                         err_desc);
   }

   LOG(Tools::Logger::lmInfo, "SSL Connection has been established successfully!");

   m_isConnected = true;
   return DH_NO_ERROR;
}

//-----------------------------------------------------------------------------
ResultDH SecureConnection::WriteSome(const UInt8* pBuf, UInt32 bytesToWrite, UInt32& bytesWritten)
//-----------------------------------------------------------------------------
{
   ResultDH result = DH_NO_ERROR;
   Int32 res = CyaSSL_write(m_sockssl,(void*)pBuf, bytesToWrite);

   if (res != RTCS_ERROR)
   {
      bytesWritten = res;
      LOG2(Tools::Logger::lmTrace, "%d bytes have been written into SSL Socket{0x%X}.", bytesWritten, m_sockssl);
   }
   else if(CyaSSL_want_write(m_sockssl) == 1)
   {
      // It is OK, will write again a bit later.
      LOG(Tools::Logger::lmTrace, "SSL socket writing timeout.");
   }
   else
   {
      result = FormatError(RTCS_geterror(m_sock), ErrorDH::ET_RTCS,
                           ErrorDH::EP_SslSocketWtiteSome,
                           "Failed to write data into a SslSocket");
   }

   return result;
}

//-----------------------------------------------------------------------------
ResultDH SecureConnection::ReadSome(UInt8* pBuf, UInt32 bufCapacity, UInt32& bytesRead)
//-----------------------------------------------------------------------------
{
   ResultDH result = DH_NO_ERROR;
   Int32 res = CyaSSL_read(m_sockssl, (void*)pBuf, bufCapacity);
   if (res > 0)
   {
      bytesRead = res;
      LOG2(Tools::Logger::lmTrace, "%d bytes have been read from SSL Socket{0x%X}.", bytesRead, m_sockssl);
   }
   else if(CyaSSL_want_read(m_sockssl) == 1)
   {
      // It is OK, will read again a bit later.
      LOG(Tools::Logger::lmTrace, "SSL socket reading timeout.");
   }
   else
   {
      result = FormatError(RTCS_geterror(m_sock), ErrorDH::ET_RTCS,
                           ErrorDH::EP_SslSocketReadSome,
                           "Failed to read data from a SslSocket");
   }

   return result;
}

//-----------------------------------------------------------------------------
UInt32 SecureConnection::Shutdown()
//-----------------------------------------------------------------------------
{
   UInt32 result;

   if (m_sockssl != 0)
   {
      result = CyaSSL_shutdown(m_sockssl);
      CyaSSL_free(m_sockssl);
      CyaSSL_CTX_free(m_ctx);
      m_sockssl = 0;
   }

   result = Base::Shutdown();
   m_isConnected = false;
   LOG1(Tools::Logger::lmTrace, "SecureConn{0x%X} Shutdown", this);
   return result;
}
