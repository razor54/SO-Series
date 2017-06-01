// Test1.cpp : Defines the exported functions for the DLL application.
//

#include "stdafx.h"
#include <iostream>
#include "ExifProcess.h"
#include <tchar.h>
#include "windows.h"
#include "Winsock.h"


namespace exifo_pri_library
{

	// auxiliary method to convert the file access mode to file mapping access
	static DWORD fileToMapAccess(int fa) {
		int ma = ((fa & GENERIC_READ) != 0) ? PAGE_READONLY : 0;
		if ((fa & GENERIC_WRITE) != 0) ma = PAGE_READWRITE;
		return ma;
	}

	// auxiliary method to convert the file access mode to view  access
	static DWORD mapToViewAccess(int fa) {
		int va = ((fa & GENERIC_READ) != 0) ? FILE_MAP_READ : 0;
		if ((fa & GENERIC_WRITE) != 0) va |= FILE_MAP_WRITE;
		return va;
	}

	//--------------------------------------------------------------------------------
	// This function create a memory mapping of the file with the given name. 
	// The other parameters include the access type (read/readwrite), mode(create or open), 
	// mapping size and an optional name of the section object
	// Parameters detail:
	// "fileName": the name of the file to be memory mapped. 
	//           If NULL a shared memory region is created instead
	// "access": the pretended access (GENERIC_READ, GENERIC_WRITE or both)
	// "mode":   the creation mode (OPEN_EXISTING, CREATE_ALLWAYS)
	// "name":   the name of the created/used section object. If NULL an anonymous
	//           section object will be created
	// "size":   the size of the mapped region
	// "fm":     the struct to fill with mapping or error state
	// Return:
	//   the function returns TRUE if the mapping creation succeeded, FALSE if an error ocurrs.
	//   In that case the error state is filled
	//---------------------------------------------------------------------
	BOOL mapFile(LPCSTR fileName, int access, int mode, LPCSTR name, SIZE_T size, PFILEMAP fm, BOOL isUnicode) {

		HANDLE fh = INVALID_HANDLE_VALUE;
		HANDLE mh = nullptr;
		LPVOID mapAddress = nullptr;


		if (fileName != nullptr) {
			if (!isUnicode)
				fh = CreateFileA(fileName, access, 0, nullptr, mode, FILE_ATTRIBUTE_NORMAL, nullptr);
			else
				fh = CreateFileW(LPCWSTR(fileName), access, 0, nullptr, mode, FILE_ATTRIBUTE_NORMAL, nullptr);
			if (fh == INVALID_HANDLE_VALUE) goto error;
			if (size == 0) {
				LARGE_INTEGER fileSize;
				if (!GetFileSizeEx(fh, &fileSize)) goto error;
				size = static_cast<SIZE_T>(fileSize.QuadPart);
			}
		}
		LARGE_INTEGER aux;
		aux.QuadPart = size;
		if (!isUnicode)
			mh = CreateFileMappingA(fh, nullptr, fileToMapAccess(access), aux.HighPart, aux.LowPart, name);
		else mh = CreateFileMappingW(fh, nullptr, fileToMapAccess(access), aux.HighPart, aux.LowPart, reinterpret_cast<LPCWSTR>(name));
		if (mh == nullptr) goto error;


		mapAddress = MapViewOfFile(mh, mapToViewAccess(access), 0, 0, 0);
		if (mapAddress == nullptr) goto error;
		fm->baseAddress = static_cast<BYTE*>(mapAddress);
		fm->mapHandle = mh;
		fm->error = 0;
		fm->size = size;
		// After creation of section object, the file handle can be closed
		CloseHandle(fh);
		return TRUE;

	error:
		fm->error = GetLastError();
		if (mapAddress != nullptr) UnmapViewOfFile(mapAddress);
		if (mh != nullptr) CloseHandle(mh);
		if (fh != INVALID_HANDLE_VALUE) CloseHandle(fh);
		return FALSE;
	}

	//---------------------------------------------------------------------------
	// Destroy the mapping resources (mapped region and the associated section object)
	//------------------------------------------------------------------------------
	VOID unmapFile(FILEMAP fm) {
		UnmapViewOfFile(fm.baseAddress);
		CloseHandle(fm.mapHandle);
	}


