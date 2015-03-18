/*-----------------------------------------------------------------------------

Autor                                                           Datum
Nikolay Peganov <Nikolay.Peganov@dataart.com>                   17.11.2014

-- BESCHREIBUNG ---------------------------------------------------------------


-- AENDERUNGEN ----------------------------------------------------------------

Autor                  Datum

-----------------------------------------------------------------------------*/
#include "string.h"

#include "HttpTask.h"
#include "HttpClient.h"

using namespace DH;

// Construction
//-----------------------------------------------------------------------------
CHttpTask::CHttpTask()
: m_RestListener(0)
, m_OnTaskComplitedCallback(0)
, m_OnTaskWithObjectComplitedCallback(0)

, m_ObjectToPostProcess(0)

, m_Timoeut(0)
, m_pConn(0)

, m_pHost(0)
, m_Port(m_cDefaultPort)

, m_isActive(true)
, m_hasContent(false)
, m_isInterrupted(false)
, m_CurrentCapacity(cRequestBufferSize - 1)
, m_StartTime(0)
, m_RetryCounter(0)

, m_ResponseAssemblerState(RAS_WaitStatusCode)
, m_HttpStatusCode(0)
, m_HttpContentLength(0)
, m_pContent(0)

//-----------------------------------------------------------------------------
{
   LOG1(Tools::Logger::lmInfo, "Creating NEW HttpTask{0x%X}", this);

   FlushRequestBuffer();
   FlushResponseBuffer();

   _mem_zero(m_Proto, sizeof(m_Proto));
   m_TaskName[0] = 0;
}

//-----------------------------------------------------------------------------
CHttpTask::~CHttpTask()
//-----------------------------------------------------------------------------
{
   LOG1(Tools::Logger::lmInfo, "Destroying HttpTask{0x%X}", this);
   if (m_pHost)
   {
      delete [] m_pHost;
      m_pHost = 0;
   }
}

//-----------------------------------------------------------------------------
void CHttpTask::Init(ReqTypeEnum Type, const char* url)
//-----------------------------------------------------------------------------
{
   FlushRequestBuffer();

   switch (Type)
   {
      case RT_POST:
         strcpy(m_RequestBuffer, "POST ");
         break;

      case RT_GET:
         strcpy(m_RequestBuffer, "GET ");
         break;

      case RT_PUT:
         strcpy(m_RequestBuffer, "PUT ");
         break;

      case RT_DELETE:
         strcpy(m_RequestBuffer, "DELETE ");
         break;
   }

   m_CurrentCapacity -= strlen(m_RequestBuffer);

   ParseUrl((char*)url);
}

//-----------------------------------------------------------------------------
const char* CHttpTask::GetHost()
//-----------------------------------------------------------------------------
{
   return m_pHost;
}

//-----------------------------------------------------------------------------
bool CHttpTask::NeedsSecure()
//-----------------------------------------------------------------------------
{
   if (!strncmp(m_Proto, "https", strlen("https")))
      return true;
   else if (!strncmp(m_Proto, "wss", strlen("wss")))
      return true;
   else
      return false;
}

//-----------------------------------------------------------------------------
UInt16 CHttpTask::GetPort()
//-----------------------------------------------------------------------------
{
   return m_Port;
}

//-----------------------------------------------------------------------------
void CHttpTask::SetConnection(CConnection* conn)
//-----------------------------------------------------------------------------
{
   m_pConn = conn;
}

//-----------------------------------------------------------------------------
CConnection* CHttpTask::GetConnection()
//-----------------------------------------------------------------------------
{
   return m_pConn;
}

//-----------------------------------------------------------------------------
bool CHttpTask::HasContent()
//-----------------------------------------------------------------------------
{
   return m_hasContent == true;
}

//-----------------------------------------------------------------------------
void CHttpTask::AddPath(const char* path)
//-----------------------------------------------------------------------------
{
   if(path)
   {
      strcat(m_RequestBuffer, path);
      strcat(m_RequestBuffer, "/");
      m_CurrentCapacity = cRequestBufferSize - strlen(m_RequestBuffer);
   }
}

