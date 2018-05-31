#ifndef WELLDOUBLETCONTROL_H
#define WELLDOUBLETCONTROL_H

#include<string>
#define LOG(x)
//#define LOG(x) std::cout << x << std::endl 
#define EPSILON 1.e-2

#include <math.h>
#include <iostream>
#include "comparison.h"
#include "fakeSimulator.h"


class WellSchemeA;
class WellSchemeB;
class WellSchemeC;
class FakeSimulator;


struct WellDoubletCalculation
{
	double Q_H, Q_w;  // power rate, flow rate
	double T1, T2;  // temperature at well 1 ("warm well") and 2 ("cold well")
	
	bool flag_powerrateAdapted;

	void adapt_powerrate(WellSchemeA* scheme);
	void adapt_powerrate(WellSchemeB* scheme);
	void adapt_powerrate(WellSchemeC* scheme);

	void calculate_flowrate(WellSchemeA* scheme);
	void calculate_flowrate(WellSchemeB* scheme);
	void calculate_flowrate(WellSchemeC* scheme);
};


class WellDoubletControl
{
protected:
	WellDoubletCalculation result;
	double value_target, value_threshold;
	// A: T1_target, Q_w_max 
	// B: Q_w_target, T1_max 
	// C: DT_target, T1_max
	bool flag_storing;  // else extracting

	FakeSimulator* simulator;  // to have access to parameters
	Comparison beyondThreshold, check_for_flowrateAdaption;
public:
	WellDoubletControl() {}
	virtual ~WellDoubletControl() {}

	double& get_Q_H() { return result.Q_H; }
	double& get_Q_w() { return result.Q_w; }
	WellDoubletCalculation& get_result() { return result; }
	FakeSimulator* get_fakeSimulator() { return simulator; }

	virtual void initialize() = 0;
	virtual void calculate_flowrate() = 0;
	virtual bool check_result() = 0;
	
	void set_constraints(const double& _Q_H,  
		const double& _value_target, const double& _value_threshold);
			// constraints are set at beginning of time step
	void set_temperatures(const double& _T1, const double& _T2);
			// temperatures are updated in the iteration loop

	static WellDoubletControl* createWellDoubletControl(
				const char& selection, 
				FakeSimulator* simulator);
			// instance is created before time-stepping
	
	void print_temperatures();

	friend class WellDoubletCalculation;
};

class WellSchemeA : public WellDoubletControl
{
public:
	WellSchemeA(FakeSimulator* _simulator) { simulator = _simulator; }

	void initialize();
	void calculate_flowrate();
	bool check_result();
};

class WellSchemeB : public WellDoubletControl
{
public:
	WellSchemeB(FakeSimulator* _simulator) { simulator = _simulator; }

	void initialize();
	void calculate_flowrate();
	bool check_result();
};


class WellSchemeC : public WellDoubletControl
{

public:
	WellSchemeC(FakeSimulator* _simulator) { simulator = _simulator; }

	void initialize();
	void calculate_flowrate();
	bool check_result();
};

#endif
