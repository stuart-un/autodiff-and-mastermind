#include <iostream>
#include <stack>
#include <iomanip>
#include <numeric>
#include <cmath>
#include <random>
#include <deque>
#include <algorithm>
#include <map>
#include <limits>
#include <iterator>
#include <memory>
#include <vector>

using namespace std;

class value;

double randor(double a, double b)
{
	static random_device rd;
	static mt19937 gen(rd());

	return a + generate_canonical<double,numeric_limits<double>::digits>(gen)*(b - a);
}

class gradient_propagator
{
	public:
		virtual ~gradient_propagator() {}

		virtual void back_propogate(const value*) = 0;
};

class plus_propagator;
class relu_propagator;
class times_propagator;
class pow_propagator;
class abs_propogator;


class value
{
	struct vimpl
	{
		double mdat;
		mutable double mgrad;
		vector<value> mparents;
		shared_ptr<gradient_propagator> mgp;
		mutable bool mbadded;

		vimpl(double dat,vector<value> parents,gradient_propagator* gp):mdat(dat),mgrad(0),mparents(parents),mgp(gp),mbadded(false){}
	};
	shared_ptr<vimpl> mpimpl;
	public:
		value(double dat,const vector<value>& parents = vector<value>(),gradient_propagator* gp = 0):mpimpl(new vimpl(dat,parents,gp)) { }

		value(const value& rhs):mpimpl(rhs.mpimpl){}

		double get_dat() const {return mpimpl->mdat;}

		double get_grad() const {return mpimpl->mgrad;}

		void add_grad(double d) const {mpimpl->mgrad += d;}

		void add_data(double d) {mpimpl->mdat += d;}

		value operator+(const value& rhs) const;
		value operator*(const value& rhs) const;
		value pow(double exp) const;
		value relu() const;
		value abs() const;

		
		value operator/(const value& rhs) const
		{
			return *this * rhs.pow(-1);
		}
		
		value operator-(const value& rhs) const
		{
			return *this + value(-1.0) * rhs;
		}

		
		void clear() const
		{
			mpimpl->mgrad = 0.0;
			mpimpl->mbadded = false;
		}

	private:
		template<class T>
		void enum_parents(T add_it) const
		{
			for_each(mpimpl->mparents.begin(),mpimpl->mparents.end(),[add_it](const value& v){v.enum_parents(add_it);});

			if (!mpimpl->mbadded && (mpimpl->mbadded = true)) *add_it++ = this;
		}

		void _backward() const
		{
			if (mpimpl->mgp) mpimpl->mgp->back_propogate(this);
		}

	public:
		void backwards()
		{
			mpimpl->mgrad = 1.0;

			deque<const value*> vals;

			enum_parents(front_inserter(vals));

			for_each(vals.begin(),vals.end(),[](const value*& v){v->_backward();});
		}

};

class plus_propagator:public gradient_propagator
{
	value ml,mr;

	public:
		virtual void back_propogate(const value*);
		
		plus_propagator(const value l,const value r):ml(l),mr(r) {}

};

class times_propagator: public gradient_propagator
{
	value ml,mr;

	public:
		virtual void back_propogate(const value*);

		times_propagator(const value l,const value r):ml(l),mr(r) {}
};

class relu_propagator: public gradient_propagator
{
	value mp;

	public:
		relu_propagator(const value p):mp(p) {}

		virtual void back_propogate(const value*);

};

class abs_propagator: public gradient_propagator
{
	value mp;

	public:
		abs_propagator(value p):mp(p){}

		virtual void back_propogate(const value*);
};

class pow_propagator: public gradient_propagator
{
	value mp;
	double mexp;

	public:
		pow_propagator(const value p,double exp):mp(p),mexp(exp) {}

		virtual void back_propogate(const value*);
};


void plus_propagator::back_propogate(const value* v)
{
	ml.add_grad(v->get_grad());
	mr.add_grad(v->get_grad());
}

void times_propagator::back_propogate(const value* v)
{
	ml.add_grad(mr.get_dat() * v->get_grad());
	mr.add_grad(ml.get_dat() * v->get_grad());
}

void pow_propagator::back_propogate(const value* v)
{
	mp.add_grad(mexp*pow(mp.get_dat(),mexp - 1)*v->get_grad());
}

