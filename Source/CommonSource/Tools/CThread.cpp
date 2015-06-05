/*-----------------------------------------------------------------------------
Autor                  Datum
E. Fitze               14.06.2013

-- BESCHREIBUNG ---------------------------------------------------------------


-- AENDERUNGEN ----------------------------------------------------------------

Autor                  Datum

-----------------------------------------------------------------------------*/
#include "CThread.h"

#include "MQX.h"

#include "Logger.h"


using namespace Tools;
//-----------------------------------------------------------------------------


//-----------------------------------------------------------------------------
void CThread::ThreadProc(UInt32 lpParameter)
//-----------------------------------------------------------------------------
{
   CThread* pThread  = NULL;

   // Cast back to the thread object
   pThread = reinterpret_cast<CThread*>(lpParameter);
   //pThread = (CThread*)lpParameter;

   // Let the thread run
   if (pThread != NULL)
   {
      pThread->m_running = TRUE;

      // Initialize the thread
      if (pThread->InitInstance() == true)
      {
         // Initialization succeeded, allow it to run
         UInt32 result = pThread->Run();

         // Exit the thread
         pThread->ExitInstance(result);
      }

      _task_abort(GetCurrentThreadId()); // end of life...
   }
}


// Construction / Destruction
//-----------------------------------------------------------------------------
CThread::CThread()
: m_TaskId(0)
, m_lastCall(0)
, m_running(0)
//-----------------------------------------------------------------------------
{
}

//-----------------------------------------------------------------------------
CThread::~CThread()
//-----------------------------------------------------------------------------
{
}


//-----------------------------------------------------------------------------
bool CThread::InitInstance()
//-----------------------------------------------------------------------------
{
   return true;
}

//-----------------------------------------------------------------------------
void CThread::ExitInstance()
//-----------------------------------------------------------------------------
{
}

//-----------------------------------------------------------------------------
void CThread::ExitInstance(UInt32 result)
//-----------------------------------------------------------------------------
{
}

//-----------------------------------------------------------------------------
UInt32 CThread::Run()
//-----------------------------------------------------------------------------
{
   return 0;
}


//-----------------------------------------------------------------------------
bool CThread::SetThreadPriority(UInt8 nPrio)
//-----------------------------------------------------------------------------
{
   UInt32 mqxRet;
   bool bRetVal = IsRunning();

   if (bRetVal == true)
   {
      mqxRet = _task_set_priority(m_TaskId, nPrio, NULL);
      return mqxRet == MQX_OK;
   }
   return FALSE;
}

//-----------------------------------------------------------------------------
UInt8 CThread::GetThreadPriority() const
//-----------------------------------------------------------------------------
{
   return 0;
}

//-----------------------------------------------------------------------------
bool CThread::CreateThread(char* name, UInt8 ui8Prio, UInt32 stackSize)
//-----------------------------------------------------------------------------
{
   // Check that this instance doesn't have a running thread attached
   if (IsRunning() == false)
   {
      TASK_TEMPLATE_STRUCT  taskTempl;
      UInt32                tec;

      m_ticksPerSecond = _time_get_ticks_per_sec();
      m_HWticksPerTicks = _time_get_hwticks_per_tick();
      m_HWticksPerMicroSecond = m_ticksPerSecond * m_HWticksPerTicks / 1000000;

      // preserve this task's error code
      tec = _task_get_error();
      _mem_zero((void*)&taskTempl, sizeof(taskTempl));
      taskTempl.TASK_ADDRESS       = (TASK_FPTR)ThreadProc;
      taskTempl.TASK_STACKSIZE     = stackSize;
      taskTempl.TASK_PRIORITY      = ui8Prio;
      taskTempl.TASK_NAME          = name;
      taskTempl.CREATION_PARAMETER = (UInt32)static_cast<CThread*>(this);
      m_TaskId = _task_create(0, 0, (UInt32)&taskTempl);
      if (m_TaskId == MQX_NULL_TASK_ID)
      {
         //err = _task_get_error();
      }

      // restore the task error code
      _task_set_error(tec);

      return m_TaskId != MQX_NULL_TASK_ID;
   }

   return FALSE;
}

