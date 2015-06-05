//-----------------------------------------------------------------------------
#pragma once

#include "CommonTypes.h"
#include <mfs.h>

#include "Namespace.h"

//-----------------------------------------------------------------------------

/* Specifies the file open mode
 *
 * This enumeration specifies how to open a file
 * using the CFile::Open method.
 *
 */
typedef enum
{
	//! No file mode
	fmInvalidMode	= 0x00,
	//! Open file for reading
	fmRead			= 0x01,
	//! Open file for writing
	fmWrite			= 0x02,
	//! Open for appending
	fmAppend		= 0x06
} FileMode;

/* Specifies how to seek for the CFile::Seek method
 *
 * This method specifies the way to seek in the CFile::Seek method.
 *
 */
typedef enum
{
	// Seek from file start
	fssFileStart = 1,
	// Seek from the current position
	fssCurrent = 2,
	// Seek from file end
	fssFileEnd = 3
} FileSeekStart;

class CFileIO
{
public:
   //-----------------------------------------------------------------------------
   // default constructor
   //-----------------------------------------------------------------------------
	CFileIO();

   //-----------------------------------------------------------------------------
   // destructor
   //-----------------------------------------------------------------------------
	~CFileIO();

	/* This method opens the specified file in the specified mode.
	 *
	 * lpszFile specifies a relative file name.
	 * mode specifies the mode in which the file must be opened.
	 *
	 * The function returns true if successful or false if it
	 *			failed.
	 *
	 * The instance must not already have a file open.
	 *
	 */
	Bool	Open(const char * fileName, const FileMode mode);

	/* Closes an opened file.
	 *
	 * Calling this method on a CFile object causes it to close the file
	 * it has opened. Closing the file causes the file buffers to be flushed
	 * and the file to be unlocked.
	 *
	 * The return value is true if the method succeeds.
	 */
	Bool	Close();

	/* Reads the specified number of bytes from the file.
	 *
	 * You must allocate a memory buffer to read the file data into. This memory
	 * buffer is given as the first parameter
	 *
	 * pBuffer is the pointer, where to read the data into.
	 * ui32Size specifies the number of bytes to read.
	 *
	 * The return value is the number of bytes read or -1
	 *		if the function fails.
	 *
	 * pBuffer must point to a valid (non-null) memory location and
	 *		the CFile instance must have an open file for reading.
	 */
	UInt32	Read(void* pBuffer, const UInt32 ui32Size);

	/* Writes the specified number of bytes from the file.
	 *
	 * This method copies the given memory block into the file buffers of the
	 * operating system for storage. You can safely modify the given buffer
	 * after this call returns.
	 *
	 * pBuffer is the pointer, which is to write.
	 * ui32Size specifies the number of bytes to write.
	 *
	 * The return value is the number of bytes written or -1
	 *		if the function fails.
	 *
	 * pBuffer must point to a valid (non-null) memory location and
	 *		the CFile instance must have an open file for writing (fmWrite or fmAppend)
	 *
	 */
	UInt32	Write(void* pBuffer, const UInt32 ui32Size);

	/* Returns the size of the file.
	 *
	 * The returned file size is determined from the operating system.
	 *
	 * The return value is the size of the file.
	 *
	 * The CFile instance must have an open file.
	 *
	 */
	UInt32	GetFileSize() const;

	/* Sets the file pointer to a certain byte within the file
	 *
	 * This function sets the file pointer to the specified byte.
	 *
	 * i32Position specifies the position to move to / by
	 * fssMode specifies the seek mode
	 *
	 * The return value is true if successful.
	 */
	Bool Seek(Int32 i32Position, FileSeekStart fssMode = ::Tools::fssCurrent);

	/* Returns the current byte position in the file
	 *
	 * This method returns the current position in the file.
	 *
	 * return Current file pointer position.
	 *
	 */
	UInt32 GetPosition() const;

	/* Flushes the buffers associated with this class
	 *
	 * If the underlying operating system buffers file I/O, then this function
	 * can be used to force commiting these buffers.
	 *
	 */
	void Flush();

	/* Returns true if a file is opened
	 *
	 * This method checks if the instance manages a file and returns
	 * true if it has an openend file.
	 *
	 * The return value is true if the CFile instance has an
	 * open file, or false if it hasn't.
	 *
	 */
	Bool IsValid() const;


   // liest eine Zeile, Kommentare werden auch zurückgegeben
   Bool ReadLine(UInt8 * pBuffer, UInt32 bufferLength, UInt32 & bytesWritten);


private:
   //-----------------------------------------------------------------------------
   // Assignment not supported
   //-----------------------------------------------------------------------------
	const CFileIO& operator= (const CFileIO& rhs);

   //-----------------------------------------------------------------------------
   // Copy construction not supported
   //-----------------------------------------------------------------------------
	CFileIO(const CFileIO& rhs);

	// Handle to the file
	MQX_FILE_PTR		m_hFile;


};


#include "NamespaceEnd.h"

