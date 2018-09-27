#include "wellDoubletControl.h"
#include <stdexcept>
#include <iostream>
#include <fstream>
#include <iomanip>
#include <utility>
#include <cfloat>  // for DBL_MIN

void WellDoubletControl::print_temperatures() const
{
	std::cout << "\t\t\tT_HE: " << result.T_HE << " - T_UA: " <<
		result.T_UA << std::endl;
}

void WellDoubletControl::configure(
	const double& _Q_H,
	const double& _value_target, const double& _value_threshold,
	const balancing_properties_t& balancing_properties)
{
	set_balancing_properties(balancing_properties);
	// set input values for well doublet control, e.g. from file
	result.Q_H = _Q_H;  // stored (Q_H>0) or extracted (Q_H<0) heat
	result.flag_powerrateAdapted = false;

	value_target = _value_target;
	value_threshold = _value_threshold;
	
	if(_Q_H > 0.)
	{
		LOG("\t\t\tset power rate\t\t" <<_Q_H << " - storing");
		operationType = storing;
	}
	else
	{
		LOG("\t\t\tset power rate\t\t" << _Q_H << " - extracting");
		operationType = extracting;
	}

	// the scheme-dependent stuff
	configureScheme();  // iterationState & comparison functions 
			//for temperature target (A, C), temperature constraint (B)
	estimate_flowrate();  // an estimation for scheme A and a target for scheme B
}

void WellDoubletControl::set_balancing_properties(const balancing_properties_t& balancing_properties)
{
	//if(std::isnan(_T_HE) || std::isnan(_T_UA))
	//	throw std::runtime_error(
	//		"WellDoubletControl: Simulator gave nan in temperatures");
	
	result.T_HE = balancing_properties.T_HE;  // warm well
	result.T_UA = balancing_properties.T_UA;  // cold well
	volumetricHeatCapacity_HE = balancing_properties.volumetricHeatCapacity_HE;  // warm well
	volumetricHeatCapacity_UA = balancing_properties.volumetricHeatCapacity_UA;  // cold well
	LOG("\t\t\tset temperatures\theat exchanger: " <<
			balancing_properties.T_HE << "\t\tupwind aquifer: " << balancing_properties.T_UA);
	LOG("\t\t\tset heat capacities\theat exchanger: " << balancing_properties.volumetricHeatCapacity_HE
					<< "\tupwind aquifer: " << balancing_properties.volumetricHeatCapacity_UA);
}

WellDoubletControl* WellDoubletControl::create_wellDoubletControl(
				const int& selection)
{
	switch(selection)
	{
		case  0:
			return new WellScheme_0();
		case  1:
			return new WellScheme_1();
		case  2:
			return new WellScheme_2();
		default:
			throw std::runtime_error("WDC factory failed");
	}
	return nullptr;
}

#include "wellScheme_0.cpp"
#include "wellScheme_1.cpp"
#include "wellScheme_2.cpp"
