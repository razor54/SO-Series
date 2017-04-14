// ClientTest.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include "Header.h"
#include <iostream>


int main()
{
	TCHAR* image = L"C:\\Users\\Leonardo Freire\\Desktop\\Faculdade\\2_Ano\\2_Semestre\\SO\\Series\\Serie1\\SO-Series\\Exinfo\\image\\bb-android.jpg";
	TestLibrary::Test::PrintExifTags(image);

	return 0;
}

