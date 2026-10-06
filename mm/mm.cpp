////////////////////////////////////////////////////////////////////////
//mm.cpp
//
//In this file is contained a program that uses the Mastermind AI to
//guess a user's combination. The user scores each guess, and the AI
//attempts to guess the combination. In order to avoid predictability,
//randomized first guess is turned on.
//
//It should be compiled along with comb.cpp and player.cpp.

#include <iostream>
#include <cstdlib>
#include <ctime>
#include <vector>
#include "player.h"
#include "comb.h"

using namespace std;

void machine_guess();

int main()
{
	srand(time(0));

	machine_guess();
	return 0;

}

void machine_guess()
{
	vector<pair<combination, grade> > v;

	grade g;

	ai comp;
	unsigned short i = 0;

	do
	{
		combination c;

		try
		{
			c = comp.make_guess(&v);
		}
		catch(exception& ex)
		{
			cout << ex.what() << " Exiting." << endl;
			return;
		}

		cout << "Guess " << ++i << ": ";
		c.print(cout);

		cout << "How many red? ";
		unsigned short r,w;
		cin >> r;
	
		cout << "How many white? ";
		cin >> w;

		v.push_back(pair<combination, grade>
			(c,g = grade(static_cast<unsigned char>(r),
			static_cast<unsigned char>(w))));

	} while (!g.won());
}
