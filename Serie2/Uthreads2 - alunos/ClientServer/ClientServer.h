#ifndef _CLIENTSERVERTEST_H
#define _CLIENTSERVERTEST_H

typedef struct CLIENT_ARGS {
	LONG *numbers;
	int totalNumbers;
} CLIENT_ARGS;


VOID 	ClientServer0(LONG vals[], INT size);
VOID 	ClientServer(LONG vals[], INT size);


BOOL ResultOk(LONG vals[], INT size);

#endif
