/*-----------------------------------------------------------------------------

Autor                                                           Datum
Nikolay Peganov <Nikolay.Peganov@dataart.com>                   17.11.2014

-- BESCHREIBUNG ---------------------------------------------------------------


-- AENDERUNGEN ----------------------------------------------------------------

Autor                  Datum

-----------------------------------------------------------------------------*/

#include "HttpClient.h"

using namespace DH;

CHttpClient* CHttpClient::m_pSingletone = 0;
//-----------------------------------------------------------------------------
CHttpClient* CHttpClient::GetInstance()
//-----------------------------------------------------------------------------
{
   if (0 == m_pSingletone)
   {  // Create instance
      m_pSingletone = new CHttpClient;
   }
   
   // deliver reference to instance
   return m_pSingletone;
}

//-----------------------------------------------------------------------------
CHttpClient::CHttpClient()
: m_TasksCount(0)
, m_ConnectionsCount(0)
, m_ConnCacheMutex(Tools::CCriticalSection())
//-----------------------------------------------------------------------------
{
   for(size_t i = 0; i < cCacheSize; ++i)
   {
      m_ActiveTasks[i] = 0;
      m_ConnectionCache[i] = 0;
      m_ResolverCache[i] = 0;
   }
}

//-----------------------------------------------------------------------------
CHttpClient::~CHttpClient()
//-----------------------------------------------------------------------------
{
   Stop();
}

//-----------------------------------------------------------------------------
void CHttpClient::Stop()
//-----------------------------------------------------------------------------
{
   LOG(Tools::Logger::lmError, "Stopping HttpClient"); 
   
   for(size_t i = 0; i < cCacheSize; ++i)
   {  
      if (m_ActiveTasks[i])
      {
         LOG2(Tools::Logger::lmError, "Stopping and deleting active HttTask{0x%X}, count(%d)",
              m_ActiveTasks[i], --m_TasksCount);             
         m_ActiveTasks[i]->Stop();
         
         // WARN: Do not delete tasks. Let them to be comleted instead.
         
         //delete m_ActiveTasks[i];
         //m_ActiveTasks[i] = 0;      
      }
       
      if (m_ConnectionCache[i])
      {
         LOG2(Tools::Logger::lmError, "Deleting Connection{0x%X} from cache(%d)",
              m_ConnectionCache[i], --m_ConnectionsCount);               
         
         m_ConnectionCache[i]->Stop();
         
         // WARN: Do not delete the ConnectionGuard here.
         // Just interrupt it and it will be deleted in OnConnectionExpired method
         
         //delete m_ConnectionCache[i];
         //m_ConnectionCache[i] = 0;   
      }

      if (m_ResolverCache[i])
      {
         LOG1(Tools::Logger::lmError, "Deleting Endpoint{0x%X} from resolver cache",
             m_ResolverCache[i]);           
         delete m_ResolverCache[i];
         m_ResolverCache[i] = 0;      
      }      
   }   
}

//-----------------------------------------------------------------------------
CHttpTask* CHttpClient::CreateTask()
//-----------------------------------------------------------------------------
{
   for(size_t i = 0; i < cCacheSize; ++i)
   {
      if(!m_ActiveTasks[i])
      {
         m_ActiveTasks[i] = new CHttpTask();      
         LOG1(Tools::Logger::lmInfo, "Http has created Task{0x%X}", m_ActiveTasks[i]);   
         LOG1(Tools::Logger::lmTrace, "Tasks (%d).", ++m_TasksCount);               
         return m_ActiveTasks[i];       
      }
   }  
   
   return 0;   
}

//-----------------------------------------------------------------------------
CHttpTask* CHttpClient::NewPOST(const char* url)
//-----------------------------------------------------------------------------
{
   CHttpTask* task = CreateTask();
   if (task)
   {
      task->Init(RT_POST, url);            
   }
   return task;
}

//-----------------------------------------------------------------------------
CHttpTask* CHttpClient::NewGET(const char* url)
//-----------------------------------------------------------------------------
{
   CHttpTask* task = CreateTask();
   if (task)
   {
      task->Init(RT_GET, url);            
   }
   return task;
}

//-----------------------------------------------------------------------------
CHttpTask* CHttpClient::NewPUT(const char* url)
//-----------------------------------------------------------------------------
{
   CHttpTask* task = CreateTask();
   if (task)
   {
      task->Init(RT_PUT, url);            
   }
   return task;   
}

//-----------------------------------------------------------------------------
CHttpTask* CHttpClient::NewDELETE(const char* url)
//-----------------------------------------------------------------------------
{
   CHttpTask* task = CreateTask();
   if (task)
   {
      task->Init(RT_DELETE, url);            
   }
   return task;   
}


