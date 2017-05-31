#include "stdafx.h"
#include "../Include/UThread.h"
#include <iostream>

using namespace std;

HANDLE tA, tB, tC;
HANDLE *handle;

VOID ThreadA(UT_ARGUMENT Argument) {
	UCHAR Char = UCHAR(Argument);

	printf("Thread %c start\n", Char);
	UtJoin(tA);
	printf("Thread %c end\n", Char);
}
VOID ThreadB(UT_ARGUMENT Argument) {
	UCHAR Char = UCHAR(Argument);

	printf("Thread %c start\n", Char);
	UtJoin(tB);
	printf("Thread %c end\n", Char);
}
VOID ThreadC(UT_ARGUMENT Argument) {
	UCHAR Char = UCHAR(Argument);

	printf("Thread %c start\n", Char);
	UtJoin(tC);
	printf("Thread %c end\n", Char);
}

// Test MultJoin
VOID MultJoinTest() {
	printf("\n :: MultJoin Test 1 - BEGIN :: \n\n");
	tA = UtCreate(ThreadA, UT_ARGUMENT('A'));
	tB = UtCreate(ThreadB, UT_ARGUMENT('B'));
	tC = UtCreate(ThreadC, UT_ARGUMENT('C'));

	UtRun();
	handle = new HANDLE[3];

	handle[0] = tA;
	handle[1] = tB;
	handle[2] = tC;
	BOOL finished = UtMultJoin(handle, 3);

	printf(finished ? "TRUE" : "FALSE");
	printf("\n\n :: Test 1 - END :: \n");
}

int main() {
	UtInit();
	MultJoinTest();
	printf("Press any key to finish");
	getchar();
	
	UtEnd();
	return 0;
}