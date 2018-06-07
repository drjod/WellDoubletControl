#ifndef WELLDOUBLETCONTROL_H
#define WELLDOUBLETCONTROL_H

#include<string>
#include <math.h>
#include <iostream>
#include "comparison.h"
#include "fakeSimulator.h"
#include "parameter.h"

class FakeSimulator;


class WellDoubletControl
{
public:
	enum iterationState_t {searchingFlowrate, searchingPowerrate, converged};
	// schemes A/C: first flow rate is adapted, then powerrate
	// scheme B: only powerrate is adapted
	struct result_t
	{
		double Q_H, Q_w;  // power rate Q_H (given input, potentially adapted),
                                // flow rate Q_w either calculated
                                // (schemes A and C) or set as input (Scheme B)
        	double T1, T2;  // temperature at well 1 ("warm well") 
                        // and 2 ("cold well") obtained from simulation results
        	bool flag_powerrateAdapted;  // says if storage meets requirement or not
	};
protected:
	result_t result;

	char scheme_identifier; // 'A', 'B', or 'C'	
	double value_target, value_threshold;
	// A: T1_target, Q_w_max 
	// B: Q_w_target, T1_max 
	// C: DT_target, Q_w__max

	enum {storing, extracting} operationType;
	iterationState_t iterationState;

	FakeSimulator* simulator;  // to have access to parameters
	Comparison beyond, notReached;
public:
	WellDoubletControl() {}
	virtual ~WellDoubletControl() {}

	iterationState_t get_iterationState() { return iterationState; }
	const WellDoubletControl::result_t& get_result() const { return result; }

	virtual void configure() = 0;
	virtual void set_flowrate() = 0;
	virtual void evaluate_simulation_result() = 0;
	
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
};

class WellSchemeAC : public WellDoubletControl
{
        double temperature_well1() const { return result.T1; }  // to evaluate target
        double temperature_difference_well1well2() const { return result.T1 - result.T2; }
                                                        // to evaluate target
        double (WellSchemeAC::*simulation_result_aiming_at_target) () const;
	// to differentiate between scheme A and C
        // scheme A: pointing at temperature_Well1(),
        // scheme C: pointing at temperature_difference(),
public:
	WellSchemeAC(FakeSimulator* _simulator, const char& _scheme_identifier)
	{ simulator = _simulator; scheme_identifier = _scheme_identifier; }

	void configure();
	void evaluate_simulation_result();

        void set_flowrate();
       	void adapt_flowrate();
        void adapt_powerrate();
};

class WellSchemeB : public WellDoubletControl
{
public:
	WellSchemeB(FakeSimulator* _simulator, const char& _scheme_identifier)
	{ simulator = _simulator; scheme_identifier = _scheme_identifier; }

	void configure();
	void evaluate_simulation_result();

        void set_flowrate();
        void adapt_powerrate();
};

#endif