void relu_propagator::back_propogate(const value* v)
{
	if (mp.get_dat() > 0) mp.add_grad(v->get_grad());
}
 void abs_propagator::back_propogate(const value* v)
{
	mp.add_grad(v->get_grad() * (mp.get_dat() >= 0? 1.0 : -1.0));
}

value value::operator+(const value& rhs) const
{
	vector<value> v;
	v.push_back(*this);
	v.push_back(rhs);

	return value(mpimpl->mdat + rhs.mpimpl->mdat,v,new plus_propagator(*this,rhs));
}

value value::operator*(const value& rhs) const
{
	vector<value> v;
	v.push_back(*this);
	v.push_back(rhs);

	return value(mpimpl->mdat*rhs.mpimpl->mdat,v,new times_propagator(*this,rhs));
}

value value::pow(double exp) const
{
	vector<value> v(1,*this);

	return value(std::pow(mpimpl->mdat,exp),v,new pow_propagator(*this,exp));
}

value value::relu() const
{
	vector<value> v(1,*this);

	return value(mpimpl->mdat > 0?mpimpl->mdat:0.0,v,new relu_propagator(*this));
}

value value::abs() const
{
	vector<value> v(1,*this);

	return value(std::abs(mpimpl->mdat),v,new abs_propagator(*this));
}



class neuron
{
	vector<value> mv;
	value md;
	bool mbrel;

	public:

	neuron(unsigned short nin,bool brel):mbrel(brel),md(value(randor(-1,1)))
	{
		generate_n(back_inserter(mv),nin,[](){return value(randor(-1,1));});
	}

	void clear() const
	{
		for_each(mv.begin(),mv.end(),[](const value& v){v.clear();});
		md.clear();
	}

	void shift(double dx)
	{
		for_each(mv.begin(),mv.end(),[dx](value& v){v.add_data(-v.get_grad()*dx);});
		md.add_data(-md.get_grad()*dx);
	}

	template<class T>
	value operator()(T vals) const
	{
		value v = inner_product(mv.begin(),mv.end(),vals,md);
		return mbrel? v.relu():v;
	}
};

class layer
{
	vector<neuron> mbrain;
	public:
		layer(unsigned short nin,unsigned short nout,bool brel = true)
		{
			generate_n(back_inserter(mbrain),nout,[nin,brel](){return neuron(nin,brel);});
		}

		void clear() const
		{
			for_each(mbrain.begin(),mbrain.end(),[](const neuron& n){n.clear();});
		}

		template<class outIt,class InIt>
		void operator()(outIt out,InIt in) const
		{
			transform(mbrain.begin(),mbrain.end(),out,[in](const neuron& n){return n(in);});
		}

		void shift(double dx)
		{
			for_each(mbrain.begin(),mbrain.end(),[dx](neuron& n){n.shift(dx);});
		}
};

class mlp
{
	vector<layer> mlays;

	public:
		template<class T>
		mlp(T begin,T end,unsigned short lastout = 1)
		{
			unsigned short nin = *begin++;
			unsigned short nout;
			
			for(T it = begin;it != end;++it)
			{
				nout = *it;

				mlays.push_back(layer(nin,nout,true));
				nin = nout;
			}
			mlays.push_back(layer(nin,lastout,false));
		}

		void clear() const
		{
			for_each(mlays.begin(),mlays.end(),[](const layer& lay){lay.clear();});
		}

		void shift(double dx)
		{
			for_each(mlays.begin(),mlays.end(),[dx](layer& lay){lay.shift(dx);});
		}

		vector<value> operator()(const vector<value>& v) const
		{
			vector<value> vout,vin;
			vector<value>::const_iterator it = v.begin();
			
			for_each(mlays.begin(),mlays.end(),[&vin,&vout,&it](const layer& lay)
					{
						vout.clear();
						lay(back_inserter(vout),it);
						vin = vout;
						it = vin.begin();
					});
			return vout;
		}
};

struct guess {
	int code[4];
	int guess[4];
	int red;
	int white;
};


