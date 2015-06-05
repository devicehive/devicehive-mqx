/*-----------------------------------------------------------------------------

Autor                                                           Datum
Nikolay Peganov <Nikolay.Peganov@dataart.com>                   04.12.2014

-- BESCHREIBUNG ---------------------------------------------------------------


-- AENDERUNGEN ----------------------------------------------------------------

Autor                  Datum

-----------------------------------------------------------------------------*/
#include "string.h"

#include "CommandExecutor.h"
#include "HttpClient.h"

using namespace DH;

// Construction
//-----------------------------------------------------------------------------
CCommandExecutor::CCommandExecutor()
: m_AppListener(0)
, m_OnExecutionComplitedCallback(0)
, m_jCommand(0)
//-----------------------------------------------------------------------------
{
   LOG1(Tools::Logger::lmInfo, "CommandExecutor{0x%X} created", this);
}

//-----------------------------------------------------------------------------
CCommandExecutor::~CCommandExecutor()
//-----------------------------------------------------------------------------
{
   LOG1(Tools::Logger::lmInfo, "CommandExecutor{0x%X} destroyed", this);
}

//-----------------------------------------------------------------------------
void CCommandExecutor::AsyncExecute(ExecutorListener* listener, ExecutorListener::ExecutionCallback Callback, cJSON* jCommand)
//-----------------------------------------------------------------------------
{
   m_AppListener = listener;
   m_OnExecutionComplitedCallback = Callback;
   m_jCommand = jCommand;

   LOG(Tools::Logger::lmInfo, "Starting an CommandExecutor");
   CreateThread("CommandExecutorTask", m_cTaskPrio, m_cTaskStackSize);
}

//-----------------------------------------------------------------------------
void CCommandExecutor::ExitInstance(UInt32 result)
//-----------------------------------------------------------------------------
{
   if (result == MQX_OK)
   {
      LOG(Tools::Logger::lmInfo, "CommandExecutor has completed successfully");
   }
   else
   {
      LOG1(Tools::Logger::lmError, "CommandExecutor has completed with error, code: 0x%05X", result);
   }

   UInt32 ThreadID = GetCurrentThreadId();

   if (m_AppListener && m_OnExecutionComplitedCallback)
   {
      ExecutorListener* L = m_AppListener;
      ExecutorListener::ExecutionCallback cb = m_OnExecutionComplitedCallback;

      m_AppListener = 0;
      m_OnExecutionComplitedCallback = 0;

      (L->*cb)(result, m_jCommand, this); // WARN: Do not refer to any task member after calling this callback!!!
                                          // The task is not valid (destroyed) since this moment!
   }

   LOG1(Tools::Logger::lmTrace, "CommandExecutor{0x%X}: Finishing...", this);
}
