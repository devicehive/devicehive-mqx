/*-----------------------------------------------------------------------------

Autor                                                           Datum
Nikolay Peganov <Nikolay.Peganov@dataart.com>                   04.12.2014

-- BESCHREIBUNG ---------------------------------------------------------------


-- AENDERUNGEN ----------------------------------------------------------------

Autor                  Datum

-----------------------------------------------------------------------------*/
#pragma once
#include "MQX.h"
#include "ToolsLib.h"
extern "C"
{
#include "cJSON.h"
}

#include "NonCopyable.h"
#include "Namespace.h"

//class CApplication;
class CCommandExecutor : public Tools::CThread
                       , private NonCopyable
{
public:
   class ExecutorListener
   {
   public:
      typedef void (ExecutorListener::* ExecutionCallback)(UInt32 result, cJSON* jCommand, CCommandExecutor* pExecutor);        
   }; 

public:
   CCommandExecutor();      
   virtual ~CCommandExecutor();
   
public:
   void AsyncExecute(ExecutorListener* listener, ExecutorListener::ExecutionCallback Callback, cJSON* jCommand);   

protected:  
   // Thread Functions
   virtual UInt32 Run() = 0;
   virtual void ExitInstance(){};
   virtual void ExitInstance(UInt32 result);     
      
protected: // Members
   ExecutorListener* m_AppListener;
   ExecutorListener::ExecutionCallback m_OnExecutionComplitedCallback; 
   cJSON* m_jCommand;

protected: // Constans   
   static const UInt32 m_cTaskPrio = 10;
   static const UInt32 m_cTaskStackSize = 4000;  
};

#include "NamespaceEnd.h"