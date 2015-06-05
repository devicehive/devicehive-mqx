/*-----------------------------------------------------------------------------
Autor                  Datum
E. Fitze               14.06.2013

-- BESCHREIBUNG ---------------------------------------------------------------


-- AENDERUNGEN ----------------------------------------------------------------

Autor                  Datum

-----------------------------------------------------------------------------*/
#pragma once

#include "MQX.h"
#include "Mutex.h"

#include "CommonTypes.h"

#include "Namespace.h"
//-----------------------------------------------------------------------------
class CCriticalSection
{
private:
   // Copy construction not supported
   CCriticalSection(const CCriticalSection& rhs);

   //Assignment not supported
   const CCriticalSection& operator=(const CCriticalSection& rhs);

public:
   // Construction / Destruction
   CCriticalSection();

   virtual ~CCriticalSection();

public:
   // Virtual Functions

public:
   // Thread Functions

   void Lock();
   void Unlock();


public:	/* Static members */


private:

protected:
   MUTEX_STRUCT   m_cs;

private:
   Bool           m_isLocked;

};


#include "NamespaceEnd.h"