void CThread::LogTaskInfo(void)
//-----------------------------------------------------------------------------
{

   UInt32 tick;
#if MQX_VERSION < 411
   uint_32 stack_size, stack_used, taskID;
#else
   UInt32 stack_size, stack_used, taskID;
#endif
   TASK_TEMPLATE_STRUCT * pTaskTempl;

   tick = GetTickCount();
   if (tick - m_lastCall > 5000)
   {  // es wird nur 1 mal in 5 Sekunden geloggt.
      m_lastCall = tick;
      taskID = _task_get_id();
      pTaskTempl = _task_get_template_ptr(taskID);
      // Get stack usage for this task:
      _klog_get_task_stack_usage(_task_get_id(), &stack_size, &stack_used);
      LOG4(Tools::Logger::lmPerformance, "Task <%s> STACK: Size: %d, Used: %d, %d\%", pTaskTempl->TASK_NAME, stack_size, stack_used, 100*stack_used/stack_size);
   }
}

//-----------------------------------------------------------------------------
bool CThread::Suspend()
//-----------------------------------------------------------------------------
{
   bool bRetVal = IsRunning();

   if (bRetVal == true)
   {
   }

   return bRetVal;
}


//-----------------------------------------------------------------------------
bool CThread::Resume()
//-----------------------------------------------------------------------------
{
   bool bRetVal = IsRunning();

   if (bRetVal == true)
   {
   }

   return bRetVal;
}

//-----------------------------------------------------------------------------
bool CThread::Terminate(UInt32 dwExitCode)
//-----------------------------------------------------------------------------
{
   // Return value
   bool bRetVal = IsRunning();

   // Are we running?
   if (bRetVal == true)
   {
      // Yes, we are running - terminate the thread
   }

   return bRetVal;
}

//-----------------------------------------------------------------------------
void CThread::Sleep(UInt32 dwTimeout)
//-----------------------------------------------------------------------------
{
   // Let the calling thread sleep for this time in ms
   _time_delay(dwTimeout);
}

//-----------------------------------------------------------------------------
void CThread::Wait(UInt32 dwMycroSeconds)
//-----------------------------------------------------------------------------
{
   UInt32 t1, t2;
   UInt32 delay = dwMycroSeconds * m_HWticksPerMicroSecond;
   t1 = t2 = _time_get_hwticks();
   while ((t2 - t1) < delay)
   {
      t2 = _time_get_hwticks();
   }
}

//-----------------------------------------------------------------------------
UInt32 CThread::GetTicksHW()
//-----------------------------------------------------------------------------
{
   return _time_get_hwticks();
}
//-----------------------------------------------------------------------------
UInt32 CThread::TicksHW2MicroSecond(UInt32 ticksHW)
//-----------------------------------------------------------------------------
{
   return ticksHW / m_HWticksPerMicroSecond;
}
//-----------------------------------------------------------------------------
UInt32 CThread::GetTickCount()
//-----------------------------------------------------------------------------
{
   TIME_STRUCT     time;
   _time_get_elapsed(&time);
   return 1000*time.SECONDS + time.MILLISECONDS;
}

//-----------------------------------------------------------------------------
Bool CThread::TimeExpired(UInt32 startTime, UInt32 timeout)
//-----------------------------------------------------------------------------
{
   UInt32 delta;
   UInt32 actTime = GetTickCount();
   if (actTime >= startTime)
   {
      delta = actTime - startTime;
   }
   else
   {
      delta = (UInt32)(-1) - startTime + actTime + 1;
   }
   
   return delta > timeout;
}



//-----------------------------------------------------------------------------
bool CThread::IsRunning() const
//-----------------------------------------------------------------------------
{
   return m_running;
}

//-----------------------------------------------------------------------------
UInt32 CThread::GetCurrentThreadId()
//-----------------------------------------------------------------------------
{
   return _task_get_id();
}