//-----------------------------------------------------------------------------
void CHttpTask::QueryPath(const char* query, const char* value)
//-----------------------------------------------------------------------------
{
   if(query && value)
   {
      if(strlen(m_RequestBuffer) > 0)
      {
         char lastChar = *(m_RequestBuffer + strlen(m_RequestBuffer) - 1);
         if (lastChar == '/')
         {
            strcat(m_RequestBuffer, "\?");
         }
         else
         {
            strcat(m_RequestBuffer, "&");
         }
      }

      strcat(m_RequestBuffer, query);
      strcat(m_RequestBuffer, "=");
      strcat(m_RequestBuffer, value);
      m_CurrentCapacity = cRequestBufferSize - strlen(m_RequestBuffer);
   }
}

//-----------------------------------------------------------------------------
void CHttpTask::FinishUrl()
//-----------------------------------------------------------------------------
{
   strcat(m_RequestBuffer, " ");
   strcat(m_RequestBuffer, HTTP::cHTTP_1_1);
   AddHeader(HTTP::Host, GetHost());
   m_CurrentCapacity = cRequestBufferSize - strlen(m_RequestBuffer);
}

//-----------------------------------------------------------------------------
void CHttpTask::AddHeader(const char* header, const char* value)
//-----------------------------------------------------------------------------
{
   if(header && value)
   {
      strcat(m_RequestBuffer, HTTP::cCRLF);
      strcat(m_RequestBuffer, header);
      strcat(m_RequestBuffer, value);
      m_CurrentCapacity = cRequestBufferSize - strlen(m_RequestBuffer);
   }
}

//-----------------------------------------------------------------------------
void CHttpTask::SetContent(char* content)
//-----------------------------------------------------------------------------
{
   if(content)
   {
      UInt32 contentLength = strlen(content);
      char sContentLen[16];
      sprintf(sContentLen, "%d", contentLength);
      UInt32 capacity = m_CurrentCapacity - strlen(HTTP::Content_Length) -
         strlen(sContentLen) - strlen(HTTP::cCRLFx2);

      char tmpCh = 0;
      if (contentLength > capacity)
      {
         tmpCh = *(content + capacity);
         *(content + m_CurrentCapacity) = 0;
      }

      sprintf(sContentLen, "%d", strlen(content));
      AddHeader(HTTP::Content_Length, sContentLen);
      Finalize();
      strcat(m_RequestBuffer, content);
      m_CurrentCapacity = cRequestBufferSize - strlen(m_RequestBuffer);

      if(tmpCh != 0)
      {
         *(content + capacity) = tmpCh;
      }

      m_hasContent = true;
   }
}

//-----------------------------------------------------------------------------
void CHttpTask::Finalize()
//-----------------------------------------------------------------------------
{
   strcat(m_RequestBuffer, HTTP::cCRLFx2);
   m_CurrentCapacity -= strlen(m_RequestBuffer);
}

//-----------------------------------------------------------------------------
bool CHttpTask::IsInterrupted()
//-----------------------------------------------------------------------------
{
   return m_isInterrupted == true;
}

//-----------------------------------------------------------------------------
bool CHttpTask::ResponseIsComplited()
//-----------------------------------------------------------------------------
{
   return m_ResponseAssemblerState == RAS_Complited;
}

//-----------------------------------------------------------------------------
bool CHttpTask::HttpStatusIsSuccessfull()
//-----------------------------------------------------------------------------
{
   return (m_HttpStatusCode == HTTP::Status::OK) ||
          (m_HttpStatusCode == HTTP::Status::CREATED) ||
          (m_HttpStatusCode == HTTP::Status::ACCEPTED) ||
          (m_HttpStatusCode == HTTP::Status::NO_CONTENT);
}

//-----------------------------------------------------------------------------
UInt16 CHttpTask::GetHttpStatus()
//-----------------------------------------------------------------------------
{
   return m_HttpStatusCode;
}

//-----------------------------------------------------------------------------
UInt16 CHttpTask::GetResponseState()
//-----------------------------------------------------------------------------
{
   return m_ResponseAssemblerState;
}

//-----------------------------------------------------------------------------
UInt8* CHttpTask::GetResponseContent()
//-----------------------------------------------------------------------------
{
   return m_pContent;
}