//-----------------------------------------------------------------------------
ResultDH CHttpClient::PrepareTask(CHttpTask* task)
//-----------------------------------------------------------------------------
{
   const char* TaskHost = task->GetHost();
   _ip_address ip = 0;
   
   // Get an IP address from resolver cache
   ResultDH result = GetIPForHostName(TaskHost, ip);
   if (result != DH_NO_ERROR)
   {
      return result; // DNS resolver error
   }  
   
   // Get an appropriate from cache
   m_ConnCacheMutex.Lock();
   for(size_t i = 0; i < cCacheSize; ++i)
   {
      if(m_ConnectionCache[i])
      {
         if (m_ConnectionCache[i]->Match(ip, task->GetPort(), task->NeedsSecure()))
         {
            CConnection* pConn = m_ConnectionCache[i]->TakeConnection();
            task->SetConnection(pConn);
            LOG2(Tools::Logger::lmTrace, "Got a Connection{0x%X} from cache, pos. %d.", pConn, i);                  
            break;            
         }
      }
   } 
   m_ConnCacheMutex.Unlock();   
   
   // No appropriate in cache -> Create a new one.
   if (task->GetConnection() == 0)
   {
      if (!task->NeedsSecure())
      {
         task->SetConnection(new SimpleConnection(ip, task->GetPort()));
      }
      else
      {
         task->SetConnection(new SecureConnection(ip, task->GetPort()));     
      }  
   }  
   
   return DH_NO_ERROR;
}

//-----------------------------------------------------------------------------
ResultDH CHttpClient::GetIPForHostName(const char* hostName, _ip_address& ip)
//-----------------------------------------------------------------------------
{
   for(size_t i = 0; i < cCacheSize; ++i)
   {
      if(m_ResolverCache[i])
      {
         if (!strcmp(hostName, m_ResolverCache[i]->HostName))
         {
            ip = m_ResolverCache[i]->IP;
            break;
         }
      }
   }
   
   if (ip == 0) // IP for the task host has not been found in the cache
   {
      HOSTENT_STRUCT_PTR host = gethostbyname((char*)hostName);  
      if (host)
      {
         ip = *(uint32_t*)host->h_addr_list[0];
         LOG5(Tools::Logger::lmTrace, "Host name %s has been successfully resolved to IP: %d.%d.%d.%d!", 
              hostName,
              (ip >> 24) & 0xFF, 
              (ip >> 16) & 0xFF, 
              (ip >> 8) & 0xFF, 
              ip & 0xFF);   
      }        
      else
      {
         char msg[128 + 1];
         sprintf(msg, "Failed to resolve host name %s.", hostName);
         LOG(Tools::Logger::lmError, msg);
         
         return FormatError(ErrorDH::EC_DnsResolverError, ErrorDH::ET_AppSpecific, 
                            ErrorDH::EP_DnsResolving, msg);
      }
      
      for(size_t i = 0; i < cCacheSize; ++i)
      {
         if(!m_ResolverCache[i])
         {
            LOG1(Tools::Logger::lmTrace, "Storing IP for host name %s into cache.", hostName);             
            m_ResolverCache[i] = new Endpoint;
            strcpy(m_ResolverCache[i]->HostName, hostName);
            m_ResolverCache[i]->IP = ip;
            break;
         }
                 
         if (i == cCacheSize - 1)
         {
            LOG1(Tools::Logger::lmTrace, "No cell for storing IP for host name %s in cache.", hostName);             
            LOG(Tools::Logger::lmTrace, "Storing it into the first cell.");                         
            strcpy(m_ResolverCache[0]->HostName, hostName);
            m_ResolverCache[0]->IP = ip;            
         }
      }      
   }   

   return DH_NO_ERROR;  
}

//-----------------------------------------------------------------------------
ResultDH CHttpClient::AsyncSend(CHttpTask* task, TaskListener* listener, 
                                TaskListener::TaskCallback Callback, UInt32 timeoutMs,
                                const char* taskName)
//-----------------------------------------------------------------------------
{
   ResultDH result = PrepareTask(task);
   if(result != DH_NO_ERROR)
   {
      return result;   
   }
   
   task->m_RestListener = listener;
   task->m_OnTaskComplitedCallback = Callback;    
   task->m_Timoeut = timeoutMs;   
  
   task->AsyncRun(taskName);
   
   return DH_NO_ERROR;   
}

