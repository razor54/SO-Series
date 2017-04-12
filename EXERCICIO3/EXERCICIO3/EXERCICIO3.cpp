// EXERCICIO3.cpp : Defines the entry point for the console application.
//
#include "stdafx.h"
#include "windows.h"
#include <iostream>
#include "psapi.h"
using namespace std;


unsigned long usage;


// For Test Purposes
void show_modules(HANDLE process) {

	unsigned char *p = nullptr;
	MEMORY_BASIC_INFORMATION info;
	

	for (p = nullptr;
		VirtualQueryEx(process, p, &info, sizeof(info)) == sizeof(info);
		p += info.RegionSize)
	{
		printf("%#10.10p (%6uK)\t", info.BaseAddress, info.RegionSize / 1024);

		switch (info.State) {
		case MEM_COMMIT:
			printf("Committed");
			break;
		case MEM_RESERVE:
			printf("Reserved");
			break;
		case MEM_FREE:
			printf("Free");
			break;
		}
		printf("\t");
		switch (info.Type) {
		case MEM_IMAGE:
			printf("Code Module");
			break;
		case MEM_MAPPED:
			printf("Mapped     ");
			break;
		case MEM_PRIVATE:
			printf("Private    ");
		}
		printf("\t");

		if ((info.State == MEM_COMMIT) && (info.Type == MEM_PRIVATE))
			usage += info.RegionSize;

		int guard = 0, nocache = 0;

		if (info.AllocationProtect & PAGE_NOCACHE)
			nocache = 1;
		if (info.AllocationProtect & PAGE_GUARD)
			guard = 1;

		info.AllocationProtect &= ~(PAGE_GUARD | PAGE_NOCACHE);

		switch (info.AllocationProtect) {
		case PAGE_READONLY:
			printf("Read Only");
			break;
		case PAGE_READWRITE:
			printf("Read/Write");
			break;
		case PAGE_WRITECOPY:
			printf("Copy on Write");
			break;
		case PAGE_EXECUTE:
			printf("Execute only");
			break;
		case PAGE_EXECUTE_READ:
			printf("Execute/Read");
			break;
		case PAGE_EXECUTE_READWRITE:
			printf("Execute/Read/Write");
			break;
		case PAGE_EXECUTE_WRITECOPY:
			printf("COW Executable");
			break;
		}

		if (guard)
			printf("\tguard page");
		if (nocache)
			printf("\tnon-cachable");
		printf("\n");
	}
}



size_t get_region(DWORD id)
{
	auto h = OpenProcess(
		PROCESS_VM_READ | PROCESS_QUERY_INFORMATION,
		false,
		id);

	if (h == nullptr)
		return 0;

	_MEMORY_BASIC_INFORMATION memory_info;


	SIZE_T maxSize = 0;
	SIZE_T currentSize=0;
	char* currAddress = nullptr;
	auto aux = VirtualQueryEx(h, currAddress, &memory_info, sizeof(memory_info));
	auto base = LPCVOID(memory_info.RegionSize);

	for (; aux == sizeof(memory_info); currAddress += memory_info.RegionSize) {

		currentSize = memory_info.RegionSize;
			if (currentSize > maxSize) {
				maxSize = currentSize;
		}


		aux = VirtualQueryEx(h, currAddress, &memory_info, sizeof(memory_info));
		
	}
	
	TCHAR Buffer[_MAX_FNAME];
	
	GetModuleBaseName(h, GetModuleHandle(nullptr), Buffer, _MAX_FNAME);
	
	wstring arr_w(Buffer);
	string arr_s(arr_w.begin(), arr_w.end());
	
	cout << "The base address is : " << base << endl;
	
	cout << "The process name is : " << arr_s.c_str() << endl;
	
	cout << "Max Region Size is " << maxSize << " Bytes"<< endl;

	return maxSize;
}

int main(int argc, char* argv[])
{
	auto id = GetCurrentProcessId();
	/*auto process = OpenProcess(
		PROCESS_VM_READ | PROCESS_QUERY_INFORMATION,
		false,
		id);

	show_modules(process);
	printf("Total memory used: %luKB\n", usage / 1024);
	*/
	
	get_region(id);

	
	cout << "Press [Enter] to continue . . .";
	cin.get();

	return 0;
}
