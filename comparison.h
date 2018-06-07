#ifndef WDCUTILITIES_H
#define WDCUTILITIES_H

#include <iostream>
#include <cmath>

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
        ComparisonMethod* method;

        Comparison() 
	{
		//std::cout << "construct comparison without method" << std::endl;
		method = 0; 
	}

        Comparison(ComparisonMethod* _method) : method(_method)
	{
		//std::cout << "construct comparison" << std::endl;
	}

	Comparison(Comparison& other)
	{
		//std::cout << "copy constructor" << std::endl;
		method = other.method;
		other.method = 0;
	}

	Comparison& operator=(Comparison&& other)
	{
		//std::cout << "move assignment" << std::endl;
		method = other.method;
		other.method = 0;
		return *this;
	}

        ~Comparison() 
	{ 
		//std::cout << "destruct comparison" << std::endl;
		if(method != 0) 
			delete method;
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


/*
threshold > 0 (storing) : looks if value at threshold (or larger)
threshold < 0 (extracting): looks if value at zero (or larger)
*/
inline bool at_maximum(const double& value, const double& threshold, const double &accuracy)
{
	return (threshold>0) ? value > threshold - accuracy : value > -accuracy;
}


/*
threshold > 0 (storing) : looks if value at zero (or smaller)
threshold < 0 (extracting): looks if value at threshold (or smaller)
*/
inline bool at_minimum(const double& value, const double& threshold, const double &accuracy)
{
	return (threshold>0) ? value < accuracy : value < threshold + accuracy;
}




}  // end namespace wdc


#endif
