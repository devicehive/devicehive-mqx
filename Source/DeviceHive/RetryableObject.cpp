/*-----------------------------------------------------------------------------

Autor                                                           Datum
Nikolay Peganov <Nikolay.Peganov@dataart.com>                   19.12.2014

-- BESCHREIBUNG ---------------------------------------------------------------


-- AENDERUNGEN ----------------------------------------------------------------

Autor                  Datum

-----------------------------------------------------------------------------*/

#include "RetryableObject.h"

using namespace DH;

//-----------------------------------------------------------------------------
RetryableObject::RetryableObject()
//-----------------------------------------------------------------------------
: m_RetryCount(0)
, m_419RetryCount(0)
, m_Obj(0)
, m_KeepsJson(false) 
, m_KeepsArray(false)
{
   LOG2(Tools::Logger::lmTrace, "RetryableObject{0x%X} created with dummy Obj{0x%X}",
        this, m_Obj);           
} 

//-----------------------------------------------------------------------------
RetryableObject::RetryableObject(void* obj, bool keepsArray)
//-----------------------------------------------------------------------------
: m_RetryCount(0)
, m_419RetryCount(0)
, m_Obj(obj)
, m_KeepsJson(false) 
, m_KeepsArray(keepsArray)
{
   LOG2(Tools::Logger::lmTrace, "RetryableObject{0x%X} created with Obj{0x%X}",
        this, obj);           
} 

//-----------------------------------------------------------------------------
RetryableObject::RetryableObject(cJSON* obj, bool keepsArray)
//-----------------------------------------------------------------------------
: m_RetryCount(0)
, m_419RetryCount(0)
, m_Obj((void*)obj)
, m_KeepsJson(true) 
, m_KeepsArray(keepsArray)
{
   LOG2(Tools::Logger::lmTrace, "RetryableObject{0x%X} created with cJSON{0x%X}",
        this, obj);           
} 

//-----------------------------------------------------------------------------
RetryableObject::~RetryableObject()
//-----------------------------------------------------------------------------
{
   if (m_KeepsJson)
   {
      cJSON_Delete((cJSON*)m_Obj);
      
      LOG2(Tools::Logger::lmTrace, "RetryableObject{0x%X} has released cJSON{0x%X}",
           this, m_Obj);          
      
      m_Obj = 0;
   }
   else
   {
      if (m_Obj)
      {
         if (!m_KeepsArray)
            delete m_Obj;
         else
            delete [] m_Obj;
         
         LOG2(Tools::Logger::lmTrace, "RetryableObject{0x%X} has released Obj{0x%X}",
              this, m_Obj);           
         
         m_Obj = 0;      
      }      
   }
   
   LOG1(Tools::Logger::lmTrace, "RetryableObject{0x%X} destroyed", this);        
}


//-----------------------------------------------------------------------------
void* RetryableObject::GetObject() const
//-----------------------------------------------------------------------------
{
   return m_Obj;
}

//-----------------------------------------------------------------------------
UInt8 RetryableObject::GetRetryCount()
//-----------------------------------------------------------------------------
{
   return m_RetryCount;
}
  
//-----------------------------------------------------------------------------
void RetryableObject::SetRetryCount(UInt8 newValue)
//-----------------------------------------------------------------------------
{
   m_RetryCount = newValue;
}

//-----------------------------------------------------------------------------
UInt8 RetryableObject::Get419RetryCount()
//-----------------------------------------------------------------------------
{
   return m_419RetryCount;
}
  
//-----------------------------------------------------------------------------
void RetryableObject::Set419RetryCount(UInt8 newValue)
//-----------------------------------------------------------------------------
{
   m_RetryCount = m_419RetryCount;
}
  