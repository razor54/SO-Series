// ClientTest.cpp : Defines the entry point for the console application.
//


#include "stdafx.h"

#include "../Test1/Header.h"
#include <iostream>


int main()
{
	TCHAR* image = L"C:\\Users\\andre_000\\Documents\\SO\\so-series\\Exinfo\\image\\test1.jpg";
	exifo_pri_library::Test::PrintExifTags(image);

	std::cout << "Press [Enter] to continue . . .";
	std::cin.get();

	return 0;
}