	BOOL getImageInfo(TCHAR* image,char* ini, char* end)
	{
		int res;
		FILEMAP mapDesc;

		bool isUnicode;
		if (sizeof(*image) == sizeof(char))isUnicode = false;
		else if (sizeof(*image) == sizeof(wchar_t))isUnicode = true;

		if (!(res = mapFile(reinterpret_cast<LPSTR>(image), GENERIC_READ, OPEN_EXISTING, nullptr, 0, &mapDesc, isUnicode)))
			return FALSE;
		
		auto len = mapDesc.size;
		auto buf = mapDesc.baseAddress;


		// Normally, we should able to find the JPEG end marker 0xFFD9 at the end
		while (len > 2) {
			if (buf[len - 1] == 0 || buf[len - 1] == 0xFF) 
				len--;
			
			else {
				if (buf[len - 1] != 0xD9 || buf[len - 2] != 0xFF) 
					return FALSE;
				break;
			}
		}

		// Scan for EXIF header (bytes 0xFF 0xE1) and 
		// look for bytes "Exif\0\0".
		// The marker has to contain at least the TIFF header, otherwise the
		// EXIF data is corrupt. So the minimum length specified here has to be:
		//   2 bytes: section size
		//   6 bytes: "Exif\0\0" string
		//   2 bytes: TIFF header (either "II" or "MM" string)
		//   2 bytes: TIFF magic (short 0x2a00 in Motorola byte order)
		//   4 bytes: Offset to first IFD
		// =========
		//  16 bytes
		unsigned offs = 0;  // current offset into buffer
		for (offs = 0; offs < len - 1; offs++)
			if (buf[offs] == 0xFF && buf[offs + 1] == 0xE1) break;
		if (offs + 4 > len)
			return FALSE;
		offs += 2;
		auto section_length = Get16m(buf + offs);

		if (offs + section_length > len || section_length < 16)
			return FALSE;
		offs += 2;
		//get curr length and address
		buf = buf + offs;
		len = len - offs;

		offs = 0;       // current offset into buffer
		if (!buf || len < 6)
			return FALSE;

		//'buf' start of the EXIF TIFF, which must be the bytes "Exif\0\0".
		if (!std::equal(buf, buf + 6, "Exif\0\0"))
			return FALSE;
		offs += 6;

		// Now parsing the TIFF header. The first two bytes are either "II" or
		// "MM" for Intel or Motorola byte alignment. Sanity check by parsing
		// the unsigned short that follows, making sure it equals 0x2a. The
		// last 4 bytes are an offset into the first IFD, which are added to
		// the global offset counter. For this block, we expect the following
		// minimum size:
		//  2 bytes: 'II' or 'MM'
		//  2 bytes: 0x002a
		//  4 bytes: offset to first IDF
		// -----------------------------
		//  8 bytes

		if (offs + 8 > len)
			return FALSE;
		bool motorola_code;

		if (memcmp(buf + offs, "II", 2) == 0)
			motorola_code = false;
		else
		{
			if (memcmp(buf + offs, "MM", 2) == 0)
				motorola_code = true;
			else
				return FALSE;
		}

		offs += 2;

		if (Get16u(buf + offs, motorola_code) != 0x2a)
			return FALSE;

		offs += 2;

		int FirstOffset = Get32u(buf + offs, motorola_code);
		if (FirstOffset < 8 || FirstOffset > 16) 
			return FALSE;
		
		
		/* First directory starts 16 unsigned chars in.  Offsets start at 8 unsigned chars in. */
		if (!ProcessExifDir(buf + 14, buf + OFFSETBASE, len - OFFSETBASE, motorola_code,ini,end))
			return FALSE;



		unmapFile(mapDesc);
	}




	//--------------------------------------------------------------------------
	// Get 16 bits motorola order (always) for jpeg header stuff.
	//--------------------------------------------------------------------------
	int Get16m(void * Short)
	{
		return (static_cast<unsigned char *>(Short)[0] << 8) | static_cast<unsigned char *>(Short)[1];
	}
	////////////////////////////////////////////////////////////////////////////////
	/*--------------------------------------------------------------------------
	Convert a 16 bit unsigned value from file's native unsigned char order
	--------------------------------------------------------------------------*/
	int Get16u(void * Short, bool MotorolaOrder)
	{
		if (MotorolaOrder) {
			return (static_cast<unsigned char *>(Short)[0] << 8) | static_cast<unsigned char *>(Short)[1];
		}
		return (static_cast<unsigned char *>(Short)[1] << 8) | static_cast<unsigned char *>(Short)[0];

	}
	////////////////////////////////////////////////////////////////////////////////
	/*--------------------------------------------------------------------------
	Convert a 32 bit signed value from file's native unsigned char order
	--------------------------------------------------------------------------*/
	long Get32s(void * Long, bool MotorolaOrder)
	{
		if (MotorolaOrder) {
			return  (static_cast<char *>(Long)[0] << 24) | (static_cast<unsigned char *>(Long)[1] << 16)
				| (static_cast<unsigned char *>(Long)[2] << 8) | (static_cast<unsigned char *>(Long)[3] << 0);
		}
		return  (static_cast<char *>(Long)[3] << 24) | (static_cast<unsigned char *>(Long)[2] << 16)
			| (static_cast<unsigned char *>(Long)[1] << 8) | (static_cast<unsigned char *>(Long)[0] << 0);
	}
	////////////////////////////////////////////////////////////////////////////////
	/*--------------------------------------------------------------------------
	Convert a 32 bit unsigned value from file's native unsigned char order
	--------------------------------------------------------------------------*/
	unsigned long Get32u(void * Long, bool motorola)
	{
		return static_cast<unsigned long>(Get32s(Long, motorola)) & 0xffffffff;
	}




