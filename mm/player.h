////////////////////////////////////////////////////////////////////////
//player.h
//
//This file defines a virtual player class which represents a player making
//guesses in Mastermind. The only function, which is pure virtual, is
//player::make_guess, which accepts a vector of previous guesses and their
//scores, and returns the next guess.
//
//The file also defines an implementation (concrete child class), ai,
//which implements make_guess using an ai algorithm. The current implementation
//takes up to 6 guesses, and an average of 4.37269 guesses (over all
//combinations).
//
//Finally, the class contains an exception which is thrown if bad
//grading information is received.

#ifndef _PLAYER__H
#define _PLAYER__H

#include "comb.h"
#include <map>
#include <deque>
#include <vector>
#include <exception>
#include <utility>

class bad_score: public std::exception
{
public:
		
	bad_score() {}

	virtual const char* what() const throw ()
	{
		try
		{
			return "Incorrect grade given: no consistent "\
				"codes remain.";
		}
		catch (...)
		{
			return 0;
		}
	}
};

class player
{
public:
	virtual combination make_guess(const std::vector<std::pair<combination,
	grade> >* pV) = 0; 
};

class ai : public player
{
	std::deque<combination> mvConsistent;
	std::map<unsigned long, bool> mmConsistent;
public:
	ai();
	void reset();

	virtual combination make_guess(
		const std::vector<std::pair<combination,grade> >* pV)
	{
		return make_guess(pV,true);
	}

	combination make_guess(const std::vector<std::pair<combination,grade> >*
			pV,bool bRandFirst);
};

#endif
