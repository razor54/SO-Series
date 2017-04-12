// mapfile.cpp : Defines the entry point for the console application.
//
// Exemplo de utilização da API de mapeamento de ficheiros em memória
//
// 

#include "stdafx.h"
#include <iostream>


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
BOOL mapFile(LPCTSTR fileName, int access, int mode, LPCTSTR name, SIZE_T size, PFILEMAP fm) {
	HANDLE fh = INVALID_HANDLE_VALUE;
	HANDLE mh = nullptr;
	LPVOID mapAddress = nullptr;

	if (fileName != nullptr) {
		fh = CreateFile(fileName, access, 0, nullptr, mode, FILE_ATTRIBUTE_NORMAL, nullptr);
		if (fh == INVALID_HANDLE_VALUE) goto error;
		if (size == 0) {
			LARGE_INTEGER fileSize;
			if (!GetFileSizeEx(fh, &fileSize)) goto error;
			size = static_cast<SIZE_T>(fileSize.QuadPart);
		}
	}
	LARGE_INTEGER aux;
	aux.QuadPart = size;
	mh = CreateFileMapping(fh, nullptr, fileToMapAccess(access), aux.HighPart, aux.LowPart, name);
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
VOID unmapFile(PFILEMAP fm) {
	UnmapViewOfFile(fm->baseAddress);
	CloseHandle(fm->mapHandle);
}


void clear(image_data* data)
{
	data->Make = "";
	data->Model = "";

	data->DateTime = "";


	data->ExposureTime = 0;
	data->FNumber = 0;
	data->ExposureProgram = 0;
	data->ISOSpeedRatings = 0;
	data->ShutterSpeedValue = 0;
	data->ExposureBiasValue = 0;

	data->ImageWidth = 0;
	data->ImageHeight = 0;

	// Geolocation
	data->GeoLocation.Latitude = 0;
	data->GeoLocation.Longitude = 0;
	data->GeoLocation.Altitude = 0;

	data->GeoLocation.LatComponents.degrees = 0;
	data->GeoLocation.LatComponents.minutes = 0;
	data->GeoLocation.LatComponents.seconds = 0;
	data->GeoLocation.LatComponents.direction = '?';
	data->GeoLocation.LonComponents.degrees = 0;
	data->GeoLocation.LonComponents.minutes = 0;
	data->GeoLocation.LonComponents.seconds = 0;
	data->GeoLocation.LonComponents.direction = '?';

}

int getImageInfo(LPCTSTR image)
{
	int res;
	FILEMAP mapDesc;

	if (!(res = mapFile(image, GENERIC_READ, OPEN_EXISTING, nullptr, 0, &mapDesc)))
	{
		_tprintf(_T("Error %d mapping file\n"), res);
		return -1;
	}
		
	
	image_data * result = nullptr;
	auto len = mapDesc.size;
	auto buf = mapDesc.baseAddress;
		

	// Normally, we should able to find the JPEG end marker 0xFFD9 at the end
	while (len > 2) {
		if (buf[len - 1] == 0 || buf[len - 1] == 0xFF) {
			len--;
		}
		else {
			if (buf[len - 1] != 0xD9 || buf[len - 2] != 0xFF) {
				return -1;
			}
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
	if (offs + 4 > len) return -1;
	offs += 2;
	auto section_length = Get16m(buf + offs);

	if (offs + section_length > len || section_length < 16)
		return -1;
	offs += 2;
	//get curr length and address
	buf = buf + offs;
	len = len - offs;

	offs = 0;       // current offset into buffer
	if (!buf || len < 6) return -1;

	//'buf' start of the EXIF TIFF, which must be the bytes "Exif\0\0".
	if (!std::equal(buf, buf + 6, "Exif\0\0")) return -1;
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

	if (offs + 8 > len) return -1;
	bool motorola_code;

	if (memcmp(buf + offs, "II", 2) == 0)
	{
		std::cout << ("Intel driver") << std::endl;
		motorola_code = false;
	}
	else
	{
		if (memcmp(buf + offs, "MM", 2) == 0)
		{
			std::cout << "MOTO driver" << std::endl;
			motorola_code = true;
		}
		else
		{
			return -1;
		}
	}

	offs += 2;

	if (Get16u(buf + offs, motorola_code) != 0x2a)
	{
		return -1;
	}

	offs += 2;

	int FirstOffset = Get32u(buf + offs, motorola_code);
	if (FirstOffset < 8 || FirstOffset > 16) {
		// used to ensure this was set to 8 (website indicated its 8)
		// but PENTAX Optio 230 has it set differently, and uses it as offset.
		std::cout << "Suspicious offset of first IFD value" << std::endl;
		return -1;
	}


	result = static_cast<image_data*>(malloc(sizeof(image_data)));
	clear(result);

	/* First directory starts 16 unsigned chars in.  Offsets start at 8 unsigned chars in. */
	if (!ProcessExifDir(buf + 14, buf + OFFSETBASE, len - OFFSETBASE, result, motorola_code))
		return -1;



	// Dump EXIF information
	printf("Camera make          : %s\n", result->Make);
	printf("Camera model         : %s\n", result->Model);

	printf("Image width          : %d\n", result->ImageWidth);
	printf("Image height         : %d\n", result->ImageHeight);



	printf("Image date/time      : %s\n", result->DateTime);

	printf("Exposure time        : 1/%d s\n",
		static_cast<unsigned>(1.0 / result->ExposureTime));
	printf("F-stop               : f/%.1f\n", result->FNumber);
	printf("Exposure program     : %d\n", result->ExposureProgram);
	printf("ISO speed            : %d\n", result->ISOSpeedRatings);


	printf("GPS Latitude         : %f deg (%f deg, %f min, %f sec %c)\n",
		result->GeoLocation.Latitude, result->GeoLocation.LatComponents.degrees,
		result->GeoLocation.LatComponents.minutes,
		result->GeoLocation.LatComponents.seconds,
		result->GeoLocation.LatComponents.direction);
	printf("GPS Longitude        : %f deg (%f deg, %f min, %f sec %c)\n",
		result->GeoLocation.Longitude, result->GeoLocation.LonComponents.degrees,
		result->GeoLocation.LonComponents.minutes,
		result->GeoLocation.LonComponents.seconds,
		result->GeoLocation.LonComponents.direction);
	printf("GPS Altitude         : %f m\n", result->GeoLocation.Altitude);

	free(result);
	

	unmapFile(&mapDesc);
	return 0;
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
	image_data * const m_exifinfo, bool motorola)
{
	int a;

	int NumDirEntries = Get16u(DirStart, motorola);

	if ((DirStart + 2 + NumDirEntries * 12) > (OffsetBase + ExifLength)) {
		std::cout << "Illegally sized directory" << std::endl;
		return false;
	}

	for (int de = 0; de < NumDirEntries; de++) {
		unsigned char * ValuePtr;
		unsigned char * DirEntry = DirStart + 2 + 12 * de;

		int Tag = Get16u(DirEntry, motorola);
		int Format = Get16u(DirEntry + 2, motorola);
		int Components = Get32u(DirEntry + 4, motorola);
		unsigned data = Get32u(DirEntry + 8, motorola);

		if ((Format - 1) >= NUM_FORMATS) {
			/* (-1) catches illegal zero case as unsigned underflows to positive large */
			std::cout << "Illegal format code in EXIF dir" << std::endl;
			return false;
		}

		int BytesCount = Components * BytesPerFormat[Format];

		if (BytesCount > 4) {
			unsigned OffsetVal = Get32u(DirEntry + 8, motorola);
			/* If its bigger than 4 unsigned chars, the dir entry contains an offset.*/
			if (OffsetVal + BytesCount > ExifLength) {
				/* Bogus pointer offset and / or unsigned charcount value */
				std::cout << "Illegal pointer offset value in EXIF." << std::endl;
				return false;
			}
			ValuePtr = OffsetBase + OffsetVal;
		}
		else {
			/* 4 unsigned chars or less and value is in the dir entry itself */
			ValuePtr = DirEntry + 8;
		}

		unsigned gps_sub_ifd_offset;
		/* Extract useful components of tag */
		switch (Tag) {

		case TAG_MAKE:
			m_exifinfo->Make = reinterpret_cast<char*>(ValuePtr);
			break;

		case TAG_MODEL:
			m_exifinfo->Model = reinterpret_cast<char*>(ValuePtr);
			break;


		case TAG_DATETIME_ORIGINAL:
			m_exifinfo->DateTime = reinterpret_cast<char*>(ValuePtr);
			break;


		case TAG_FNUMBER:
			/* Simplest way of expressing aperture, so I trust it the most.
			(overwrite previously computd value if there is one)
			*/
			m_exifinfo->FNumber = static_cast<float>(ConvertAnyFormat(ValuePtr, Format, motorola));
			break;

		case TAG_GPS_INFO:

			gps_sub_ifd_offset = OFFSETBASE + data;
			getGPSData(ValuePtr, OffsetBase, ExifLength, m_exifinfo, gps_sub_ifd_offset, motorola);
			break;

		case TAG_EXPOSURETIME:
			/* Simplest way of expressing exposure time, so I
			trust it most.  (overwrite previously computd value
			if there is one)
			*/
			m_exifinfo->ExposureTime =
				static_cast<float>(ConvertAnyFormat(ValuePtr, Format, motorola));
			break;

		case TAG_SHUTTERSPEED:
			/* More complicated way of expressing exposure time,
			so only use this value if we don't already have it
			from somewhere else.
			*/
			if (m_exifinfo->ExposureTime == 0) {
				m_exifinfo->ExposureTime = static_cast<float>(1 / exp(ConvertAnyFormat(ValuePtr, Format, motorola) * log(2)));
			}
			break;


		case TAG_EXIF_IMAGELENGTH:
			a = static_cast<int>(ConvertAnyFormat(ValuePtr, Format, motorola));
			m_exifinfo->ImageHeight = a;
			break;
		case TAG_EXIF_IMAGEWIDTH:
			/* Use largest of height and width to deal with images
			that have been rotated to portrait format.
			*/
			a = static_cast<int>(ConvertAnyFormat(ValuePtr, Format, motorola));
			m_exifinfo->ImageWidth = a;
			break;


		case TAG_EXPOSURE_BIAS:
			m_exifinfo->ExposureBiasValue = static_cast<float>(ConvertAnyFormat(ValuePtr, Format, motorola));
			break;


		case TAG_EXPOSURE_PROGRAM:
			m_exifinfo->ExposureProgram = static_cast<int>(ConvertAnyFormat(ValuePtr, Format, motorola));
			break;

		case TAG_ISO_EQUIVALENT:
			m_exifinfo->ISOSpeedRatings = static_cast<int>(ConvertAnyFormat(ValuePtr, Format, motorola));
			if (m_exifinfo->ISOSpeedRatings < 50) m_exifinfo->ISOSpeedRatings *= 200;
			break;

		}

		if (Tag == TAG_EXIF_OFFSET || Tag == TAG_INTEROP_OFFSET) {

			unsigned char * SubdirStart = OffsetBase + Get32u(ValuePtr, motorola);
			if (SubdirStart < OffsetBase ||
				SubdirStart > OffsetBase + ExifLength) {
				printf("Illegal subdirectory link");
				return false;
			}
			ProcessExifDir(SubdirStart, OffsetBase, ExifLength, m_exifinfo, motorola);
		}
	}

	return true;
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

void getGPSData(unsigned char* value_ptr, unsigned char* offset_base, unsigned exif_length, image_data* const imdata, unsigned gps_sub_ifd_offset, bool motorola)
{
	u_char* buf = offset_base - OFFSETBASE;

	unsigned offs = gps_sub_ifd_offset;
	int num_entries = Get16u(buf + offs, motorola);
	if (offs + 6 + 12 * num_entries > exif_length) return;
	offs += 2;
	while (--num_entries >= 0) {
		
		unsigned char * ValuePtr;
		unsigned char * DirEntry = buf+offs;
		int Tag = Get16u(DirEntry, motorola);
		int Format = Get16u(DirEntry + 2, motorola);
		int Components = Get32u(DirEntry + 4, motorola);
		unsigned data = Get32u(DirEntry + 8, motorola);
		

		switch (Tag) {
		case 1:
			// GPS north or south
			imdata->GeoLocation.LatComponents.direction = *(buf + offs + 8);
			if (imdata->GeoLocation.LatComponents.direction == 0) {
				imdata->GeoLocation.LatComponents.direction = '?';
			}
			if ('S' == imdata->GeoLocation.LatComponents.direction) {
				imdata->GeoLocation.Latitude = -imdata->GeoLocation.Latitude;
			}
			break;

		case 2:
			// GPS latitude
			if ((Format == 5 || Format == 10) && Components == 3) {
				imdata->GeoLocation.LatComponents.degrees = ConvertAnyFormat(
					buf + data + OFFSETBASE,Format, motorola);
				imdata->GeoLocation.LatComponents.minutes = ConvertAnyFormat(
					buf + data + OFFSETBASE + 8,Format, motorola);
				imdata->GeoLocation.LatComponents.seconds = ConvertAnyFormat(
					buf + data + OFFSETBASE + 16,Format, motorola);
				imdata->GeoLocation.Latitude =
					imdata->GeoLocation.LatComponents.degrees +
					imdata->GeoLocation.LatComponents.minutes / 60 +
					imdata->GeoLocation.LatComponents.seconds / 3600;
				if ('S' == imdata->GeoLocation.LatComponents.direction) {
					imdata->GeoLocation.Latitude = -imdata->GeoLocation.Latitude;
				}
			}
			break;

		case 3:
			// GPS east or west
			imdata->GeoLocation.LonComponents.direction = *(buf + offs + 8);
			if (imdata->GeoLocation.LonComponents.direction == 0) {
				imdata->GeoLocation.LonComponents.direction = '?';
			}
			if ('W' == imdata->GeoLocation.LonComponents.direction) {
				imdata->GeoLocation.Longitude = -imdata->GeoLocation.Longitude;
			}
			break;

		case 4:
			// GPS longitude
			if ((Format == 5 || Format == 10) && Components == 3) {
				imdata->GeoLocation.LonComponents.degrees = ConvertAnyFormat(
					buf + data + OFFSETBASE,Format, motorola);
				imdata->GeoLocation.LonComponents.minutes = ConvertAnyFormat(
					buf + data + OFFSETBASE + 8, Format,motorola);
				imdata->GeoLocation.LonComponents.seconds = ConvertAnyFormat(
					buf + data + OFFSETBASE + 16,Format, motorola);
				imdata->GeoLocation.Longitude =
					imdata->GeoLocation.LonComponents.degrees +
					imdata->GeoLocation.LonComponents.minutes / 60 +
					imdata->GeoLocation.LonComponents.seconds / 3600;
				if ('W' == imdata->GeoLocation.LonComponents.direction)
					imdata->GeoLocation.Longitude = -imdata->GeoLocation.Longitude;
			}
			break;
		case 5:
			// GPS altitude reference (below or above sea level)
			imdata->GeoLocation.AltitudeRef = *(buf + offs + 8);
			if (1 == imdata->GeoLocation.AltitudeRef) {
				imdata->GeoLocation.Altitude = -imdata->GeoLocation.Altitude;
			}
			break;

		case 6:
			// GPS altitude
			if ((Format == 5 || Format == 10)) {
				imdata->GeoLocation.Altitude = ConvertAnyFormat(
					buf + data + OFFSETBASE,Format, motorola);
				if (1 == imdata->GeoLocation.AltitudeRef) {
					imdata->GeoLocation.Altitude = -imdata->GeoLocation.Altitude;
				}
			}
			break;

		

		}
		offs += 12;
	}

}


// Test program:
//--------------------------------------------------------------
int _tmain(int argc, _TCHAR* argv[])
{
	LPCTSTR image = L"C:\\Users\\andre\\Documents\\SO\\SO-Series\\Exinfo\\image\\IMG_20170412_113708.jpg";
	int toRet = getImageInfo(image);

	std::cout << "Press [Enter] to continue . . .";
	std::cin.get();

	return toRet;
}

