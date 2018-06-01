#ifndef WELLDOUBLETCONTROL_H
#define WELLDOUBLETCONTROL_H

#include<string>


#include <math.h>
#include <iostream>
#include "wellDoubletCalculation.h"
#include "comparison.h"
#include "fakeSimulator.h"
#include "parameter.h"

class FakeSimulator;


class WellDoubletControl
{
protected:
	char scheme_identifier; // 'A', 'B', or 'C'	
	WellDoubletCalculation result;
	double value_target, value_threshold;
	// A: T1_target, Q_w_max 
	// B: Q_w_target, T1_max 
	// C: DT_target, Q_w__max

	bool flag_storing;  // else extracting

	FakeSimulator* simulator;  // to have access to parameters
	Comparison beyond, notReached;
public:
	WellDoubletControl() {}
	virtual ~WellDoubletControl() {}

	const WellDoubletCalculation& get_result() const { return result; }
	FakeSimulator* get_fakeSimulator() { return simulator; }

	virtual void configure() = 0;
	virtual void provide_flowrate() = 0;
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
	
	void print_temperatures() const;

	friend class WellDoubletCalculation;
};

class WellSchemeAC : public WellDoubletControl
{
public:
	WellSchemeAC(FakeSimulator* _simulator, const char _scheme_identifier)
	{ simulator = _simulator; scheme_identifier = _scheme_identifier; }

	void configure();
	void provide_flowrate();
	bool check_result();
};

class WellSchemeB : public WellDoubletControl
{
public:
	WellSchemeB(FakeSimulator* _simulator, const char _scheme_identifier)
	{ simulator = _simulator; scheme_identifier = _scheme_identifier; }

	void configure();
	void provide_flowrate();
	bool check_result();
};

#endif
