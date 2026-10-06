////////////////////////////////////////////////////////////////////////
//player.cpp
//
//This file contains the implementation of class ai, defined in player.h.
//The central function is ai::make_guess. It also contains a
//utility class used in combination with STL algorithms
//in ai::make_guess; and functions such as ai::reset(), which is used
//to maintain the data structures (pre-computations) which allow make_guess
//to work efficiently.

#include "player.h"
#include <cmath>
#include <vector>
#include <deque>
#include <map>
#include <algorithm>

using namespace std;

//utility class

class check_inconsistency
{
	const combination* mpC;
public:
	check_inconsistency(const combination* pC):mpC(pC) { }

	bool operator()(const pair<combination,grade>& p) const
	{
		if (mpC->score(&p.first) == p.second) return false;

		return true;
	}
};

//class ai
//

void ai::reset()
{
	mmConsistent.clear();
	mvConsistent.clear();

	combination c;

	do
	{
		mmConsistent[c.hash()] = true;
		mvConsistent.push_back(c);
	} while (++c);
}

ai::ai()
{
	combination c,d;

	reset();
}


combination ai::make_guess(const vector<pair<combination,grade> >* pV,
		bool bRandFirst)
{
	if (pV->size() == 0)
	{
		reset();

		if (!bRandFirst)
			return combination(0,0,4,5);
		else
		{
			//If random is requested, mix the colors and the order,
			//but still double exactly one color.
			
			unsigned char cColors[] = {0,1,2,3,4,5};
			random_shuffle(cColors,cColors + 6);

			unsigned char cGuess[] = {cColors[0],cColors[0],
				cColors[1],cColors[2]};
			random_shuffle(cGuess,cGuess + 4);

			return combination(cGuess[0],cGuess[1],cGuess[2],
					cGuess[3]);
		}
	}

	combination c;

	for (deque<combination>::iterator it = mvConsistent.begin(); it!= mvConsistent.end();)
	{
		if (find_if(pV->begin(),pV->end(),
					check_inconsistency(&(*it))) != pV->end())
		{
			mmConsistent[it->hash()] = false;
			it = mvConsistent.erase(it);
		}
		else ++it;
	}

	//cout << "Possibilities remaining: " << mvConsistent.size() << endl;

	if (mvConsistent.size() == 1) return *mvConsistent.begin();

	if (mvConsistent.size() == 0)
		throw bad_score();

	combination bestCombo;
	double max = 0;

	do
	{
		vector<unsigned short> vGradeCounts(15);

		for (deque<combination>::iterator it = mvConsistent.begin();
				it!=mvConsistent.end();++it)
		{
			++vGradeCounts[it->score(&c).hash()];
		}

		double cur = 0;
		int i;
		for (vector<unsigned short>::iterator it = vGradeCounts.begin();
				it != vGradeCounts.end(); ++it)
		{
			if (*it)
			{
				double p = static_cast<double>(*it) /
					static_cast<double>(mvConsistent.size());
				cur -= p * log(p) / log(2.0);
			}
		}

		//The following constants are empirically determined for approximately optimal performance.
		cur *= (mmConsistent[c.hash()]?1.00 + 1.20 / mvConsistent.size() : 1.00);


		if (cur > max)
		{
			max = cur;
			bestCombo = c;
		}
	} while (++c);
	
	//Uncommenting the following lines will cause the program to
	//observe on standard output whenever it chooses a guess
	//which it knows is wrong, because of its high
	//informativeness.
	
	/*if (!mConsistent[bestCombo.hash()])
		cout << "Choosing inconsistent with " << mvConsistent.size() << " left." << endl;*/

	return bestCombo;
}