//-----------------------------------------------------------------------------
void CHttpTask::ParseUrl(char* url)
//-----------------------------------------------------------------------------
{
   char tmpCh;

   // protocol
   if (char* p = strstr(url, "://"))
   {
      const size_t len = (p - url);

      if (len > m_cProtoSize - 1)
      {
         tmpCh = url[m_cProtoSize - 1];
         url[m_cProtoSize - 1] = 0;
      }
      else
      {
         tmpCh = *p;
         *p = 0;
      }

      strcpy(m_Proto, url);
      *p = tmpCh;

      if (!strncmp(m_Proto, "http:", strlen("http:")))
         m_Port = 80;
      else if (!strncmp(m_Proto, "https", strlen("https")))
         m_Port = 443;
      else if (!strncmp(m_Proto, "ftp", strlen("ftp")))
         m_Port = 21;
      else if (!strncmp(m_Proto, "ws:", strlen("ws:")))
         m_Port = 80;
      else if (!strncmp(m_Proto, "wss", strlen("wss")))
         m_Port = 443;

      url += len + 3;
   }

   // host
   {
      const size_t len = strcspn(url, ":/?#");
      if (m_pHost)
      {
         delete [] m_pHost;
         m_pHost = 0;
      }

      m_pHost = new char[len + 1];
      Tools::OS::CopyBytes(url, m_pHost, len);
      m_pHost[len] = 0;

      /*
      tmpCh = *(url + len);
      *(url + len) = 0;
      strcpy(m_pHost, url);
      *(url + len) = tmpCh;
      */

      url += len;
   }

   // port
   if (url[0] == ':')
   {
      const size_t len = strcspn(++url, "/?#");
      if (len)
      {
         tmpCh = *(url + len);
         *(url + len) = 0;
         sscanf(url, "%d", &m_Port);
         *(url + len) = tmpCh;

         url += len;
      }
   }

   // path
   if (url[0] == '/')
   {
      strcat(m_RequestBuffer, url);
      if (strlen(m_RequestBuffer) > 0)
      {
         if (*(m_RequestBuffer + strlen(m_RequestBuffer) - 1) != '/')
         {
            strcat(m_RequestBuffer, "/");
         }
      }
   }
   else
   {
      strcat(m_RequestBuffer, "/");
   }
}

//-----------------------------------------------------------------------------
void CHttpTask::AsyncRun(const char* taskName)
//-----------------------------------------------------------------------------
{
   LOG1(Tools::Logger::lmTrace, "Task{0x%X}: About to run...", this);

   if (!taskName)
   {
      if(strlen(m_TaskName) == 0)
         strcpy(m_TaskName, "HttpTask");
   }
   else
   {
      strcpy(m_TaskName, taskName);
   }

   CreateThread(m_TaskName, m_cTaskPrio, m_cTaskStackSize);
}

//-----------------------------------------------------------------------------
bool CHttpTask::isTimedOut()
//-----------------------------------------------------------------------------
{
   bool res = (bool)TimeExpired(m_StartTime, m_Timoeut);

   if (res)
      LOG(Tools::Logger::lmWarning, "Timeout");

   return res;
}

//-----------------------------------------------------------------------------
void CHttpTask::FlushRequestBuffer()
//-----------------------------------------------------------------------------
{
   _mem_zero(m_RequestBuffer, cRequestBufferSize);
   m_CurrentCapacity = cRequestBufferSize - 1;
}

//-----------------------------------------------------------------------------
void CHttpTask::FlushResponseBuffer()
//-----------------------------------------------------------------------------
{
   _mem_zero(m_ResponseBuffer, cResponseBufferSize);
   m_CurrentCapacity = cResponseBufferSize - 1;
   m_ResponseAssemblerState = RAS_WaitStatusCode;
}

//-----------------------------------------------------------------------------
ResultDH CHttpTask::WriteRequest()
//-----------------------------------------------------------------------------
{
   ResultDH result = DH_NO_ERROR;
   UInt32 bytesProcessed = 0;
   UInt32 bytesLeft = strlen(m_RequestBuffer);
   UInt8* pCurrentPointer = (UInt8*)&m_RequestBuffer[0];

   LOG3(Tools::Logger::lmTrace, "Task{0x%X} is sending a request using Conn{0x%X}. Length: %d",
        this, m_pConn, bytesLeft);

   do
   {
      if (isTimedOut())
      {
         m_pConn->Shutdown(); // Connection is invalid anyway
         result = FormatError(ErrorDH::EC_TimedOut, ErrorDH::ET_AppSpecific,
                              ErrorDH::EP_WtiteRequest,
                              "A task has timed out while WriteRequest");
         break;
      }

      bytesProcessed = 0;
      result = m_pConn->WriteSome(pCurrentPointer, bytesLeft, bytesProcessed);
      if (m_isInterrupted)
      {
         if (result)
         {
            delete result;
            result = 0;
         }

         result = FormatError(ErrorDH::EC_Interrupted, ErrorDH::ET_AppSpecific,
                              ErrorDH::EP_WtiteRequest,
                              "A task has been interrupted while WriteRequest");
         break;
      }

      if (result != DH_NO_ERROR)
      {
         m_pConn->Shutdown();
         break;
      }

      bytesLeft -= bytesProcessed;
      pCurrentPointer += bytesProcessed;
   }while(bytesLeft > 0);

   return result;
}


