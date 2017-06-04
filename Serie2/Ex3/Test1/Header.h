#pragma once

#include <string>
#include "stdafx.h"
#include "windows.h"
#include <tchar.h>

#ifdef PROCESS_EXIF_EXPORTS  
#define PROCESS_EXIF_API __declspec(dllexport)
#else  
#define PROCESS_EXIF_API __declspec(dllimport)   
#endif  

// this struct maintains the state of an active mapping
// (or the error state if some error ocurrs in the map creation)
typedef  struct filemap {
	BYTE* baseAddress;		// base address of the map view 
	SIZE_T size;			// the size of the mapping (and view)
	HANDLE mapHandle;		// section object handle  
	DWORD  error;			// state (!= 0 means error)
} FILEMAP, *PFILEMAP;



/* Describes format descriptor */
static const int BytesPerFormat[] = { 0,1,1,2,4,8,1,1,2,4,8,4,8 };
#define NUM_FORMATS 12

#define FMT_BYTE       1 
#define FMT_STRING     2
#define FMT_USHORT     3
#define FMT_ULONG      4
#define FMT_URATIONAL  5
#define FMT_SBYTE      6
#define FMT_UNDEFINED  7
#define FMT_SSHORT     8
#define FMT_SLONG      9
#define FMT_SRATIONAL 10
#define FMT_SINGLE    11
#define FMT_DOUBLE    12

/* Describes tag values */


#define TAG_EXIF_OFFSET       0x8769
#define TAG_INTEROP_OFFSET    0xa005
#define TAG_DATETIME_ORIGINAL 0x9003
#define OFFSETBASE 6 

namespace exifo_pri_library
{
	//---------------------------------------------------------------------------
	// Destroy the mapping resources (mapped region and the associated section object)
	//------------------------------------------------------------------------------
	VOID unmapFile(FILEMAP fm);

	// This class is exported from the exifo_pri_library.dll  
	class Test
	{
	public:

		typedef BOOL(*PROCESS_EXIF_TAG)(LPCVOID ctx, DWORD tagNumber, LPCVOID value);
		static  PROCESS_EXIF_API VOID JPEG_ProcessExifTags(PTCHAR fileImage, PROCESS_EXIF_TAG processor, LPCVOID ctx);
	};

	static BOOL mapFile(LPCSTR fileName, int access, int mode, LPCSTR name, SIZE_T size, PFILEMAP fm, BOOL isUnicode);


	static int Get16m(void * Short);
	static int Get16u(void * Short, bool MotorolaOrder);
	static long Get32s(void * Long, bool MotorolaOrder);
	static unsigned long Get32u(void * Long, bool motorola);
	static VOID ProcessExifDir(unsigned char * DirStart, unsigned char * OffsetBase, unsigned ExifLength,
		bool motorola, Test::PROCESS_EXIF_TAG processor, LPCVOID ctx);
	static double ConvertAnyFormat(void * ValuePtr, int Format, bool motorola);
}

