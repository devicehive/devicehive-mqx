/*-----------------------------------------------------------------------------
Autor                  Datum
E. Fitze               14.06.2013

-- BESCHREIBUNG ---------------------------------------------------------------


-- AENDERUNGEN ----------------------------------------------------------------

Autor                  Datum

-----------------------------------------------------------------------------*/
#pragma once

#include "MQX.h"
#include "lwEvent.h"

#include "CommonTypes.h"

#include "Namespace.h"

//-----------------------------------------------------------------------------
class CEvent
{
private:
   // Copy construction not supported
   CEvent(const CEvent& rhs);

   //Assignment not supported
   const CEvent& operator=(const CEvent& rhs);

public:
   // Construction / Destruction
   CEvent();

   virtual ~CEvent();

public:
   // Virtual Functions

public:
   // Thread Functions

   // Create the event.
   Bool Create(Bool bManualReset = false);


   // Method to set the event state to signaled.
   // This method sets an event object to signaled state.
   // Auto-Reset objects are automatically reset after releasing one thread.
   // Manual-Reset objects must be explicitly reset by a call to Reset().
   // The return value is true if successful.
   Bool Set();

   // Method to reset the event state
   // This method resets an event object to non-signaled state.
   // The return value is true if successful.
   Bool Reset();

   // waits for the signal for a maximum time  in [ms].
   // return TRUE: event was signalled
   // return FALSE: Timeout
   Bool Wait(UInt32 timeout);


public:	/* Static members */


private:
   LWEVENT_STRUCT    m_event;

   UInt32            m_ticksPerSec;

};


#include "NamespaceEnd.h"

