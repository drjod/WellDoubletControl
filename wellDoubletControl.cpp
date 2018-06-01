#include "wellDoubletControl.h"
#include <stdexcept>

void WellDoubletControl::print_temperatures() const
{
	std::cout << "\t\t\tT1: " << result.T1 << " - T2: " <<
		result.T2 << std::endl;
}


void WellDoubletControl::set_constraints(const double& _Q_H,
	const double& _value_target, const double& _value_threshold) 
{
	// set input values for well doublet control, e.g. from file
	result.Q_H = _Q_H;  // stored (Q_H>0) or extracted (Q_H<0) heat

	value_target = _value_target;
	value_threshold = _value_threshold;

	result.flag_powerrateAdapted = false;
	result.flag_flowrateAdapted = false;

	if(_Q_H > 0.)
	{
		LOG("\t\t\tset power rate\t\t" << std::to_string(_Q_H) <<
			+ " - storing");
		flag_storing = true;
	}
	else
	{
		LOG("\t\t\tset power rate\t\t" << std::to_string(_Q_H) <<
			+ " - extracting");
		flag_storing = false;
	}
	configure();  // the scheme-dependent stuff (comparison functions)
}

void WellDoubletControl::set_temperatures(const double& _T1, const double& _T2)  
{
	result.T1 = _T1;  // warm well
	result.T2 = _T2;  // cold well
	LOG("\t\t\tset temperatures\tT1: " + std::to_string(_T1) +
					" - T2: " + std::to_string(_T2));
}



WellDoubletControl* WellDoubletControl::createWellDoubletControl(
				const char& selection, FakeSimulator* simulator)
{
	switch(selection)
	{
		case  'A':
			return new WellSchemeAC(simulator, 'A');
			break;
		case  'B':
			return new WellSchemeB(simulator, 'B');
			break;
		case  'C':
			return new WellSchemeAC(simulator, 'C');
			break;
		default:
			throw std::runtime_error("Well scheme not set");
			break;
	}
	return 0;
}


//////////////////////////////////////////////////////////


void WellSchemeAC::configure()
{
	if(scheme_identifier == 'A')  // T1 at warm well
	{
		LOG("\t\t\tconfigure scheme A");
		result.simulation_result_aiming_at_target =
			&WellDoubletCalculation::temperature_well1;
	} 
	else if(scheme_identifier == 'C')  // T1 - T2
	{
		LOG("\t\t\tconfigure scheme C");
		result.simulation_result_aiming_at_target =
			&WellDoubletCalculation::temperature_difference;
	}
	if(flag_storing)
	{
		LOG("\t\t\t\tfor storing");
		beyond = Comparison(new Greater(EPSILON));
		notReached =  Comparison(new Smaller(EPSILON));
	}
	else
	{
		LOG("\t\t\t\tfor extracting");
		beyond = Comparison(new Smaller(EPSILON));
		notReached = Comparison(new Greater(EPSILON));
	}
}

void WellSchemeAC::provide_flowrate() { result.estimate_flowrate(this); }

bool WellSchemeAC::evaluate_simulation_result()
{
	double simulation_result_aiming_at_target = (result.*(
				result.simulation_result_aiming_at_target))();
	// first adapt flow rate if temperature 1 at warm well is not
	// at target value
	if(!result.flag_flowrateAdapted)
	{
		if(beyond(simulation_result_aiming_at_target, value_target))
		{
			// if temperature T1 at warm well T1 is beyond target 
			// and flow rate Q_w already at threshold, 
			// Q_w cannot be increased and,  therefore, 
			// flow adaption stops
        		if(fabs(result.Q_w - value_threshold) < 1.e-5)
        		{  // Q_w at threshold
                		result.flag_flowrateAdapted = true;  
                		LOG("\t\t\tstop adapting flow rate");
			}
			else
			{
				result.adapt_flowrate(this);
				return true;
			}
		}
		else if(notReached(
			simulation_result_aiming_at_target, value_target))
		{
			// if T1 has not reached the target value
			// although flow Q_W is zero, 
			// flow adaption stops as well
        		if(fabs(result.Q_w) < 1.e-5)  // flow rate becomes zero 
        		{
                		result.flag_flowrateAdapted = true;  
                		LOG("\t\t\tstop adapting flow rate");
			}
			else
			{
				result.adapt_flowrate(this);
				return true;
			}
		}
		// else T1 at threshold - converged (function will return false)
	}
	
	// then adapt power rate if temperature 1 at warm well is beyond target
	// (and flow rate adaption as not succedded before)
	if(beyond(simulation_result_aiming_at_target, value_target))
	{
		result.adapt_powerrate(this);
		return true;
	}

	return false;  // means converged
}


void WellSchemeB::configure()
{
	if(flag_storing)
	{
		LOG("\t\t\tconfigure scheme B for storing");
		beyond = Comparison(new Greater(0.));
	}
	else
	{
		LOG("\t\t\tconfigure scheme B for extracting");
		beyond = Comparison(new Smaller(0.));
	}
}

void WellSchemeB::provide_flowrate() { result.set_flowrate(this); }

bool WellSchemeB::evaluate_simulation_result()
{
	if(result.flag_powerrateAdapted || beyond(result.T1, value_threshold))
	{
		if(fabs(result.T1 - value_threshold) < EPSILON)
			return false;  // stop iterating
		result.adapt_powerrate(this);
		return true;
	}
	return false;
}
