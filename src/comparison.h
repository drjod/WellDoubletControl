#ifndef WDCUTILITIES_H
#define WDCUTILITIES_H

#include <iostream>
#include <cmath>
#include <cassert>
#include <memory>

namespace wdc
{

struct Greater;
struct Smaller;

struct ComparisonMethod
{
        virtual bool execute(const double& x, const double& y) const = 0;
        virtual ~ComparisonMethod() {}
};

struct Greater : public ComparisonMethod
{
        double epsilon;
        Greater(const double& _epsilon) { epsilon = _epsilon; }
        bool execute(const double& x, const double& y) const 
        { return x > y + epsilon; }
};

struct Smaller : public ComparisonMethod
{
        double epsilon;
        Smaller(const double& _epsilon) { epsilon = _epsilon; }
        bool execute(const double& x, const double& y) const
        { return x < y - epsilon; }
};

struct Comparison
{
        std::unique_ptr<ComparisonMethod> method;

        Comparison() = default;
        Comparison(ComparisonMethod* _method) : method(std::unique_ptr<ComparisonMethod>(_method)) {}

        void configure(ComparisonMethod* _method)
	{
		method = std::unique_ptr<ComparisonMethod>(_method);
	}

        bool operator()(const double& x, const double& y) const
        {
                return method->execute(x, y);
        }

};

inline double confined(const double& value, const double& lower_limit, const double& upper_limit)
{
	return fmin(fmax(value, lower_limit), upper_limit);
}


inline int sign(double x)
{
	return (x>0.) ? 1 : ((x<0.) ? -1 : 0);
}


enum threshold_t{lower, upper};

inline double threshold(double value, double threshold_value, double delta, threshold_t threshold)
{
	assert(delta != 0);
	int sigma = (threshold == lower)? 1: -1;
	double U = sigma * (value - threshold_value) / delta;

	//std::cout << "U: " << U << std::endl;
	if(U <= 0)
		return 0;
	else if(0 < U && U < 1)
		return pow(U, 2*(1-U));
	else
		return 1;
}


}  // end namespace wdc


#endif
