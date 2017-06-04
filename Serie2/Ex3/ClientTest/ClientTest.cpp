// TestApp1.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include <Windows.h>
#include <tchar.h>
#include <stdio.h>
#include <assert.h>
#include <iostream>
#include "../Test1/Header.h"


// Struct to hold context to JPG_ProcessExifTags
typedef struct
{
	PCTSTR filepath;
	PCTSTR searchArgs;
	PCHAR startDate;
	PCHAR endDate;
} JPG_CTX, *PJPG_CTX;

// Callback to JPG_ProcessExifTags
BOOL ProcessExifTag(LPCVOID ctx, DWORD tag, LPCVOID value)
{
	PCTSTR searchArgs = PJPG_CTX(ctx)->searchArgs;
	DWORD tagToFind = _ttoi(searchArgs);
	BOOL bRes = tagToFind == tag;
	if (bRes)
	{
		PCHAR startDate = PJPG_CTX(ctx)->startDate;
		PCHAR endDate = PJPG_CTX(ctx)->endDate;
		PCTSTR filepath = PJPG_CTX(ctx)->filepath;

		PCHAR tts = PCHAR(value);
		
		if (strcmp(tts, startDate) > 0 && strcmp(tts, endDate) < 0)
			printf("%ls\n", filepath);
	}
	return !bRes;
}


VOID SearchFileDir(PCTSTR path, PCTSTR searchArgs, LPVOID ctx, PCHAR startDate, PCHAR endDate)
{
	TCHAR buffer[MAX_PATH]; // auxiliary buffer
							// the buffer is needed to define a match string that guarantees 
							// a priori selection for all files
	_stprintf_s(buffer, _T("%s%s"), path, _T("*.*"));

	WIN32_FIND_DATA fileData;
	HANDLE fileIt = FindFirstFile(buffer, &fileData);
	if (fileIt == INVALID_HANDLE_VALUE) return;

	// Process directory entries
	do
	{
		if ((fileData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) != 0)
		{
			// Not processing "." and ".." files!
			if (_tcscmp(fileData.cFileName, _T(".")) && _tcscmp(fileData.cFileName, _T("..")))
			{
				_stprintf_s(buffer, _T("%s%s/"), path, fileData.cFileName);
				// Recursively process child directory
				SearchFileDir(buffer, searchArgs, ctx, startDate, endDate);
			}
		}
		else
		{
			// Process file archive
			TCHAR filepath[MAX_PATH];
			_stprintf_s(filepath, _T("%s%s"), path, fileData.cFileName);
			JPG_CTX jpgCtx = { filepath, searchArgs,startDate,endDate };
			exifo_pri_library::Test::JPEG_ProcessExifTags(filepath, ProcessExifTag, &jpgCtx);
		}
	} while (FindNextFile(fileIt, &fileData) == TRUE);

	FindClose(fileIt);
}

/*
DWORD _tmain(DWORD argc, PTCHAR argv[]) {
//36867 -- tag localDate

if (argc < 3) {
_tprintf(_T("Use: %s <repository path> <tag exif>"), argv[0]);
exit(0);
}

SearchFileDir(argv[1], argv[2], NULL);

PRESS_TO_FINISH("");

return 0;

}
*/


DWORD _tmain()
{
	PCHAR ini = "2003:05:23 16:40:33";
	PCHAR end = "2016:05:25 16:40:33";

	SearchFileDir(L"test-images/", L"36867", nullptr, ini, end);
	
	std::cout << "Press [Enter] to continue . . .";
	std::cin.get();

	return 0;
}

