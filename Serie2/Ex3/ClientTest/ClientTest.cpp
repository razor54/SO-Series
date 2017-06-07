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
#include "ClientTest.h"

using namespace std;
// Struct to hold context to JPG_ProcessExifTags
typedef struct
{
	PCTSTR filepath;
	PCTSTR searchArgs;
	PCTCH startDate;
	PCTCH endDate;
} JPG_CTX, *PJPG_CTX;


HANDLE mutex;
DWORD counter;
list<PCTSTR> files;
list<PJPG_CTX> jpgContexts;

// Callback to JPG_ProcessExifTags
BOOL ProcessExifTag(LPCVOID ctx, DWORD tag, LPCVOID value)
{
	PJPG_CTX context = PJPG_CTX(ctx);
	PCTSTR searchArgs = context->searchArgs;
	DWORD tagToFind = _ttoi(searchArgs);
	BOOL bRes = tagToFind == tag;

	if (bRes)
	{
		PCTCH startDate = context->startDate;
		PCTCH endDate = context->endDate;
		PCTSTR filepath = context->filepath;

		PCHAR tts = PCHAR(value);


		wchar_t * vOut = new wchar_t[strlen(tts) + 1];
		mbstowcs_s(NULL, vOut, strlen(tts) + 1, tts, strlen(tts));

		if (wcscmp(vOut, startDate) > 0 && wcscmp(vOut, endDate) < 0)
		{
			//printf("%ls\n", filepath);
			// save filepath to a list to print later
			files.push_back(filepath);
		}
	}

	return !bRes;
}

DWORD WINAPI workToDo(LPVOID param)
{
	PJPG_CTX ctx = PJPG_CTX(param);
	PCTSTR filepath = ctx->filepath;

	// each thread has to wait for her time to act
	WaitForSingleObject(mutex, INFINITE);

	exifo_pri_library::Test::JPEG_ProcessExifTags((TCHAR *)(filepath), ProcessExifTag, ctx);

	// decrements the number of threads that still have work to do
	counter--;
	// releases mutex for another thread to own it
	ReleaseMutex(mutex);

	// saves contexts to a list to delete them later
	jpgContexts.push_back(ctx);

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
			TCHAR* filepath1 = new TCHAR[MAX_PATH];
			_stprintf_s(filepath, _T("%s%s"), path, fileData.cFileName);
			wcscpy(filepath1, filepath);
			PJPG_CTX jpgctx = new JPG_CTX{ filepath1, searchArgs,startDate,endDate };
			
			QueueUserWorkItem(workToDo, jpgctx, WT_EXECUTEDEFAULT);
			// increments the number of threads that have work to do
			counter++;
		}
	} while (FindNextFile(fileIt, &fileData) == TRUE);

	FindClose(fileIt);
}


//DWORD _tmain(DWORD argc, TCHAR *argv[])
//{
//	//36867 -- tag localDate
//
//	if (argc < 5)
//	{
//		_tprintf(_T("Use: %s <repository path> <tag exif> <start-date> <end-date>"), argv[0]);
//		exit(0);
//	}
//
//	SearchFileDir(argv[1], argv[2], NULL,argv[3],argv[4]);
//
//	std::cout << "Press [Enter] to continue . . .";
//	std::cin.get();
//	
//	return 0;
//}



DWORD _tmain()
{
	PCTSTR ini = L"2003:05:23 16:40:33";
	PCTSTR end = L"2017:05:25 16:40:33";
	mutex = CreateMutex(NULL, FALSE, NULL);

	if (mutex == NULL)
		printf("Error creating mutex.");

	SearchFileDir(L"test-images/", L"36867", nullptr, ini, end);

	while (counter != 0);
	

	for (list<PCTSTR>::iterator list_iter = files.begin(); list_iter != files.end(); list_iter++)
	{
		printf("%ls\n", *list_iter);
	}
	
	for (list<PJPG_CTX>::iterator list_iter = jpgContexts.begin(); list_iter != jpgContexts.end(); list_iter++)
	{
		PJPG_CTX ctx = *list_iter;
		delete ctx->filepath;
	}

	

	//cout << "Press [Enter] to continue . . .";
	//cin.get();

	return 0;
}

