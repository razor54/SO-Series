// ComutationTime.cpp : Define o ponto de entrada para a aplicação de console.
//

#include "stdafx.h"
#include "windows.h"
static DWORD times = 10000000;


DWORD WINAPI switchToArg(LPVOID arg) {
	for (DWORD i = 0; i <= DWORD(arg); ++i) {
		SwitchToThread();
	}
	return 0;
}

DWORD CommutatingThreads(DWORD threadNumber) {
	LPVOID lp_parameter = LPVOID(threadNumber);

	DWORD tick_count = GetTickCount();

	HANDLE temp1 = CreateThread(nullptr, 0, switchToArg, lp_parameter, 0, nullptr);
	HANDLE tmp2 = CreateThread(nullptr, 0, switchToArg, lp_parameter, 0, nullptr);

	HANDLE array[2];
	array[0] = temp1;
	array[1] = tmp2;
	WaitForMultipleObjects(2, array, TRUE, INFINITE);
	DWORD final_tick_count = GetTickCount();


	return ((final_tick_count - tick_count));// (threadNumber);
}


DWORD CommutatingProcesses(DWORD threadNumber) {
	DWORD tick_count = GetTickCount();
	PROCESS_INFORMATION pi;
	STARTUPINFO si = { sizeof(si) };
	char* cmd = "java.exe";

	wchar_t cmd_w[50];
	// ReSharper disable once CppDeprecatedEntity
	mbstowcs(cmd_w, cmd, strlen(cmd) + 1);//Plus null
	if (!CreateProcess(nullptr, cmd_w, nullptr, nullptr, FALSE, 0, nullptr, nullptr, &si, &pi)) {
		printf("CreateProcess failed (%d).\n", GetLastError());
		return 0;
	}

	WaitForSingleObject(pi.hProcess, INFINITE);
	CloseHandle(pi.hThread);
	CloseHandle(pi.hProcess);
	DWORD final_tick_count = GetTickCount();


	return ((final_tick_count - tick_count) / threadNumber);
}

int main() {

	//printf("Commutation between threads is %d ms.\n", CommutatingThreads(times));
	printf("Commutation between Processes is %d ms.\n", CommutatingProcesses(20));

	getchar();
	return 0;
}
