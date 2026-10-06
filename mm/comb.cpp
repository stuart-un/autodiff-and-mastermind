////////////////////////////////////////////////////////////////////////
//comb.cpp
//
//This file contains implementation of a few utility functions relating
//to grades and combinations.

#include <iostream>
#include <vector>
using namespace std;
#include "comb.h"

void grade::print(ostream& of) const
{
	of << "(" << static_cast<unsigned int>(r) << 
		static_cast<unsigned int>(w) << ")" << endl;
}

void combination::print(ostream& of)
{
	string sNames[6] = {"red","white","blue","green","orange","yellow"};
	for (int i = 0; i < 4; ++i)
	{
		of << sNames[static_cast<unsigned int>(code[i])];
		if (i < 3) of << ',';
	}

	of << endl;
}

void combination::generate()
{
	set(rand() % 6, rand() % 6, rand() % 6, rand() % 6);
}

combination& combination::operator++()
{
	unsigned long l;
	mhash = 0;

	for (int i = 0; i < 4;++i)
	{
		if (++code[i] < 6) return *this;
		code[i] = 0;
	}

	return *this;
}
