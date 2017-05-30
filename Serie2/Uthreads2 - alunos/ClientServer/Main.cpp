/////////////////////////////////////////////
//
// CCISEL 
// 2007-2010
//
// Client/Server implementations
//
// Jorge Martins, 2014
////////////////////////////////////////////

#include "stdafx.h"

VOID FillData(LONG vals[], INT size) {
	for (int i = 0; i < size; ++i) {
		vals[i] = i + 1;
	}
}
 
BOOL ResultOk( LONG vals[], INT size) {
	BOOL error = FALSE;
	printf("Results: ");
	for (int i = 0; i < size; ++i) {
		if (vals[i] != 2 * (i + 1)) {
			error = true;
			break;
		}
	}
	return !error;
}
 
#define VALS_SIZE	10000
LONG vals[VALS_SIZE];

int main () { 
	FillData(vals, VALS_SIZE);

	UtInit();

	ClientServer(vals, VALS_SIZE);
	BOOL success = ResultOk(vals, VALS_SIZE);
	printf("Test done with %s\n", success ? "success" : "error");
	
	UtEnd();


	getchar();
	return 0;
}
