#include "wellDoubletControl.h"
#include <stdexcept>

void WellDoubletControl::print_temperatures()
{
	std::cout << "\t\t\tT1: " << result.T1 << " - T2: " <<
		result.T2 << std::endl;
}


void WellDoubletControl::set_constraints(const double& _Q_H,
	const double& _value_target, const double& _value_threshold) 
{ 
	result.Q_H = _Q_H;  // stored (Q_H>0) or extracted (Q_H<0) heat
	value_target = _value_target;
	value_threshold = _value_threshold;

	result.flag_powerrateAdapted = false;

	if(_Q_H > 0.)
		flag_storing = true;
	else  // extracting
		flag_storing = false;

	initialize();  // the scheme-dependent stuff (comparison functions)
}

void WellDoubletControl::set_temperatures(const double& _T1, const double& _T2)  
{
	result.T1 = _T1;  // warm well
	result.T2 = _T2;  // cold well
}



WellDoubletControl* WellDoubletControl::createWellDoubletControl(
				const char& selection, FakeSimulator* simulator)
{
	switch(selection)
	{
		case  'A':
			return new WellSchemeA(simulator);
			break;
		case  'B':
			return new WellSchemeB(simulator);
			break;
		case  'C':
			return new WellSchemeC(simulator);
			break;
		default:
			throw std::runtime_error("Well scheme not set");
			break;
	}
	return 0;
}


//////////////////////////////////////////////////////////


void WellSchemeA::initialize()
{
	//calculation.T1 = value_target;

	if(flag_storing)
	{
		beyondThreshold = Comparison(new Greater(EPSILON));
		check_for_flowrateAdaption =  Comparison(new Smaller(EPSILON));
	}
	else
	{
		beyondThreshold = Comparison(new Smaller(EPSILON));
		check_for_flowrateAdaption = Comparison(new Greater(EPSILON));
	}
}

void WellSchemeA::calculate_flowrate() { result.calculate_flowrate(this); }

bool WellSchemeA::check_result()
{
	LOG("\\\check result");
	if(beyondThreshold(result.T1, value_target))
	{
		result.adapt_powerrate(this);
		return true;
	}
	else
	{
		if(check_for_flowrateAdaption(result.T1, value_target))
		{
			result.calculate_flowrate(this);
			return true;
		}
	}
	return false;
}


void WellSchemeB::initialize()
{
	if(flag_storing)
		beyondThreshold = Comparison(new Greater(0.));
	else
		beyondThreshold = Comparison(new Smaller(0.));
}

void WellSchemeB::calculate_flowrate() { result.calculate_flowrate(this); }

bool WellSchemeB::check_result()
{
	if(result.flag_powerrateAdapted || beyondThreshold(result.T1, value_threshold))
	{
		if(fabs(result.T1 - value_threshold) < EPSILON)
			return false;  // stop iterating
		result.adapt_powerrate(this);
		return true;
	}
	return false;
}


void WellSchemeC::initialize()
{
	if(flag_storing)
	{
		beyondThreshold = Comparison(new Greater(0.));
	}
	else
	{
		beyondThreshold = Comparison(new Smaller(0.));
	}
}

void WellSchemeC::calculate_flowrate() { result.calculate_flowrate(this); }

bool WellSchemeC::check_result()
{
	if(beyondThreshold(result.T1, value_threshold))
	{
		result.adapt_powerrate(this);
		return true;
	}
	return false;
}

////////////////////////////////////////////


void WellDoubletCalculation::adapt_powerrate(WellSchemeA* scheme)
{
	Q_H = Q_w * scheme->simulator->get_heatcapacity() * (
				2 * scheme->value_target - T1 - T2);
	LOG("\t\t\tadapt power rate to " + std::to_string(Q_H));
	flag_powerrateAdapted = true;

}

void WellDoubletCalculation::adapt_powerrate(WellSchemeB* scheme)
{
	Q_H -= Q_w * scheme->simulator->get_heatcapacity() * (
					T1 - scheme->value_threshold);
	LOG("\t\t\tadapt power rate to " + std::to_string(Q_H));
	flag_powerrateAdapted = true;
}

void WellDoubletCalculation::adapt_powerrate(WellSchemeC* scheme)
{
	Q_H = Q_w * scheme->simulator->get_heatcapacity() * (
					scheme->value_threshold - T2);
	LOG("\t\t\tadapt power rate to " + std::to_string(Q_H));
	flag_powerrateAdapted = true;

}


void WellDoubletCalculation::calculate_flowrate(WellSchemeA* scheme)
{
	if(scheme->flag_storing)
		Q_w = fmin(Q_H / (scheme->simulator->get_heatcapacity()  *
			(scheme->value_target - T2)), scheme->value_threshold);
	else
		Q_w = fmax(Q_H / (scheme->simulator->get_heatcapacity()  *
			(scheme->value_target - T2)), scheme->value_threshold);
}

void WellDoubletCalculation::calculate_flowrate(WellSchemeB* scheme)
{
	Q_w = scheme->value_target;
}

void WellDoubletCalculation::calculate_flowrate(WellSchemeC* scheme)
{
	// no range check
	if(scheme->flag_storing)
		Q_w = Q_H / (scheme->simulator->get_heatcapacity() *
							scheme->value_target);
	else
		Q_w = Q_H / (scheme->simulator->get_heatcapacity() *
							scheme->value_target);
}