//-----------------------------------------------------------------------------
ResultDH CHttpTask::ReadResponse()
//-----------------------------------------------------------------------------
{
   ResultDH result = DH_NO_ERROR;
   UInt32 bytesProcessed = 0;
   UInt32 bufCapacity = m_CurrentCapacity;
   UInt8* pCurrentPointer = (UInt8*)&m_ResponseBuffer[0];
   LOG2(Tools::Logger::lmTrace, "Task{0x%X} is reading a response using Conn{0x%X}",
        this, m_pConn);
   do
   {
      if (isTimedOut())
      {
         m_pConn->Shutdown(); // Connection is invalid anyway
         result = FormatError(ErrorDH::EC_TimedOut, ErrorDH::ET_AppSpecific,
                              ErrorDH::EP_ReadResponse,
                              "A task has timed out while ReadResponse");
      }

      bytesProcessed = 0;
      result = m_pConn->ReadSome(pCurrentPointer, bufCapacity, bytesProcessed);
      if (m_isInterrupted)
      {
         if (result)
         {
            delete result;
            result = 0;
         }

         result = FormatError(ErrorDH::EC_Interrupted, ErrorDH::ET_AppSpecific,
                              ErrorDH::EP_ReadResponse,
                              "A task has been interrupted while ReadResponse");
         break;
      }

      if (result != DH_NO_ERROR)
      {
          m_pConn->Shutdown();
          return result;
      }
   }while(AssembleResponse(bytesProcessed, &bufCapacity, &pCurrentPointer));

   return result;
}

