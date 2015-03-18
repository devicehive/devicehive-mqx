/*-----------------------------------------------------------------------------

Autor                                                           Datum
Nikolay Peganov <Nikolay.Peganov@dataart.com>                   18.12.2014

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

class RetryableObject : private NonCopyable
{  
public:
   // Construction 
   RetryableObject();
   explicit RetryableObject(void* obj, bool keepsArray = false);
   explicit RetryableObject(cJSON* obj, bool keepsArray = false);   
        
   // Destruction
   virtual ~RetryableObject();
        
public:   
   void* GetObject() const;
   
   UInt8 GetRetryCount();
   void SetRetryCount(UInt8 newValue);
   UInt8 Get419RetryCount();
   void Set419RetryCount(UInt8 newValue);   
   
private:
   UInt8 m_RetryCount;
   UInt8 m_419RetryCount;   
   void* m_Obj;
   bool m_KeepsJson;
   bool m_KeepsArray;   
};

#include "NamespaceEnd.h"