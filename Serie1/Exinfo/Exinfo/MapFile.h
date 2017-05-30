// to avoid multiple includes 
#pragma once
#include <string>


// this struct maintains the state of an active mapping
// (or the error state if some error ocurrs in the map creation)
typedef  struct filemap {
	BYTE* baseAddress;		// base address of the map view 
	SIZE_T size;			// the size of the mapping (and view)
	HANDLE mapHandle;		// section object handle  
	DWORD  error;			// state (!= 0 means error)
} FILEMAP, *PFILEMAP;


typedef struct Data{
	char* Make;
	char* Model;

	char* DateTime;            


	double ExposureTime;             
	double FNumber;                   
	unsigned short ExposureProgram;   

	unsigned short ISOSpeedRatings;   
	double ShutterSpeedValue;         
	double ExposureBiasValue;         

	unsigned ImageWidth;              
	unsigned ImageHeight;           
	struct Geolocation_t {          
		double Latitude;            
		double Longitude;           
		double Altitude;            
		char AltitudeRef;                 // 0 = above sea level, -1 = below sea level
		struct Coord_t {
			double degrees;
			double minutes;
			double seconds;
			char direction;
		} LatComponents, LonComponents;   // Latitude, Longitude expressed in deg/min/sec
	} GeoLocation;

}image_data;


BOOL mapFile(LPCTSTR fileName, int access, int mode, LPCTSTR name, SIZE_T size, PFILEMAP fm);

//---------------------------------------------------------------------------
// Destroy the mapping resources (mapped region and the associated section object)
//------------------------------------------------------------------------------
VOID unmapFile(FILEMAP fm);


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

#define TAG_EXIF_VERSION      0x9000
#define TAG_EXIF_OFFSET       0x8769
#define TAG_INTEROP_OFFSET    0xa005

#define TAG_MAKE              0x010F
#define TAG_MODEL             0x0110

#define TAG_ORIENTATION       0x0112
#define TAG_XRESOLUTION       0x011A
#define TAG_YRESOLUTION       0x011B
#define TAG_RESOLUTIONUNIT    0x0128

#define TAG_EXPOSURETIME      0x829A
#define TAG_FNUMBER           0x829D

#define TAG_SHUTTERSPEED      0x9201
#define TAG_APERTURE          0x9202
#define TAG_BRIGHTNESS        0x9203
#define TAG_MAXAPERTURE       0x9205
#define TAG_FOCALLENGTH       0x920A

#define TAG_DATETIME_ORIGINAL 0x9003
#define TAG_USERCOMMENT       0x9286

#define TAG_SUBJECT_DISTANCE  0x9206
#define TAG_FLASH             0x9209

#define TAG_FOCALPLANEXRES    0xa20E
#define TAG_FOCALPLANEYRES    0xa20F
#define TAG_FOCALPLANEUNITS   0xa210
#define TAG_EXIF_IMAGEWIDTH   0xA002
#define TAG_EXIF_IMAGELENGTH  0xA003

#define TAG_EXPOSURE_BIAS     0x9204
#define TAG_WHITEBALANCE      0x9208
#define TAG_METERING_MODE     0x9207
#define TAG_EXPOSURE_PROGRAM  0x8822
#define TAG_ISO_EQUIVALENT    0x8827
#define TAG_COMPRESSION_LEVEL 0x9102

#define TAG_GPS_INFO 0x8825

#define TAG_THUMBNAIL_OFFSET  0x0201
#define TAG_THUMBNAIL_LENGTH  0x0202


#define OFFSETBASE 6 


void getGPSData(unsigned char* value_ptr, unsigned char* offset_base, unsigned exif_length, image_data* const data, unsigned gps_sub_ifd_offset, bool motorola);

void clear(image_data* data);
int Get16m(void * Short);
int Get16u(void * Short, bool MotorolaOrder);
long Get32s(void * Long, bool MotorolaOrder);
unsigned long Get32u(void * Long, bool motorola);
bool ProcessExifDir(unsigned char * DirStart, unsigned char * OffsetBase, unsigned ExifLength,
	image_data * const m_exifinfo, bool motorola);
double ConvertAnyFormat(void * ValuePtr, int Format, bool motorola);