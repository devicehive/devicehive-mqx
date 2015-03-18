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

//-----------------------------------------------------------------------------
class OS
//-----------------------------------------------------------------------------
{
public:
   // Allocate Memory from fast SRAM
   // return: Pointer to SRAM meory.
   static UInt8* AllocateSRAM(UInt32 blockSize);
   static void DeallocateSRAM(UInt8* memoryPointer);

   // set to True: Allocated Momory will be cleard
   // default: FALSE
   static void ClearOnAllocate(Bool clear);

   // Kopiert einen NULL Terminierten String. Der Zielstring wird NULL terminiert.
   static void CopyString(char* from, char* to);

   static void CopyBytes(PVOID from, PVOID to, UInt32 count);
   static void ZeroBytes(PVOID memAddress, UInt32 count);

   static UInt32 GetTickCount();

   //-----------------------------------------------------------------------------
   // Wandelt die IP adresse von einem string in eine UInt32.
   // ipAdress: "192.168.10.1"
   // return 0xC0A80A01
   // return 0, bei einem Fehler.
   //-----------------------------------------------------------------------------
   static UInt32 ConvertToIP(char * ipAddress);
   static Bool   ConvertFromIP(UInt32 ipAdr, char * ipText);
   //-----------------------------------------------------------------------------

private: // static class do not support constructor...
   /** Constructor */
   OS();

   /** Destructor */
   virtual ~OS();

private:
   static Bool m_clearOnAllocate;

public:

};


//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------

#include "NamespaceEnd.h"