//-----------------------------------------------------------------------------
ResultDH CHttpClient::AsyncSendWithObject(CHttpTask* task, TaskListener* listener, 
                                          TaskListener::TaskWithObjectCallback Callback,
                                          UInt32 timeoutMs, void* objectToPostProcess, 
                                          const char* taskName)
//-----------------------------------------------------------------------------                              
{
   ResultDH result = PrepareTask(task);
   if(result != DH_NO_ERROR)
   {
      return result;   
   }   
   
   task->m_RestListener = listener;
   task->m_OnTaskWithObjectComplitedCallback = Callback;    
   task->m_ObjectToPostProcess = objectToPostProcess;  
   task->m_Timoeut = timeoutMs;     
   
   task->AsyncRun(taskName);
   
   return DH_NO_ERROR;  
}


//-----------------------------------------------------------------------------
bool CHttpClient::ReleaseTask(CHttpTask* task)
//-----------------------------------------------------------------------------
{
   bool taskFreed = false;
   if (task)
   {
      LOG1(Tools::Logger::lmInfo, "Http is releasing Task{0x%X}", task);         
      CConnection* pTaskConn = task->GetConnection();
      if (pTaskConn && pTaskConn->IsConnected())
      {
         // Put the connection into cache
         for(size_t i = 0; i < cCacheSize; ++i)
         {
            m_ConnCacheMutex.Lock();
            if(!m_ConnectionCache[i])
            {
               LOG3(Tools::Logger::lmTrace, "Storing a Connection{0x%X} into cache (%d), pos %d.", pTaskConn, ++m_ConnectionsCount, i);   
               char GuardName[32];
               sprintf(GuardName, "Guard for Conn{0x%X}", pTaskConn);

               CConnectionGuard* pGuard = new CConnectionGuard();
               pGuard->Guard(pTaskConn, cIdleConnectionTimeMs, this,
                   (CConnectionGuard::GuardListener::ExpirationCallback)&CHttpClient::OnConnectionExpired,
                   GuardName);
               
               m_ConnectionCache[i] = pGuard;
               break;
            }
            m_ConnCacheMutex.Unlock();
            
            if (i == cCacheSize - 1)
            {
               LOG(Tools::Logger::lmWarning, "No cell for storing a Connection in cache.");                         
               LOG(Tools::Logger::lmTrace, "Destroy it!");      
               delete pTaskConn;
               pTaskConn = 0;    
            }            
         }                  
      }
      else if (pTaskConn)
      {
         LOG1(Tools::Logger::lmTrace, "Destroying useless Connection{0x%X}.", pTaskConn);           
         delete pTaskConn;
         pTaskConn = 0;                        
      }
      
      // Remove the task from active tasks and destroy it.
      for(size_t i = 0; i < cCacheSize; ++i)
      {
         if (m_ActiveTasks[i] == task)
         {
            LOG1(Tools::Logger::lmInfo, "Http is releasing Task{0x%X}", task);    
            delete m_ActiveTasks[i];
            m_ActiveTasks[i] = 0;     
            taskFreed = true;
            task = 0;
            LOG1(Tools::Logger::lmTrace, "Tasks (%d).", --m_TasksCount);                   
            break;
         }
      }  
               
      if (task)
      {
         LOG1(Tools::Logger::lmWarning, "Zombie Task{0x%X} has been detected! Deleting it.", task);   
         delete task;
         task = 0;
      }      
   }
   
   return taskFreed;
}

//-----------------------------------------------------------------------------
void CHttpClient::OnConnectionExpired(UInt32 result, CConnectionGuard* pGuard)
//-----------------------------------------------------------------------------
{
   if (result == RTCS_OK)
   {
      LOG1(Tools::Logger::lmInfo, "ConnectionGuard{0x%X} has been interrupted", pGuard);   
   }
   else if (result == RTCSERR_TCP_TIMED_OUT)
   {
      LOG1(Tools::Logger::lmInfo, "ConnectionGuard{0x%X} has expired", pGuard);   
   }
   else
   {
      LOG2(Tools::Logger::lmError, "ConnectionGuard{0x%X} has completed with error, code: 0x%05X", pGuard, result);             
   }   
   
   if (pGuard)
   {
      m_ConnCacheMutex.Lock();
      for(size_t i = 0; i < cCacheSize; ++i)
      {
         if(m_ConnectionCache[i] == pGuard)
         {
            LOG3(Tools::Logger::lmTrace, "Removing a ConnectionGuard{0x%X} from cache (%d), pos.%d.", pGuard, --m_ConnectionsCount, i);             
            m_ConnectionCache[i] = 0;
            break;
         }
      } 
      m_ConnCacheMutex.Unlock();
      
      delete pGuard;
      pGuard = 0;
   }
}
