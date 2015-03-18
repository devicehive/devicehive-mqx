/*-----------------------------------------------------------------------------

Autor                                                           Datum
Nikolay Peganov <Nikolay.Peganov@dataart.com>                   25.12.2014

-- BESCHREIBUNG ---------------------------------------------------------------


-- AENDERUNGEN ----------------------------------------------------------------

Autor                  Datum

-----------------------------------------------------------------------------*/
#include "string.h"

#include "ErrorDH.h"
#include "HttpClient.h"

using namespace DH;

// Construction
//-----------------------------------------------------------------------------
ErrorDH::ErrorDH(UInt32 code, ErrorType type, ErrorPlace place, const char* what, UInt32 lineNumber, char* sourceFilename)
//-----------------------------------------------------------------------------
: m_Code(code)
, m_Type(type)
, m_Place(place)
, m_pWhat(0)
, m_pFile(0)
, line(lineNumber)
{
   size_t whatLen = strlen(what);
   if (whatLen > 0)
   {
      m_pWhat = new char[whatLen + 1];
      strcpy(m_pWhat, what);
   }
   
   char* file = strrchr(sourceFilename, '\\');
   if (file)
   {
      ++file;
      size_t fileLen = strlen(file);  
      m_pFile = new char[fileLen + 1];
      strcpy(m_pFile, file);      
   } 
   
   LOG3(Tools::Logger::lmInfo, "ErrorDH{0x%X} created in %s(%d)",
        this, m_pFile, lineNumber);     
}

//-----------------------------------------------------------------------------
ErrorDH::~ErrorDH()
//-----------------------------------------------------------------------------
{
   if (m_pWhat)
   {
      delete [] m_pWhat;
      m_pWhat = 0;      
   }
   
   if (m_pFile)
   {
      delete [] m_pFile;
      m_pFile = 0;      
   }   
   
   LOG1(Tools::Logger::lmInfo, "ErrorDH{0x%X} destroyed", this);       
}


//-----------------------------------------------------------------------------
bool ErrorDH::Interrupted()
//-----------------------------------------------------------------------------
{
   return (m_Type == ET_AppSpecific) && (m_Code == EC_Interrupted);
}

//-----------------------------------------------------------------------------
const char* ErrorDH::What()
//-----------------------------------------------------------------------------
{
   return m_pWhat;
}