//-----------------------------------------------------------------------------
bool CHttpTask::AssembleResponse(UInt32 bytesReceived, UInt32* newBufCapacity, UInt8** pNewStoringLocation)
//-----------------------------------------------------------------------------
{
   char *p = 0;
   char tmpCh = 0;
   char *Start = m_ResponseBuffer;
   char *End = m_ResponseBuffer;
   static UInt16 bytesToReceiveLeft = m_CurrentCapacity;
   bool ContinueAssembling = true;

   //LOG1(Tools::Logger::lmTrace, "Assembling a response. Got %d bytes", bytesReceived);

   if (bytesReceived > 0)
   {
      m_CurrentCapacity -= bytesReceived;
      *newBufCapacity = m_CurrentCapacity;
      *pNewStoringLocation += bytesReceived;
   }

   do
   {
      switch (m_ResponseAssemblerState)
      {
      case RAS_WaitStatusCode:
         {
            if (strstr(Start, HTTP::cHTTP_1_1))
            {
               if (strstr(Start, HTTP::cCRLF))
               {
                  Start = Start + strlen(HTTP::cHTTP_1_1) + 1;
                  const UInt8 StatusCodeLen = 3;
                  End = Start + StatusCodeLen;

                  tmpCh = *End; *End = 0;
                  sscanf(Start, "%d", &m_HttpStatusCode);
                  *End = tmpCh;

                  LOG2(Tools::Logger::lmTrace, "Task{0x%X} has got HTTP status code: %d.",
                       this, m_HttpStatusCode);

                  m_HttpContentLength = 0;
                  m_pContent = 0;
                  bytesToReceiveLeft = 0;

                  m_ResponseAssemblerState = RAS_WaitHeaders;
               }
            }
            else
            {
               if (strlen(Start) >= strlen(HTTP::cHTTP_1_1)) // Unexpected data
               {
                  LOG1(Tools::Logger::lmWarning, "Task{0x%X} has got unexpected data from socket", this);

                  FlushResponseBuffer();
                  *newBufCapacity = m_CurrentCapacity;
                  *pNewStoringLocation = (UInt8*)m_ResponseBuffer;
               }

               return ContinueAssembling;
            }
         }
         break;

      case RAS_WaitHeaders:
         {
            if (p = strstr(Start, HTTP::cCRLFx2))
            {
               LOG(Tools::Logger::lmTrace, "Got all headers.");
               m_pContent = (UInt8*)p + strlen(HTTP::cCRLFx2);

               if (p = strstr(Start, HTTP::Content_Length))
               {
                  Start = p + strlen(HTTP::Content_Length);
                  End = strstr(Start, HTTP::cCRLF);

                  tmpCh = *End; *End = 0;
                  sscanf(Start, "%d", &m_HttpContentLength);
                  *End = tmpCh;

                  LOG2(Tools::Logger::lmTrace, "Task{0x%X} has got Content-Length header: %d.",
                       this, m_HttpContentLength);
                  m_ResponseAssemblerState = RAS_WaitContent;
               }
               else // Response with no content
               {
                  LOG1(Tools::Logger::lmTrace, "Task{0x%X}: No Content-Length header: response seems to be complited.", this);
                  m_ResponseAssemblerState = RAS_Complited;
               }
            }
            else
            {
               if (m_CurrentCapacity == 0)
               {
                  LOG1(Tools::Logger::lmWarning, "Task{0x%X}: Buffer overflow detected while reading HTTP headers.", this);

                  FlushResponseBuffer();
                  *newBufCapacity = m_CurrentCapacity;
                  *pNewStoringLocation = (UInt8*)m_ResponseBuffer;
                  m_ResponseAssemblerState = RAS_Overflow; // Will read the socket out until the first read operation's timeout
               }

               return ContinueAssembling;
            }
         }
         break;

      case RAS_WaitContent:
         {
            UInt32 CurrentRespLen = strlen((char*)m_pContent);

            if (m_HttpContentLength > CurrentRespLen)
            {
               bytesToReceiveLeft = m_HttpContentLength - CurrentRespLen;
               if(bytesToReceiveLeft > m_CurrentCapacity)
               {
                  LOG3(Tools::Logger::lmWarning, "Task{0x%X}: No room %d for the remaining %d bytes of response.",
                       this, m_CurrentCapacity, bytesToReceiveLeft);

                  FlushResponseBuffer();
                  *newBufCapacity = m_CurrentCapacity;
                  *pNewStoringLocation = (UInt8*)m_ResponseBuffer;
                  m_ResponseAssemblerState = RAS_Overflow; // Will read the socket out until the end of current response
               }

               return ContinueAssembling;
            }
            else if (m_HttpContentLength < CurrentRespLen)
            {
               LOG1(Tools::Logger::lmWarning, "Task{0x%X} has got longer response than expected!", this);
               LOG3(Tools::Logger::lmWarning, "Task{0x%X}: Expected length: %d, current length: %d",
                    this, m_HttpContentLength, CurrentRespLen);

               *(m_pContent + m_HttpContentLength) = 0; // Cut the response

               m_ResponseAssemblerState = RAS_Complited;
            }
            else if (m_HttpContentLength == CurrentRespLen)
            {
               m_ResponseAssemblerState = RAS_Complited;
            }
         }
         break;

      case RAS_Complited:
         {
            LOG3(Tools::Logger::lmTrace, "Task{0x%X} has got Response! Total length: %d, content length: %d",
                 this, strlen(m_ResponseBuffer), m_HttpContentLength);
            ContinueAssembling = false;
         }
         break;

      case RAS_Overflow:
         {
            LOG1(Tools::Logger::lmWarning, "Task{0x%X}: Response buffer overflow.", this);
            LOG3(Tools::Logger::lmWarning, "Task{0x%X}: Expecting the remainig %d bytes of %d-length response.",
                 this, bytesToReceiveLeft, m_HttpContentLength);

            if (bytesReceived > 0)
            {
               LOG3(Tools::Logger::lmWarning, "Task{0x%X} has got %d bytes of %d expecting.",
                    this, bytesReceived, bytesToReceiveLeft);

               if (bytesToReceiveLeft > 0 && m_HttpContentLength > 0)
               {
                  bytesToReceiveLeft -= bytesReceived;
                  if (bytesToReceiveLeft == 0)
                  {
                     LOG1(Tools::Logger::lmWarning, "Task{0x%X}: The socket has been completely read out.", this);
                     FlushResponseBuffer();
                     m_pContent = 0;
                     ContinueAssembling = false;
                  }
               }

               LOG1(Tools::Logger::lmWarning, "Task{0x%X} is reading out the response.", this);
               FlushResponseBuffer(); // Do not care about the buffer and read the socket out till the end
               *newBufCapacity = m_CurrentCapacity;
               *pNewStoringLocation = (UInt8*)m_ResponseBuffer;
               m_pContent = 0;
            }
            else
            {
               if (bytesToReceiveLeft == 0 && m_HttpContentLength == 0)
               {
                  LOG1(Tools::Logger::lmWarning, "Task{0x%X}: The socket has been completely read out.", this);
                  ContinueAssembling = false;
               }
            }

            return ContinueAssembling;
         }
         break;
      }
   }while (ContinueAssembling);

   return ContinueAssembling;
}


