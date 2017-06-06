// TestApp1.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include <Windows.h>
#include <tchar.h>
#include <stdio.h>
#include <assert.h>
#include <iostream>
#include "../Test1/Header.h"
#include <list>

using namespace std;
// Struct to hold context to JPG_ProcessExifTags
typedef struct
{
	PCTSTR filepath;
	PCTSTR searchArgs;
	PCTCH startDate;
	PCTCH endDate;
} JPG_CTX, *PJPG_CTX;

// Callback to JPG_ProcessExifTags
BOOL ProcessExifTag(LPCVOID ctx, DWORD tag, LPCVOID value)
{
	PCTSTR searchArgs = PJPG_CTX(ctx)->searchArgs;
	DWORD tagToFind = _ttoi(searchArgs);
	BOOL bRes = tagToFind == tag;
	if (bRes)
	{
		PCTCH startDate = PJPG_CTX(ctx)->startDate;
		PCTCH endDate = PJPG_CTX(ctx)->endDate;
		PCTSTR filepath = PJPG_CTX(ctx)->filepath;

		PCHAR tts = PCHAR(value);
		
		
		wchar_t * vOut = new wchar_t[strlen(tts) + 1];
		mbstowcs_s(NULL, vOut, strlen(tts) + 1, tts, strlen(tts));

		if (wcscmp(vOut, startDate) > 0 && wcscmp(vOut, endDate) < 0)
			printf("%ls\n", filepath);
	}
	return !bRes;
}


DWORD WINAPI re_routine(LPVOID param)
{
	PJPG_CTX ctx = PJPG_CTX(param);
	PCTSTR filepath = ctx->filepath;

	exifo_pri_library::Test::JPEG_ProcessExifTags(PTCHAR(filepath), ProcessExifTag, &ctx);
	
	delete ctx;
	return 0;

	
}

VOID SearchFileDir(PCTSTR path, PCTSTR searchArgs, LPVOID ctx, PCTCH startDate, PCTCH endDate)
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
			PJPG_CTX jpgcts = new JPG_CTX{ filepath, searchArgs,startDate,endDate };
			//JPG_CTX jpgCtx = { filepath, searchArgs,startDate,endDate };
			//exifo_pri_library::Test::JPEG_ProcessExifTags(filepath, ProcessExifTag, &jpgCtx);
			
			QueueUserWorkItem(re_routine,jpgcts, WT_EXECUTEINIOTHREAD);
		}
	} while (FindNextFile(fileIt, &fileData) == TRUE);

	FindClose(fileIt);
}


DWORD _tmain(DWORD argc, PTCHAR argv[])
{
	//36867 -- tag localDate

	if (argc < 5)
	{
		_tprintf(_T("Use: %s <repository path> <tag exif> <start-date> <end-date>"), argv[0]);
		exit(0);
	}

	SearchFileDir(argv[1], argv[2], NULL,argv[3],argv[4]);

	std::cout << "Press [Enter] to continue . . .";
	std::cin.get();
	
	return 0;
}


/*
DWORD _tmain()
{
	PCTSTR ini = L"2003:05:23 16:40:33";
	PCTSTR end = L"2010:05:25 16:40:33";

	SearchFileDir(L"test-images/", L"36867", nullptr, ini, end);
	
	std::cout << "Press [Enter] to continue . . .";
	std::cin.get();

	return 0;
}
*/
