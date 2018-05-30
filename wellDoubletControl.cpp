#ifndef WELLDOUBLETCONTROL_CPP
#define WELLDOUBLETCONTROL_CPP

#define Log(x) std::cout << x << std::endl 
#include <math.h>
#include <iostream>
#include "comparison.h"

struct WellDoubletResult
{
	double Q_H, Q_w;  // power rate, flow rate
	double T1, T2;  // temperature at well 1 ("warm well") and 2 ("cold well")
	
	bool flag_powerrateAdapted;
};


class WellDoubletControl
{
protected:
	double epsilon;  // accuracy
	WellDoubletResult result;
	double value_target, value_threshold;
	// A: T1_target, Q_w_max 
	// B: Q_w_target, T1_max 
	// C: DT_target, T1_max

	double capacity;  // set in iteration loop later on

	Comparison beyondThreshold, check_for_flow_rate_adaption;
public:
	WellDoubletControl() : epsilon(1.e-5), capacity(5e6) {}
	virtual ~WellDoubletControl() {}
	virtual void initialize() = 0;

	void set_timeStepValues(const double& _Q_H,
		const double& _value_target, const double& _value_threshold) 
	{ 
		Log("set time step values");
		result.Q_H = _Q_H;
		value_target = _value_target;
		value_threshold = _value_threshold;

		result.flag_powerrateAdapted = false;
		initialize();
	}

	void set_iterationValues(const double& _T1, const double& _T2)  
	{
		Log("set iteration values");
		result.T1 = _T1;
		result.T2 = _T2;
	}

	virtual void calculate_flowrate() = 0;
	virtual void adapt_powerrate() = 0;
	virtual bool check_result() = 0;	
	WellDoubletResult& get_result()	{ return result; }

	static WellDoubletControl* createWellDoubletControl(
						const char& selection);
	
};

class WellSchemeA : public WellDoubletControl
{
public:
	void calculate_flowrate()
	{
		if(result.Q_H > 0)
			result.Q_w = fmin(result.Q_H / (capacity  * (result.T1 - result.T2)), value_threshold);
		else
			result.Q_w = fmax(result.Q_H / (capacity  * (result.T1 - result.T2)), value_threshold);
	}
	
	void adapt_powerrate()
	{
		Log("adapt power rate");
		result.Q_H = result.Q_w *  capacity * (result.T1 - result.T2);
		result.flag_powerrateAdapted = true;
	}
	
	void initialize()
	{
		Log("initialize");
		result.T1 = value_target;

		if(result.Q_H > 0.)
		{
			beyondThreshold = Comparison(new Greater(1.e-5));
			check_for_flow_rate_adaption =  Comparison(new Smaller(1.e-5));
		}
		else
		{
			beyondThreshold = Comparison(new Smaller(1.e-5));
			check_for_flow_rate_adaption = Comparison(new Greater(1.e-5));
		}
	}
	bool check_result()
	{
		Log("check result");
		if(beyondThreshold(result.T1, value_target))
			adapt_powerrate();
		else
		{
			if(check_for_flow_rate_adaption(result.T1, value_target))
				return true;  // iterate
		}
		return true;
	}
};

class WellSchemeB : public WellDoubletControl
{

public:
	void calculate_flowrate() { result.Q_w = value_target; }
	
	void adapt_powerrate()
	{
		result.Q_H = result.Q_w *  capacity * (value_threshold - result.T2);
		result.flag_powerrateAdapted = true;
	}

	void initialize()
	{
		if(result.Q_H > 0.)
			beyondThreshold = Comparison(new Greater(0.));
		else
			beyondThreshold = Comparison(new Smaller(0.));
	}

	bool check_result()
	{
		if(beyondThreshold(result.T1, value_threshold))
			adapt_powerrate();
		return false;
	}
};


class WellSchemeC : public WellDoubletControl
{

public:
	void calculate_flowrate()
	{
		result.Q_w = result.Q_H / (capacity *  value_target);
	}
	
	void adapt_powerrate()
	{
		result.Q_H = result.Q_w * capacity * (value_threshold - result.T2);
		result.flag_powerrateAdapted = true;
	}

	void initialize()
	{
		if(result.Q_H > 0.)
		{
			beyondThreshold = Comparison(new Greater(0.));
		}
		else
		{
			beyondThreshold = Comparison(new Smaller(0.));
		}
	}

	bool check_result()
	{
		if(beyondThreshold(result.T1, value_threshold))
		{
			adapt_powerrate();
			return true;
		}
		return false;
	}
};


WellDoubletControl* WellDoubletControl::createWellDoubletControl(
						const char& selection)
{
	switch(selection)
	{
		case  'A':
			return new WellSchemeA;
			break;
		case  'B':
			return new WellSchemeB;
			break;
		case  'C':
			return new WellSchemeC;
			break;
		default:
			break;
	}
	return 0;
}

#endif