void grade_guess(guess* g)
{
	g->red = 0;
	g->white = 0;
	int i;
	for (i = 0; i < 4; ++i)
	{
		if (g->code[i] == g->guess[i])
		{
			g->guess[i] -= 10;
			g->code[i] += 10;
			++g->red;
		}
	}

	for (i = 0; i < 4; ++i)
	{
		for (int j = 0; j < 4; ++j)
		{
			if (g->code[i] == g->guess[j])
			{
				g->guess[j] -= 10;
				++g->white;
				break;
			}
		}
	}

	for (i = 0; i < 4; ++i)
	{
		if(g->guess[i] < 0) g->guess[i] += 10;
		if (g->code[i]>5) g->code[i] -= 10;
	}
}

std::vector<guess> make_guesses(int n_samples = 250, double noise = 0.1, unsigned seed = 0) {
    static random_device rd;
    std::mt19937 rng(rd());
    std::uniform_int_distribution dist(0,5);

    std::vector<guess> guesses;

    //map<guess,unsigned short,decltype([](const guess& lhs,const guess& rhs){return (lhs.red < rhs.red || lhs.red == rhs.red && lhs.white < rhs.white);})> mgrades;

    for (int i = 0; i < n_samples;++i)
    {
	    guess g;
	    for_each(g.code,g.code+4,[&dist,&rng](int& n){n = dist(rng);});
	    for_each(g.guess,g.guess+4,[&dist,&rng](int&n){n = dist(rng);});
	    grade_guess(&g);
	    guesses.push_back(g);

	    ++mgrades[g];
    }

    //for_each(mgrades.begin(),mgrades.end(),[](auto p){cout << "(" << p.first.red << "," << p.first.white << "): " << p.second << endl;});


    return guesses;
}

int main()
{
	unsigned short lays[] = {8,20,20};
	mlp megatronbrain(lays,lays+sizeof(lays)/sizeof(unsigned short),2);

	double dx = 0.0001;

	double lasterr = 10000;

	for (int i = 0; i < 1000; ++i)
	{

		vector<guess> points = make_guesses();

		vector<pair<vector<value>,pair<double,double> > > vecpoints;

		transform(points.begin(),points.end(),back_inserter(vecpoints),[](const guess& g)
		{
			vector<value> v;
			transform(g.code,g.code + 4,back_inserter(v),[](double d){return value(d/2.5 - 1);});
			transform(g.guess,g.guess + 4,back_inserter(v),[](double d){return value(d/2.5 - 1);});
			pair<double,double> p;
			p.first = (g.red/2.0 - 1);
			p.second = (g.white/2.0 - 1);

			return pair<vector<value>,pair<double,double> >(v,p);
		});

		double err = 0;

		for (auto it = vecpoints.begin(); it!= vecpoints.end(); ++it)
		{
			megatronbrain.clear();

			vector<value> vt = it->first;
			vector<value> output = megatronbrain(it->first);
			pair<double,double> p = it->second;
			value loss = (output[0] - value(it->second.first)).abs() + (output[1] - value(it->second.second)).abs();
			
			err += loss.get_dat();

			loss.backwards();

			megatronbrain.shift(dx);

		}
		cout << "Run " << i + 1 << ": " << fixed << setprecision(10) << "dx: " << dx << " Error: " << err << endl;

		if (err == 0) break;
		if (lasterr > err && lasterr / err < 1.01) dx *= 1.02;
		else if (err > lasterr*1.05) dx /= 1.02;
		lasterr = err;


	}

	while(true) //Ctrl+C to terminate.
	{

	cout << "OK, enter each color (0-5) and press enter:" << endl;
	guess g;
	cin >> g.code[0] >> g.code[1] >> g.code[2] >> g.code[3];
	cout << "Now enter the guess." << endl;
	cin >> g.guess[0] >> g.guess[1] >> g.guess[2] >> g.guess[3];

	vector<value> v;

	transform(g.code,g.code + 4,back_inserter(v),[](double d){return value(d/2.5 - 1);});
	transform(g.guess,g.guess + 4,back_inserter(v),[](double d){return value(d/2.5 - 1);});

	vector<value> output = megatronbrain(v);
	cout << output[0].get_dat() << endl << output[1].get_dat() << endl;
	grade_guess(&g);
	cout << g.red << endl << g.white << endl;

	cout << "Reds:" << round((output[0].get_dat() + 1)*2) << endl;
	cout << "Whites:" << round((output[1].get_dat() + 1)*2) << endl;
	}

	return 0;
}
