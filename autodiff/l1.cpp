#include <iostream>
#include <iomanip>
#include <numeric>
#include <cmath>
#include <random>
#include <deque>
#include <algorithm>
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
		mlp(T begin,T end)
		{
			unsigned short nin = *begin++;
			unsigned short nout;
			
			for(T it = begin;it != end;++it)
			{
				nout = *it;

				mlays.push_back(layer(nin,nout,true));
				nin = nout;
			}
			mlays.push_back(layer(nin,1,false));
		}

		void clear() const
		{
			for_each(mlays.begin(),mlays.end(),[](const layer& lay){lay.clear();});
		}

		void shift(double dx)
		{
			for_each(mlays.begin(),mlays.end(),[dx](layer& lay){lay.shift(dx);});
		}

		value operator()(const vector<value>& v) const
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
			return *it;
		}
};

struct point {
    double x, y;
    int label; // 0 or 1
};

std::vector<point> make_moons(int n_samples = 25, double noise = 0.1, unsigned seed = 0) {
    static random_device rd;
    std::mt19937 rng(rd());
    std::normal_distribution<double> gauss(0.0, noise);

    std::vector<point> points;
    int n_per_class = n_samples / 2;

    for (int i = 0; i < n_per_class; ++i) {
        double angle = M_PI * i / n_per_class;
        double x = std::cos(angle) + gauss(rng);
        double y = std::sin(angle) + gauss(rng);
        points.push_back({x, y, 0});
    }

    for (int i = 0; i < n_per_class; ++i) {
        double angle = M_PI * i / n_per_class;
        double x = 1.0 - std::cos(angle) + gauss(rng);
        double y = 0.5 - std::sin(angle) + gauss(rng);
        points.push_back({x, y, 1});
    }

    return points;
}

int main()
{
	unsigned short lays[] = {2,16,16};
	mlp megatronbrain(lays,lays+sizeof(lays)/sizeof(unsigned short));

	double dx = 0.05;

	vector<point> points = make_moons();

	vector<pair<vector<value>,int> > vecpoints;

	transform(points.begin(),points.end(),back_inserter(vecpoints),[](const point& p){vector<value> v; v.push_back(p.x);v.push_back(p.y); return pair<vector<value>,int> (v,p.label);});

	for (int i = 0; i < 40; ++i)
	{

		double err = 0;

		for (auto it = vecpoints.begin(); it!= vecpoints.end(); ++it)
		{
			megatronbrain.clear();

			value loss = (value(1.0) - value(2*it->second - 1) * megatronbrain(it->first)).relu();
			
			err += loss.get_dat();

			loss.backwards();

			megatronbrain.shift(dx);

		}
		cout << fixed << setprecision(10) << err << endl;
	}

	return 0;
}
