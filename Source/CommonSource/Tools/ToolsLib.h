/*-----------------------------------------------------------------------------
Autor                  Datum
E. Fitze               14.06.2013

-- BESCHREIBUNG ---------------------------------------------------------------


-- AENDERUNGEN ----------------------------------------------------------------

Autor                  Datum

-----------------------------------------------------------------------------*/
#pragma once


// Check for C++ compilation
#if !defined(__cplusplus)
	#error requires C++ compilation (use a .cpp suffix for your file)
#endif

//-----------------------------------------------------------------------------
#include "MQX.h"

#include "CommonTypes.h"
#include "Logger.h"
#include "OS.h"

//-----------------------------------------------------------------------------
#include "CThread.h"
#include "CCriticalSection.h"
#include "CEvent.h"
#include "CFileIO.h"