	bool ProcessExifDir(unsigned char * DirStart, unsigned char * OffsetBase, unsigned ExifLength,
		 bool motorola, char* ini, char* end)
	{
		int a;

		int NumDirEntries = Get16u(DirStart, motorola);

		if ((DirStart + 2 + NumDirEntries * 12) > (OffsetBase + ExifLength)) 
			return false;
		

		for (int de = 0; de < NumDirEntries; de++) {
			unsigned char * ValuePtr;
			unsigned char * DirEntry = DirStart + 2 + 12 * de;

			int Tag = Get16u(DirEntry, motorola);
			int Format = Get16u(DirEntry + 2, motorola);
			int Components = Get32u(DirEntry + 4, motorola);
			unsigned data = Get32u(DirEntry + 8, motorola);

			if ((Format - 1) >= NUM_FORMATS) 
				return false;
			

			int BytesCount = Components * BytesPerFormat[Format];

			if (BytesCount > 4) {
				unsigned OffsetVal = Get32u(DirEntry + 8, motorola);
				/* If its bigger than 4 unsigned chars, the dir entry contains an offset.*/
				if (OffsetVal + BytesCount > ExifLength) 
					return false;
				
				ValuePtr = OffsetBase + OffsetVal;
			}
			else {
				/* 4 unsigned chars or less and value is in the dir entry itself */
				ValuePtr = DirEntry + 8;
			}

			if (Tag == TAG_DATETIME_ORIGINAL) {

				//TODO the comparing
				char * date = reinterpret_cast<char*>(ValuePtr);
				
				if (strcmp(date, ini) < 0)
					return false;

				if (strcmp(date, end) > 0)
					return false;

				return true;

			}

			if (Tag == TAG_EXIF_OFFSET || Tag == TAG_INTEROP_OFFSET) {

				unsigned char * SubdirStart = OffsetBase + Get32u(ValuePtr, motorola);
				if (SubdirStart < OffsetBase ||
					SubdirStart > OffsetBase + ExifLength) {
					return false;
				}
				return ProcessExifDir(SubdirStart, OffsetBase, ExifLength, motorola,ini,end);
			}
		}

		return false;
	}



	double ConvertAnyFormat(void * ValuePtr, int Format, bool motorola)
	{
		double Value = 0;

		switch (Format) {
		case FMT_SBYTE:     Value = *static_cast<signed char *>(ValuePtr);  break;
		case FMT_BYTE:      Value = *static_cast<unsigned char *>(ValuePtr);        break;

		case FMT_USHORT:    Value = Get16u(ValuePtr, motorola);          break;
		case FMT_ULONG:     Value = Get32u(ValuePtr, motorola);          break;

		case FMT_URATIONAL:
		case FMT_SRATIONAL:
		{
			int Num = Get32s(ValuePtr, motorola);
			int Den = Get32s(4 + static_cast<char *>(ValuePtr), motorola);
			if (Den == 0) {
				Value = 0;
			}
			else {
				Value = static_cast<double>(Num) / Den;
			}
			break;
		}

		case FMT_SSHORT:    Value = static_cast<signed short>(Get16u(ValuePtr, motorola));  break;
		case FMT_SLONG:     Value = Get32s(ValuePtr, motorola);                break;

		}
		return Value;
	}

	
	// Test program:
	//--------------------------------------------------------------
	void Test::PrintExifTags(TCHAR* filename, CHAR*ini, CHAR*end)
	{
		std::wstring s = filename;

		if (getImageInfo(filename, ini, end))
			printf("%ls\n", filename);

	}


}
