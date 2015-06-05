/*-----------------------------------------------------------------------------
Autor                  Datum
E. Fitze               14.06.2013

-- BESCHREIBUNG ---------------------------------------------------------------


-- AENDERUNGEN ----------------------------------------------------------------

Autor                  Datum

-----------------------------------------------------------------------------*/
#include "CEvent.h"

using namespace Tools;
//-----------------------------------------------------------------------------




// Construction / Destruction
//-----------------------------------------------------------------------------
CEvent::CEvent()
//-----------------------------------------------------------------------------
{
   // VORSICHT, im konstructor keine funktionen rufen. wenn Klass statisch,
   // wird der konstruktor gerufen befor OS initialisiert ist.
//   m_ticksPerSec = _time_get_ticks_per_sec();
}

//-----------------------------------------------------------------------------
CEvent::~CEvent()
//-----------------------------------------------------------------------------
{
   _lwevent_destroy(&m_event);
}


//-----------------------------------------------------------------------------
Bool CEvent::Create(Bool bManualReset)
//-----------------------------------------------------------------------------
{
   UInt32 mqxRet;

   m_ticksPerSec = _time_get_ticks_per_sec();

   if (bManualReset)
   {
      mqxRet = _lwevent_create(&m_event, 0);
   }
   else
   {
      mqxRet = _lwevent_create(&m_event, LWEVENT_AUTO_CLEAR);
   }
   return mqxRet == MQX_OK;
}

//-----------------------------------------------------------------------------
Bool CEvent::Set()
//-----------------------------------------------------------------------------
{
   UInt32 mqxRet;
   mqxRet = _lwevent_set(&m_event, 0x01);
   return mqxRet == MQX_OK;
}


//-----------------------------------------------------------------------------
Bool CEvent::Reset()
//-----------------------------------------------------------------------------
{
   UInt32 mqxRet;
   mqxRet = _lwevent_clear(&m_event, 0x01);
   return mqxRet == MQX_OK;
}

//-----------------------------------------------------------------------------
Bool CEvent::Wait(UInt32 timeout)
//-----------------------------------------------------------------------------
{
   UInt32 mqxRet;
   mqxRet = _lwevent_wait_ticks(&m_event, 0x01, FALSE, timeout * m_ticksPerSec / 1000);
   if (mqxRet == LWEVENT_WAIT_TIMEOUT)
   {
      return FALSE;
   }
   return mqxRet == MQX_OK;
}




