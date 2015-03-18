/*-----------------------------------------------------------------------------
Autor                  Datum
E. Fitze               14.06.2013

-- BESCHREIBUNG ---------------------------------------------------------------


-- AENDERUNGEN ----------------------------------------------------------------

Autor                  Datum

-----------------------------------------------------------------------------*/
#pragma once

#include "CommonTypes.h"

#include "Namespace.h"

/**
  CThread is the thread class. To create threads,
  you should derive a class from CThread and overload at least the Run method.

  class CSampleThread : public ::Tools::CThread
  {
  public:
 		CSampleThread();
		~CSampleThread();

		// This function is called to actually run the thread
		UInt32 Run();
  };
**/

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
class CThread
{
private:
   // Copy construction not supported
   CThread(const CThread& rhs);

   //Assignment not supported
   const CThread& operator=(const CThread& rhs);

public:
   // Construction / Destruction
   CThread();

   virtual ~CThread();

public:
   // Virtual Functions

   // Initialization function
   // This is the first function called on a thread object. The function
   // is actually already called from the thread itself. Its purpose is
   // to provide a central initialization point within the thread.
   // return: To allow the thread to run the default implementation of this function returns true.
   // If you override this function you can control the thread start by returning true or false.
   // A return value of false	causes the thread to be terminated immediately.
   virtual bool InitInstance();

   // Thread exit function
   // This function is called from within the thread just before it terminates.
   // The purpose is to provide a central cleanup place for thread specific data.
   virtual void ExitInstance();
   virtual void ExitInstance(UInt32 result);   

   // Thread procedure
   // This is the main thread procedure. All thread specific code except initialization
   // and uninitialization should go here.
   // The return value specifies the thread exit code.
   virtual UInt32 Run();

   void LogTaskInfo(void);

public:
   // Thread Functions

   // Creates the thread
   // Creates the thread corresponding to the object instance.
   // The instance must not manage an existing thread.
   // Parameter:
   //   name is used for the threadname.
   //   ui8Prio contains the thread priority from 1 to 255.
   //   stackSize
   // The return value is true, if the thread was successfully created.
   bool CreateThread(char* name, UInt8 ui8Prio, UInt32 stackSize);

   // Suspends the thread
   // Suspends the thread belonging to this object instance.
   // The return value is true, if the thread was suspended.
   bool Suspend();

   // Resumes the thread
   // Resumes the thread belonging to this object instance.
   // The return value is true, if the thread could be resumed.
   bool Resume();

   // Terminates the thread. !!! not implemented yet !!!
   // Terminates the thread belonging to this object instance. This function
   // basically kills the thread. It should be used with care, since the
   // thread doesn't get chances to clean up.
   // ui32ExitCode specifies the exit code of the thread.
   // The function returns true
   bool Terminate(UInt32 ui32ExitCode);

   // Returns the Thread ID
   // This function returns the operating systems thread id.
   // Obtains the thread ID of the thread attached or -1 if no thread is attached.
   UInt32 GetThreadId() const;

   // Determines if the thread is running
   // A thread is running if it hasn't returned from the Run method yet.
   // Returns true if the thread is running.
   bool IsRunning() const;

   void Sleep(UInt32 dwTimeout);  // time in [ms]
   void Wait(UInt32 dwMycroSeconds); // time in [us]

   // HW ticks Rundlaufzähler.
   // Zählt die Ticks seit dem letzten Threadwechsel
   // Bei 120MHz sind es 120'000'000 HW ticks pro Sekunde.
   UInt32 GetTicksHW();

   // Umrechnung von HW ticks in Micro Sekunden
   UInt32 TicksHW2MicroSecond(UInt32 ticksHW);

   // ticks in [ms]
   UInt32 GetTickCount();
   
   Bool TimeExpired(UInt32 startTime, UInt32 timeout);

   // Sets the thread priority.
   // ui8Prio contains the new thread priority, which can be taken from the
   // ThreadPriorities enumeration or be an integer between 1 and 255.
   // The return value is true, if the priority was set.
   bool SetThreadPriority(UInt8 ui8Prio);

   // Returns the thread priority.
   // The return value is the current thread priority.
   UInt8 GetThreadPriority() const;

public:	/* Static members */
   // Returns the ID of the calling thread.
   // This function returns the id of the calling thread. This is not necessarily
   // a thread, which has been created using the CThread class.
   // return the current thread ID.
   static UInt32 GetCurrentThreadId();


private:


   // Thread Procedure
   // This OS specific function is called to start a thread. The implementation
   // of this function dispatches it to the appropriate CThread class instance
   // and performs everything needed to launch the thread.
   // Parameter:
   //   lpParameter is a pointer to the CThread instance.
   // The return value is the exit code of the thread.
   static void ThreadProc(UInt32 lpParameter);

protected:
   char m_TaskName[32 + 1];
   Bool      m_running;        

private:
   UInt32    m_TaskId;
   UInt32    m_lastCall;

   UInt32   m_ticksPerSecond;
   UInt32   m_HWticksPerTicks;
   UInt32   m_HWticksPerMicroSecond;

};


#include "NamespaceEnd.h"

