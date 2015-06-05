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

#include "Connection.h"
#include "NonCopyable.h"

#include "Namespace.h"

enum ReqTypeEnum
{
   RT_POST,
   RT_GET,
   RT_PUT,
   RT_DELETE
};

class CHttpClient;
class CHttpTask : public Tools::CThread
                , private NonCopyable
{
   friend class CHttpClient;   
public:
   class TaskListener
   {
   public:
      typedef void (TaskListener::* TaskCallback)(ResultDH result, CHttpTask* task);        
      typedef void (TaskListener::* TaskWithObjectCallback)(ResultDH result, CHttpTask* task, void* objectToPostProcess);           
   }; 
   
private:
   enum ResponseAssemblerStateEnum
   {
      RAS_WaitStatusCode,
      RAS_WaitHeaders,
      RAS_WaitContent,
      RAS_Complited,
      RAS_Overflow
   };
   
public:
   static const size_t cRequestBufferSize = 1024 * 2;
   static const size_t cResponseBufferSize = 1024 * 2;   

public:
   CHttpTask();      
   virtual ~CHttpTask();
   
public:
   void AsyncRun(const char* taskName = 0);   
   void Stop();
   
public: // Request construction
   void AddPath(const char* path);
   void QueryPath(const char* query, const char* value);   
   void FinishUrl();      
   void AddHeader(const char* header, const char* value); 
   void SetContent(char* content);   
   void Finalize();   
   
public: // Response accessors
   bool IsInterrupted();      
   bool ResponseIsComplited();       
   bool HttpStatusIsSuccessfull();    
   UInt16 GetHttpStatus(); 
   UInt16 GetResponseState();       
   UInt8* GetResponseContent();  
   
private:
   void ParseUrl(char* url);   
   
private:  
   // Thread Functions
   virtual UInt32 Run();
   virtual void ExitInstance(){};
   virtual void ExitInstance(UInt32 result);     
   
private:  
   bool isTimedOut();
   void FlushRequestBuffer();
   void FlushResponseBuffer();   
   bool AssembleResponse(UInt32 bytesReceived, UInt32* newBufCapacity, UInt8** pNewStoringLocation);
   
   ResultDH WriteRequest();
   ResultDH ReadResponse();    

private: //Should be accessaable only by HttpClient (friend class)
   void Init(ReqTypeEnum Type, const char* url);  
   const char* GetHost();
   bool NeedsSecure();
   UInt16 GetPort();
   void SetConnection(CConnection* conn);
   CConnection* GetConnection();   
   bool HasContent();
      
private: // Members, accessaable only by HttpClient (friend class)
   TaskListener* m_RestListener;
   TaskListener::TaskCallback m_OnTaskComplitedCallback;  
   TaskListener::TaskWithObjectCallback m_OnTaskWithObjectComplitedCallback;
   
   void* m_ObjectToPostProcess;
   UInt32 m_Timoeut;   
   
   char m_RequestBuffer[cRequestBufferSize];
   char m_ResponseBuffer[cResponseBufferSize];   
   
   CConnection* m_pConn;
   
private: // URL info
   char* m_pHost;
   UInt16 m_Port;
   
   static const UInt8 m_cProtoSize = 6;   
   char m_Proto[m_cProtoSize];

private: // Task state
   bool m_isActive; 
   bool m_hasContent;
   bool m_isInterrupted; 
   UInt16 m_CurrentCapacity;
   UInt32 m_StartTime;
   UInt8 m_RetryCounter;          
   
private: // Response data
   ResponseAssemblerStateEnum m_ResponseAssemblerState;   
   UInt16 m_HttpStatusCode;     
   UInt32 m_HttpContentLength;      
   UInt8* m_pContent;       
   
private: // Constans 
   static const UInt16 m_cDefaultPort = 80;  
   
   static const UInt32 m_cTaskPrio = 10;
   static const UInt32 m_cTaskStackSize = 18000;  
   
   static const UInt8 cMaxRetry = 1;  
};

#include "NamespaceEnd.h"