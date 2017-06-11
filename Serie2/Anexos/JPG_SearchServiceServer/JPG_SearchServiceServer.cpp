#include <Windows.h>
#include <stdio.h>
#include <assert.h>
#include "../Include/JPG_SearchService.h"
#include "../Include/JPEGExifUtils.h"
#include <list>
#include <tchar.h>
#include <xlocmon>

// Struct to hold context to JPG_ProcessExifTags
using namespace  std;

typedef struct _internal_ctx {
	PCHAR filepath; // Name to collect
	PCHAR searchArgs; // Filter to use in tag comparation

	list<PCHAR> * files;
	PLONG elem_number;
	PLONG curr;
	HANDLE event;
	HANDLE mutex;
} JPG_CTX, *PJPG_CTX;


BOOL ProcessExifTag(LPCVOID ctx, DWORD tag, LPVOID value) {
	PJPG_CTX thread_context = ((PJPG_CTX)ctx);
	PCHAR searchArgs = thread_context->searchArgs;
	DWORD tagToFind = atoi((PCHAR)searchArgs);
	BOOL bRes = tagToFind == tag;

	if (bRes) {
		PCHAR filepath = thread_context->filepath;
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
static POUTP SearchFileDir(PCHAR path, PCHAR searchArgs, LPVOID ctx) {

	CHAR buffer[MAX_PATH]; // auxiliary buffer
						   // the buffer is needed to define a match string that guarantees 
						   // a priori selection for all files
	sprintf_s(buffer, "%s%s", path, "*.*");

	WIN32_FIND_DATA fileData;
	HANDLE fileIt = FindFirstFile(buffer, &fileData);
	if (fileIt == INVALID_HANDLE_VALUE) return NULL;

	PLONG elem_number = new LONG();
	PLONG curr = new LONG();
	HANDLE event = CreateEvent(nullptr, TRUE, FALSE, nullptr);
	HANDLE mutex = CreateMutex(nullptr, FALSE, nullptr);
	list<PCHAR> files;
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

	
	DWORD number = (*elem_number);
	PCHAR * out = (PCHAR *)( malloc(sizeof(out)* number));
	//std::copy(files.begin(), files.end(), out);

	int index = 0;
	DWORD size = 0;
	for (list<PCHAR>::iterator list_iter = files.begin(); list_iter != files.end(); list_iter++){
		PCHAR iter = *list_iter;
		out[index] = iter;
		size += strlen(iter) + 1;
		++index;
	}

	FindClose(fileIt);


	return new OUTP{ number,out,size };
}



;//

HANDLE  ProcessRepository(PCSTR repository, PCSTR filter, DWORD serviceID) {

	//envio do handle da rmp

	POUTP out_files = SearchFileDir((PCHAR)repository, (PCHAR)filter, nullptr);
	unsigned region_size =out_files->fileSize;
	// criaçao da regiao de meoria partilhada
	HANDLE handle = CreateFileMapping(
		INVALID_HANDLE_VALUE,    // not a file but a page
		NULL,                    // default security
		PAGE_READWRITE,          // read/write access
		0,                       // maximum object size (high-order DWORD)
		region_size+ sizeof(DWORD),             // maximum object size (low-order DWORD)
		NULL);
	// duplicr o handle para o passar do server para a rsposta

	PCHAR map = (PCHAR) (MapViewOfFile(handle, FILE_MAP_ALL_ACCESS, 0, 0, 0));
	 ZeroMemory(map,region_size	);
	PCHAR adress = map;
	for (int i = 0; i < out_files->elem_number; ++i) {
		PCHAR str = out_files->elems[i];
		DWORD str_size = strlen(str)+1;
		strcpy_s(adress, str_size, str);
		adress += str_size;
		free(str);
	}
	free(out_files->elems);
	//memcpy(adress, &out_files->elem_number, sizeof(DWORD));
		
	HANDLE toret;
	if(!DuplicateHandle(
		GetCurrentProcess(),
		handle,
		OpenProcess(PROCESS_ALL_ACCESS, FALSE, serviceID),
		&toret,
		0,
		FALSE,
		DUPLICATE_SAME_ACCESS
	)) {
		printf("UNABLE TO DUPLICATE!");
	};
	return toret;
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