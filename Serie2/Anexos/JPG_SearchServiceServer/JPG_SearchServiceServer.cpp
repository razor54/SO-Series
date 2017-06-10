#include <Windows.h>
#include <stdio.h>
#include <assert.h>
#include "../Include/JPG_SearchService.h"
#include "../Include/JPEGExifUtils.h"
#include <list>
#include <tchar.h>

// Struct to hold context to JPG_ProcessExifTags
using namespace  std;

typedef struct _internal_ctx {
	PCTSTR filepath; // Name to collect
	PCSTR searchArgs; // Filter to use in tag comparation

	list<PCTSTR> * files;
	PLONG elem_number;
	PLONG curr;
	HANDLE event;
	HANDLE mutex;
} JPG_CTX, *PJPG_CTX;


BOOL ProcessExifTag(LPCVOID ctx, DWORD tag, LPVOID value) {
	PJPG_CTX thread_context = ((PJPG_CTX)ctx);
	PCSTR searchArgs = thread_context->searchArgs;
	DWORD tagToFind = atoi((PCHAR)searchArgs);
	BOOL bRes = tagToFind == tag;

	if (bRes) {
		PCSTR filepath = thread_context->filepath;
		InterlockedIncrement(thread_context->elem_number);

		//WaitForSingleObject(thread_context->mutex, INFINITE);
		(*thread_context->files).push_back(filepath);

		//ReleaseMutex(thread_context->mutex);
	}

	return !bRes;
}

DWORD WINAPI work_routine(LPVOID param) {

	PJPG_CTX jpgCtx = PJPG_CTX(param);
	if (*jpgCtx->curr > 0)
		JPEG_ProcessExifTags(PCHAR(jpgCtx->filepath), ProcessExifTag, jpgCtx);

	if (InterlockedDecrement(jpgCtx->curr) <= 0) {
		SetEvent(jpgCtx->event);
	}
	return 0;
}

// @param ctx No need for this implementation.
static HANDLE SearchFileDir(PCSTR path, PCSTR searchArgs, LPVOID ctx) {

	CHAR buffer[MAX_PATH]; // auxiliary buffer
						   // the buffer is needed to define a match string that guarantees 
						   // a priori selection for all files
	sprintf_s(buffer, "%s%s", path, "*.*");

	WIN32_FIND_DATA fileData;
	HANDLE fileIt = FindFirstFile(buffer, &fileData);
	if (fileIt == INVALID_HANDLE_VALUE) return INVALID_HANDLE_VALUE;

	PLONG elem_number = new LONG();
	PLONG curr = new LONG();
	HANDLE event = CreateEvent(nullptr, TRUE, FALSE, nullptr);
	HANDLE mutex = CreateMutex(nullptr, FALSE, nullptr);
	list<PCTSTR> files;
	// Process directory entries
	do {
		if ((fileData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) != 0) {
			// Not processing "." and ".." files!
			if (strcmp(fileData.cFileName, ".") && strcmp(fileData.cFileName, "..")) {
				sprintf_s(buffer, "%s%s/", path, fileData.cFileName);
				// Recursively process child directory
				SearchFileDir(buffer, searchArgs, ctx);
			}
		}
		else {

			InterlockedIncrement(curr);
			CHAR  filepath[MAX_PATH];
			CHAR * filepath1 = new CHAR[MAX_PATH];
			_stprintf_s(filepath, _T("%s%s"), path, fileData.cFileName);
			memcpy(filepath1, &filepath, sizeof filepath);
			PJPG_CTX jpgCtx = new JPG_CTX{ filepath1 , searchArgs, &files,elem_number, curr,event,mutex };

			QueueUserWorkItem(work_routine, jpgCtx, WT_EXECUTEINIOTHREAD);
		}
	} while (FindNextFile(fileIt, &fileData) == TRUE);

	WaitForSingleObject(event, INFINITE);
	LONG buffer_size = sizeof(PCTSTR)*(*elem_number);



	// criaçao da regiao de meoria partilhada
	HANDLE handle = CreateFileMapping(
		INVALID_HANDLE_VALUE,    // not a file but a page
		NULL,                    // default security
		PAGE_READWRITE,          // read/write access
		0,                       // maximum object size (high-order DWORD)
		buffer_size,             // maximum object size (low-order DWORD)
		NULL);

	PCHAR view_of_file = static_cast<PCHAR>(MapViewOfFile(handle, FILE_MAP_WRITE, 0, 0, 0));

	PCHAR str = "tudo o tempo levou.";
	int index = 0;
	//copia dos valores para a rmp
	for (list<PCTSTR>::iterator list_iter = files.begin(); list_iter != files.end(); list_iter++)
	{

		view_of_file = _strdup(str);

		++index;
	}
	//	memcpy(view_of_file, map_view_of_file, buffer_size);
	FindClose(fileIt);
	return handle;
}





HANDLE ProcessRepository(PCSTR repository, PCSTR filter) {

	//envio do handle da rmp
	return  SearchFileDir(repository, filter, nullptr);

}

INT main(INT argc, PCHAR argv[]) {
	if (argc < 2) {
		printf("Use: %s <service name>", argv[0]);
		exit(0);
	}

	PCHAR serviceName = argv[1];

	PJPG_SEARCH_SERVICE service = JPG_SearchServiceCreate(serviceName);
	if (service == NULL) return 0;

	JPG_SearchServiceProcess(service, ProcessRepository);
	JPG_SearchServiceClose(service);

	return 0;
}