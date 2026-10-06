////////////////////////////////////////////////////////////////////////
//comb.h
//
//In this file are contained the classes grade and combination. grade
//represents a score (some number of red and white scoring pegs), and
//is essentially just a data structure with utility functions. combination
//represents a 4-peg, 6-color Mastermind combination, and includes utility
//functions, including the all-important combination::score, 
//which returns a grade comparing another combination with *this. The
//latter is inlined, as it can be called in excess of 300 million times during
//an instance of test.

#ifndef __COMB_H
#define __COMB_H

#include <cstdlib>
#include <iostream>
#include <cstring>
#include <assert.h>

struct grade
{
	unsigned char r;
	unsigned char w;

	grade(unsigned char R = 0,unsigned char W = 0):r(R),w(W) { }

	grade& operator++()
	{
		if (r + ++w > 4)
		{
			w = 0;
			if (++r > 4) r = 0;
		}

		return *this;
	}

	bool operator==(const grade& rhs) const
	{
		return (rhs.r == r && rhs.w == w);
	}

	bool operator<(const grade& rhs) const
	{
		return (hash() < rhs.hash());
	}

	operator bool()
	{
		return (w != 0 || r != 0);
	}

	//Returns a number between 0 and 15. Grades are in bijection with this range.
	unsigned char hash() const
	{
		return (11 * r - r * r) / 2 + w;
	}

	bool won()
	{
		return (r == 4);
	}

	void print(std::ostream& of) const;
};

class combination
{
	unsigned char code[4];
	mutable unsigned long mhash;

public:
	grade score(const combination* pC) const;

	//Iterate through combinations.
	combination& operator++();

	void set(unsigned char c1,unsigned char c2, unsigned char c3,
		unsigned char c4)
	{
		code[0] = c1;
		code[1] = c2;
		code[2] = c3;
		code[3] = c4;
		mhash = 0;
	}

	combination():mhash(0)
	{
		set(0,0,0,0);
	}

	combination(unsigned char c1,unsigned char c2, 
		unsigned char c3, unsigned char c4):mhash(0)
	{
		set(c1,c2,c3,c4);
	}

	combination(const combination& rhs):mhash(0)
	{
		set(rhs.code[0],rhs.code[1],rhs.code[2],rhs.code[3]);
	}

	combination& operator=(const combination& rhs)
	{
		set(rhs.code[0],rhs.code[1],rhs.code[2],rhs.code[3]);

		return *this;
	}

	//Puts combinations in bijection with the numbers from 0 to 1295.
	inline unsigned long hash() const
	{
		if (mhash != 0) return mhash;

		mhash = code[0] * 216 + code[1] * 36 + code[2] * 6 + code[3];

		return mhash;
	}

	//Generates a completely random combination. Uses rand().
	void generate();

	operator bool() const
	{
		return (hash() != 0);
	}

	inline bool operator<(const combination& rhs) const
	{
		return (hash() < rhs.hash());
	}
	bool operator==(const combination& rhs) {return (hash() == rhs.hash());}

	void print(std::ostream& of);
};


inline grade combination::score(const combination* pC) const
{
	int r = 0, w = 0;
	
	unsigned char vcThis[4] = {code[0],code[1],code[2],code[3]};
	unsigned char vcOther[4] = {pC->code[0],pC->code[1],pC->code[2],pC->code[3]};
	
	for (int i = 0; i < 4; ++i)
	{
		if (vcThis[i] == vcOther[i])
		{
			++r;
			vcOther[i] = 0xFF;
			vcThis[i] = 0xFE;
		}
	}

	int i;
	for (i = 0; i < 4;++i)
	{
		for (int j = 0; j < 4;++j)
		{
			if (i != j && vcOther[j] == vcThis[i])
			{
				++w;
				vcOther[j] = 0xFF;
				vcThis[i] = 0xFE;
				break;
			}
		}
	}

	return grade(r,w);
}

#endif
