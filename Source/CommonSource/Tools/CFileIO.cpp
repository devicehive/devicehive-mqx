//-----------------------------------------------------------------------------

#include "CFileIO.h"

#include "MQX.h"
#include <bsp.h>
#include <mfs.h>

using namespace Tools;

////////////////////////////////////////////////////////////////////////
// Static data members
////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////
// Construction / Destruction
////////////////////////////////////////////////////////////////////////
CFileIO::CFileIO()
: m_hFile(0)
{
}

CFileIO::~CFileIO()
{
   if (m_hFile)
   {
      Close();
   }
}

////////////////////////////////////////////////////////////////////////
//	Implementation
////////////////////////////////////////////////////////////////////////
//-----------------------------------------------------------------------------
Bool CFileIO::Open(const char * fileName, FileMode mode)
//-----------------------------------------------------------------------------
{
   if (!IsValid())
	{
      if (mode == fmRead)
      {
         m_hFile = fopen(fileName, "r");
      }
      else if (mode == fmWrite)
      {
         m_hFile = fopen(fileName, "w+");
      }
      else
      {
         m_hFile = fopen(fileName, "a+");
      }
		if (m_hFile)
		{
         return TRUE;
		}
	}
	return 0;
}

//-----------------------------------------------------------------------------
Bool CFileIO::Close()
//-----------------------------------------------------------------------------
{
	if (IsValid())
	{
		fclose(m_hFile);
		m_hFile = 0;
	}
	return true;
}

//-----------------------------------------------------------------------------
UInt32 CFileIO::GetFileSize() const
//-----------------------------------------------------------------------------
{
	if (m_hFile)
	{
      return m_hFile->SIZE;
	}
	return 0;
}

//-----------------------------------------------------------------------------
UInt32 CFileIO::Read(void* pBuffer, UInt32 ui32Size)
//-----------------------------------------------------------------------------
{
	UInt32 ui32SizeRead;
   ui32SizeRead = read(m_hFile, pBuffer, ui32Size);
   return ui32SizeRead;
}

//-----------------------------------------------------------------------------
UInt32 CFileIO::Write(void* pBuffer, UInt32 ui32Size)
//-----------------------------------------------------------------------------
{
   UInt32 ui32Written;
   ui32Written = write(m_hFile, pBuffer, ui32Size);
	return ui32Written;
}

//-----------------------------------------------------------------------------
Bool CFileIO::Seek(Int32 i32Position, FileSeekStart fssMode)
//-----------------------------------------------------------------------------
{
	_mqx_int bResult = false;
   bResult = fseek(m_hFile, i32Position, fssMode);
	return bResult;
}

//-----------------------------------------------------------------------------
UInt32 CFileIO::GetPosition() const
//-----------------------------------------------------------------------------
{
	return m_hFile->LOCATION;
}

//-----------------------------------------------------------------------------
Bool CFileIO::IsValid() const
//-----------------------------------------------------------------------------
{
	return m_hFile != 0;
}

//-----------------------------------------------------------------------------
void CFileIO::Flush()
//-----------------------------------------------------------------------------
{
	if (IsValid())
	{
		fflush(m_hFile);
	}
}



//-----------------------------------------------------------------------------
Bool CFileIO::ReadLine(UInt8 * pBuffer, UInt32 bufferLength, UInt32 & bytesWritten)
//-----------------------------------------------------------------------------
{
   UInt8 ch;
   bool  ok = FALSE;
   bytesWritten = 0;

   if (bufferLength < 2)
      return FALSE;

   while (Read(&ch, 1) == 1)
   {
      ok = TRUE;
      if (ch == '\n')
         continue;
      if (ch == '\r')
         break;

      if (bytesWritten < (bufferLength - 1) )
      {
         pBuffer[bytesWritten++] = ch;
      }

   }
   pBuffer[bytesWritten] = 0;

   return ok;
}


