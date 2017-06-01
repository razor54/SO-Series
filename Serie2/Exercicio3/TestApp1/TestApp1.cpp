// TestApp1.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include "../ConsoleApplication1/ExifProcess.h"
#include <iostream>


int main()
{
	TCHAR* image[] = { L"C:/Users/andre/Documents/SO/SERIE 2/Anexos/TestApp1/test-images/evil1.jpg",
		L"C:\\Users\\andre\\Documents\\SO\\SERIE 2\\Anexos\\TestApp1\\test-imageslens_info.jpg",
		L"C:\\Users\\andre\\Documents\\SO\\SERIE 2\\Anexos\\TestApp1\\test-images/lukas12p.jpg" 
	};

	CHAR* ini = "2003:05:23 16:40:33";
	CHAR* end = "2016:05:25 16:40:33";
	for (int i = 0; i < 3; i++) {
		

		exifo_pri_library::Test::PrintExifTags(image[i], ini, end);
	}
	std::cout << "Press [Enter] to continue . . .";
	std::cin.get();
}