//-----------------------------------------------------------------------------
UInt32 CHttpTask::Run()
//-----------------------------------------------------------------------------
{
   LOG1(Tools::Logger::lmTrace, "Task{0x%X}: Running...", this);
   m_StartTime = GetTickCount();
   ResultDH result = DH_NO_ERROR;

   if (!m_pConn->IsConnected())
   {
      result = m_pConn->Connect(m_Timoeut);
   }

   if (m_isInterrupted)
   {
      result = FormatError(ErrorDH::EC_Interrupted, ErrorDH::ET_AppSpecific,
                           ErrorDH::EP_SocketConnect,
                           "A task has been interrupted while connecting");
   }

   if (result != DH_NO_ERROR)
      return (UInt32)result;

   UInt32 cAtomicOperationTimeoutMs = 0;
   if (m_pConn->IsSecure())
      cAtomicOperationTimeoutMs = m_Timoeut + 500;
   else
      cAtomicOperationTimeoutMs = m_Timoeut;

   result = m_pConn->SetAtomicWriteTimeout(cAtomicOperationTimeoutMs);
   if (result != DH_NO_ERROR)
      return (UInt32)result;

   result = WriteRequest();
   if (result != DH_NO_ERROR)
      return (UInt32)result;

   FlushResponseBuffer();

   result = m_pConn->SetAtomicReadTimeout(cAtomicOperationTimeoutMs);
   if (result != DH_NO_ERROR)
      return (UInt32)result;

   result = ReadResponse();
   return (UInt32)result;
}


//-----------------------------------------------------------------------------
void CHttpTask::ExitInstance(UInt32 result)
//-----------------------------------------------------------------------------
{
   if (result == DH_NO_ERROR)
   {
      LOG1(Tools::Logger::lmInfo, "HttpTask{0x%X} has completed successfully", this);
   }
   else
   {
      LOG1(Tools::Logger::lmError, "HttpTask{0x%X} has completed with an error.", this);
   }

   UInt32 threadID = GetCurrentThreadId();

   if (m_RestListener && m_OnTaskComplitedCallback)
   {
      TaskListener* L = m_RestListener;
      TaskListener::TaskCallback cb = m_OnTaskComplitedCallback;
      m_RestListener = 0;
      m_OnTaskComplitedCallback = 0;
      (L->*cb)((ResultDH)result, this);  // WARN: Do not refer to any task member after calling this callback!!!
                               // The task is not valid (destroyed) since this moment!
   }
   else if (m_RestListener && m_OnTaskWithObjectComplitedCallback)
   {
      TaskListener* L = m_RestListener;
      TaskListener::TaskWithObjectCallback cb = m_OnTaskWithObjectComplitedCallback;
      m_RestListener = 0;
      m_OnTaskComplitedCallback = 0;
      (L->*cb)((ResultDH)result, this, m_ObjectToPostProcess);  // WARN: Do not refer to any task member after calling this callback!!!
                                                      // The task is not valid (destroyed) since this moment!
   }

   LOG1(Tools::Logger::lmTrace, "Task{0x%X}: Finishing...", this);
}


//-----------------------------------------------------------------------------
void CHttpTask::Stop()
//-----------------------------------------------------------------------------
{
   m_isInterrupted = true;
   if (m_pConn)
   {
      m_pConn->Shutdown();
   }
}

