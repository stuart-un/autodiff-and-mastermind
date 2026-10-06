/////////////////////////////////////////////////////////
//
//
//test.cpp
//In this file is contained a program that tests the efficiency of the
//Mastermind AI algorithm deterministically by trying it on every possible
//combination. It turns off randomized first guessing in order to get
//a deterministic result.
//
//It should be compiled along with comb.cpp and player.cpp, and optimized
//highly (at least -O3 on g++).

#include <iostream>
#include <vector>
#include "player.h"
#include "comb.h"

using namespace std;

int main()
{
	combination key;
	
	ai comp;
	combination c;

	unsigned char count = 0;
	grade g;

	double dTotal = 0;
	int nMax = 0;

	unsigned int i = 0;

	do
	{
		key.print(cout);
		vector<pair<combination,grade> > v;
		do
		{
			c = comp.make_guess(&v,false);
			g = key.score(&c);

			v.push_back(pair<combination,grade>(c,g));
	
			++count;
		} while (!g.won());

		dTotal += count;
		if (count > nMax) nMax = count;

		cout << static_cast<unsigned int>(count) << endl;

		count = 0;

		if (++i % 100 == 0) cout << i << endl;
	} while(++key);

	dTotal /= i;

	cout << "Average: " << dTotal << endl;
	cout << "Highest: " << nMax << endl;
}
