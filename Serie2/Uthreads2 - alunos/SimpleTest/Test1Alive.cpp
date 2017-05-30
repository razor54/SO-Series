#include "stdafx.h"
#include "../Include/UThread.h"
#include <iostream>

using namespace std;

HANDLE tA, tB, tC;

VOID ThreadA(UT_ARGUMENT Argument) {
	UCHAR Char = UCHAR(Argument);
	printf("Is Alive ? %d \n", UtAlive(tA));
	printf("Thread %c start\n", Char);
	UtJoin(tB);
	printf("Thread %c end\n", Char);
}
VOID ThreadB(UT_ARGUMENT Argument) {
	UCHAR Char = UCHAR(Argument);

	printf("Thread %c start\n", Char);
	printf("Thread %c end\n", Char);
}
VOID ThreadC(UT_ARGUMENT Argument) {
	UCHAR Char = UCHAR(Argument);

	printf("Thread %c start\n", Char);
	UtJoin(tB);
	printf("Thread %c end\n", Char);
}


/*testing UtAlive*/
VOID AliveTest() {
	printf("\n :: Test 1 - BEGIN :: \n\n");
	tA = UtCreate(ThreadA, UT_ARGUMENT('A'));
	tB = UtCreate(ThreadB, UT_ARGUMENT('B'));
	tC = UtCreate(ThreadC, UT_ARGUMENT('C'));
	
	UtRun();
	cout <<"Is A alive ? "<< UtAlive(tA) << endl;
	printf("\n\n :: Test 1 - END :: \n");
}


int main() {
	UtInit();
	AliveTest();
	printf("Press any key to finish");
	getchar();
	UtEnd();
	return 0;
}

