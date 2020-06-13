#include <stdexcept>
#include <cstdlib>
#include <iostream>
#include <fstream>
#include <iomanip>
#include <utility>
#include <cfloat>  // for DBL_MIN
#include <algorithm>
#include "wellDoubletControl.h"

namespace wdc
{
void WellDoubletControl::set_heatPump(const int& _type, const double& T_sink, const double& eta)
{
	if(_type == 1)
	{
		delete heatPump;
		heatPump = new CarnotHeatPump(T_sink, eta);
	}
}


void WellDoubletControl::print_temperatures() const
{
	std::cout << "\t\t\tT_HE: " << result.T_HE << " - T_UA: " <<
		result.T_UA << std::endl;
}

void WellDoubletControl::configure(
	const double& _Q_H_sys,
	const double& _value_target, const double& _value_threshold,
	const balancing_properties_t& balancing_properties)
{
	set_balancing_properties(balancing_properties);

	Q_H_sys_target = _Q_H_sys;  // just for output;
	result.Q_H_sys = _Q_H_sys;
	if(_Q_H_sys > 0.)
	{
		WDC_LOG("\t\t\tset system power rate\t\t" <<_Q_H_sys << " - storing");
		operationType = storing;
		result.Q_H = _Q_H_sys;
		result.Q_W = accuracies.flowrate;
	}
	else
	{
		WDC_LOG("\t\t\tset system power rate\t\t" << _Q_H_sys << " - extracting");
		operationType = extracting;
        	result.Q_H = heatPump->calculate_heat_source(_Q_H_sys, result.T_UA, result.T_HE);
		result.Q_W = -accuracies.flowrate;
	}

	// set input values for well doublet control, e.g. from file
	result.storage_state = on_demand;

	value_target = _value_target;
	value_threshold = _value_threshold;

	// the scheme-dependent stuff
	configure_scheme();  // iterationState & comparison functions 
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
	WDC_LOG("\t\t\tset temperatures\theat exchanger: " <<
			balancing_properties.T_HE << "\t\tupwind aquifer: " << balancing_properties.T_UA);
	WDC_LOG("\t\t\tset heat capacities\theat exchanger: " << balancing_properties.volumetricHeatCapacity_HE
					<< "\tupwind aquifer: " << balancing_properties.volumetricHeatCapacity_UA);
}

WellDoubletControl* WellDoubletControl::create_wellDoubletControl(
				const int& selection, const double& _well_shutdown_temperature_range, const accuracies_t& _accuracies)
{
	switch(selection)
	{
		case  0:
			return new WellScheme_0(_well_shutdown_temperature_range, _accuracies);
		case  1:
			return new WellScheme_1(_well_shutdown_temperature_range, _accuracies);
		case  2:
			return new WellScheme_2(_well_shutdown_temperature_range, _accuracies);
		default:
			WDC_LOG("WDC factory failed");
			abort();
	}
	return nullptr;
}


void WellScheme_0::configure_scheme()
{
	if (operationType == storing)
	{
		WDC_LOG("\t\t\tconfigure scheme 0 for storing");
		beyond.configure(new wdc::Greater(0.));
	}
	else
	{
		WDC_LOG("\t\t\tconfigure scheme 0 for extracting");
		beyond.configure(new wdc::Smaller(0.));
	}
}

void WellScheme_0::evaluate_simulation_result(const balancing_properties_t& balancing_properties)
{
	set_balancing_properties(balancing_properties);
	Q_H_sys_old = get_result().Q_H_sys;

	const double Q_H = (get_result().Q_H_sys > 0.) ? get_result().Q_H_sys : 
		heatPump->calculate_heat_source(get_result().Q_H_sys, balancing_properties.T_UA, balancing_properties.T_HE);  // !!! call to update COP

	if ((beyond(get_result().T_HE, value_threshold) || get_result().storage_state == powerrate_to_adapt) && get_result().storage_state != target_not_achievable )
	{
		set_storage_state(powerrate_to_adapt);
		//set_powerrate(Q_H);
		adapt_powerrate();
	}
}


void WellScheme_0::estimate_flowrate()
{
	double flowrate = value_target;

	const double operability = (operationType == WellDoubletControl::storing) ?
		wdc::make_threshold_factor(get_result().T_UA, value_threshold,
			well_shutdown_temperature_range, wdc::upper) :
		wdc::make_threshold_factor(get_result().T_UA, value_threshold,
			well_shutdown_temperature_range, wdc::lower);

	if (operability < 1)
	{
		WDC_LOG("\t\toperability: " << operability);
		flowrate *= operability;
		set_powerrate(get_result().Q_H * operability);
		set_storage_state(rates_reduced);
	}

	set_flowrate((operationType == WellDoubletControl::storing) ?
		wdc::make_confined(flowrate, accuracies.flowrate, value_target) :
		wdc::make_confined(flowrate, value_target, -accuracies.flowrate));
}


void WellScheme_0::adapt_powerrate()
{
	const double operability = (operationType == WellDoubletControl::storing) ?  // [0, 1]
		wdc::make_threshold_factor(get_result().T_UA, value_threshold,  // storing
			well_shutdown_temperature_range, wdc::upper) :
		wdc::make_threshold_factor(get_result().T_UA, value_threshold,  // retrieving
			well_shutdown_temperature_range, wdc::lower);

	set_powerrate(operability * (get_result().Q_H - c_powerrate_adaption_factor * fabs(get_result().Q_W) * volumetricHeatCapacity_HE * (
		get_result().T_HE - value_threshold)));

	if(operability < 1.)
	{
		WDC_LOG("\t\toperability: " << operability);
		set_flowrate(operability * get_result().Q_W);
		set_storage_state(rates_reduced);
	}

	if (operationType == storing && 
			get_result().Q_H < accuracies.powerrate && 
			get_result().storage_state != rates_reduced)
	{
		set_powerrate(0.);
		set_flowrate(accuracies.flowrate);
		set_storage_state(target_not_achievable);
	}
	else if (operationType == extracting && 
			get_result().Q_H > -accuracies.powerrate && 
			get_result().storage_state != rates_reduced)
	{
		set_powerrate(0.);
		set_flowrate(-accuracies.flowrate);
		set_storage_state(target_not_achievable);
	}
}


void WellScheme_1::configure_scheme()
{
	deltaTsign_stored = 0, flowrate_adaption_factor = c_flowrate_adaption_factor;

	WDC_LOG("\t\t\tconfigure scheme 1");

	if (operationType == storing)
	{
		WDC_LOG("\t\t\t\tfor storing");
		beyond.configure(new wdc::Greater(accuracies.temperature));
		notReached.configure(new wdc::Smaller(accuracies.temperature));
	}
	else
	{
		WDC_LOG("\t\t\t\tfor extracting");
		beyond.configure(new wdc::Smaller(accuracies.temperature));
		notReached.configure(new wdc::Greater(accuracies.temperature));
	}
}

void WellScheme_1::evaluate_simulation_result(const balancing_properties_t& balancing_properties)
{
	set_balancing_properties(balancing_properties);
	Q_H_sys_old = get_result().Q_H_sys;
	Q_W_old = get_result().Q_W;

	const double Q_H = (get_result().Q_H_sys > 0.)? get_result().Q_H_sys : 
		heatPump->calculate_heat_source(get_result().Q_H_sys, balancing_properties.T_UA, balancing_properties.T_HE);  // !!! call to update COP

	// double simulation_result_aiming_at_target = 
	//	(this->*(this->simulation_result_aiming_at_target))();
	// first adapt flow rate if temperature 1 at warm well is not
	// at target value
	// std::cout << "here " << get_result().storage_state << " \n";
	if (get_result().storage_state == on_demand)
	{	// do not put this after adapt_powerrate below since
		// powerrate must be adapted in this iteration if flow rate adaption fails
		// otherwise error calucaltion in iteration loop results in zero
		if (beyond(get_result().T_HE, value_target))
		{
			if (fabs(get_result().Q_W - value_threshold) > accuracies.flowrate)
				adapt_flowrate();
			else
			{  // cannot store / extract the heat
				WDC_LOG("\t\t\tstop adapting flow rate");
				set_storage_state(powerrate_to_adapt);
				//set_powerrate(get_result().Q_H);  // to set flag
							// start adapting powerrate
			}
		}
		else if (notReached(get_result().T_HE, value_target))
		{
			if (fabs(get_result().Q_W) > accuracies.flowrate)
				adapt_flowrate();
			else if(get_result().storage_state != rates_reduced)
			{
				set_storage_state(target_not_achievable);
				// cannot adapt flowrate further (and powerrate is too low)
			}
		}
	}

	if (get_result().storage_state == powerrate_to_adapt || get_result().storage_state == rates_reduced)
		adapt_powerrate(); // start and continue adapting
				// iteration is checked by simulator
}

void WellScheme_1::estimate_flowrate()
{
	const double denominator = (operationType == WellDoubletControl::storing) ?
		volumetricHeatCapacity_HE * value_target - volumetricHeatCapacity_UA * get_result().T_UA : 
		volumetricHeatCapacity_UA * get_result().T_UA - volumetricHeatCapacity_HE * value_target;

	double flowrate;
     	if (operationType == WellDoubletControl::storing)
		flowrate = (fabs(denominator) < DBL_MIN) ? accuracies.flowrate : get_result().Q_H / denominator;
	else
		flowrate = (fabs(denominator) < DBL_MIN) ? -accuracies.flowrate : get_result().Q_H / denominator;

	const double operability = (operationType == WellDoubletControl::storing) ?
		wdc::make_threshold_factor(get_result().T_UA, value_target,
			well_shutdown_temperature_range, wdc::upper) :
		wdc::make_threshold_factor(get_result().T_UA, value_target,
			well_shutdown_temperature_range, wdc::lower);

	if (operability < 1.)
	{
		WDC_LOG("\t\toperability: " << operability);
		flowrate *= operability;
		set_powerrate(get_result().Q_H * operability);
		set_storage_state(rates_reduced);
	}

	set_flowrate((operationType == WellDoubletControl::storing) ?
		wdc::make_confined(flowrate, accuracies.flowrate, value_threshold) :
		wdc::make_confined(flowrate, value_threshold, -accuracies.flowrate));
}

void WellScheme_1::adapt_flowrate()
{
	double deltaT = get_result().T_HE - value_target;

	if (operationType == WellDoubletControl::storing)
		deltaT /= std::max(get_result().T_HE - get_result().T_UA, 1.);
	else
		deltaT /= std::max(get_result().T_UA - get_result().T_HE, 1.);

	// decreases flowrate_adaption_factor to avoid that T_HE jumps 
	// around threshold (deltaT flips sign)
	if (deltaTsign_stored != 0  // == 0: take initial value for factor
		&& deltaTsign_stored != wdc::sign(deltaT))
		flowrate_adaption_factor = (c_flowrate_adaption_factor == 1) ?
		flowrate_adaption_factor * 0.9 :  // ?????
		flowrate_adaption_factor * c_flowrate_adaption_factor;

	deltaTsign_stored = wdc::sign(deltaT);


	const double operability = (operationType == WellDoubletControl::storing) ?  // [0, 1]
		wdc::make_threshold_factor(get_result().T_UA, value_target,  // storing
			well_shutdown_temperature_range, wdc::upper) :
		wdc::make_threshold_factor(get_result().T_UA, value_target,  // retrieving
			well_shutdown_temperature_range, wdc::lower);
	// temperature at cold well 2 
	// should not reach threshold of warm well 1

	if (operability < 1.)
	{	
		WDC_LOG("\t\toperability: " << operability);
		set_powerrate(get_result().Q_H * operability);
		set_storage_state(rates_reduced);
	}

	set_flowrate((operationType == WellDoubletControl::storing) ?
		wdc::make_confined(operability * get_result().Q_W *
		(1 + flowrate_adaption_factor * deltaT),
			accuracies.flowrate, value_threshold) :
		wdc::make_confined(operability * get_result().Q_W *
		(1 - flowrate_adaption_factor * deltaT),
			value_threshold, -accuracies.flowrate));
}

void WellScheme_1::adapt_powerrate()
{
	double powerrate = get_result().Q_H - c_powerrate_adaption_factor * fabs(get_result().Q_W) * volumetricHeatCapacity_HE * (
		get_result().T_HE -
		// Scheme A: T_HE, Scheme C: T_HE - T_UA
// should take actually also volumetricHeatCapacity_UA 
value_target);

	const double operability = (operationType == WellDoubletControl::storing) ?  // [0, 1]
		wdc::make_threshold_factor(get_result().T_UA, value_target,  // storing
			well_shutdown_temperature_range, wdc::upper) :
		wdc::make_threshold_factor(get_result().T_UA, value_target,  // retrieving
			well_shutdown_temperature_range, wdc::lower);
	// temperature at cold well 2 
	// should not reach threshold of warm well 1

	if (operability < 1.)
	{	
		WDC_LOG("\t\toperability: " << operability);
		powerrate *= operability;
	}

	set_powerrate(powerrate);
	
	if (operationType == storing && powerrate < accuracies.powerrate) 
	{
		set_powerrate(0.);
		set_flowrate(accuracies.flowrate);
		WDC_LOG("\t\t\tswitch off well");
	}
	else if	(operationType == extracting && powerrate > -accuracies.powerrate)
	{
		set_powerrate(0.);
		set_flowrate(-accuracies.flowrate);
		WDC_LOG("\t\t\tswitch off well");
	}
}


void WellScheme_2::configure_scheme()
{
	deltaTsign_stored = 0, flowrate_adaption_factor = c_flowrate_adaption_factor;

	WDC_LOG("\t\t\tconfigure scheme 1");

	if (operationType == storing)
	{
		WDC_LOG("\t\t\t\tfor storing");
		beyond.configure(new wdc::Greater(accuracies.temperature));
		notReached.configure(new wdc::Smaller(accuracies.temperature));
	}
	else
	{
		WDC_LOG("\t\t\t\tfor extracting");
		beyond.configure(new wdc::Smaller(accuracies.temperature));
		notReached.configure(new wdc::Greater(accuracies.temperature));
	}
}

void WellScheme_2::evaluate_simulation_result(const balancing_properties_t& balancing_properties)
{
	set_balancing_properties(balancing_properties);
	Q_H_sys_old = get_result().Q_H_sys;
	Q_W_old = get_result().Q_W;

	const double spread = get_result().T_HE - get_result().T_UA;

	if (get_result().storage_state == on_demand)
	{	// do not put this after adapt_powerrate below since
		// powerrate must be adapted in this iteration if flow rate adaption fails
		// otherwise error calucaltion in iteration loop results in zero
		if (beyond(spread, value_target))
		{
			if (fabs(get_result().Q_W - value_threshold) > accuracies.flowrate)
				adapt_flowrate();
			else
			{  // cannot store / extract the heat
				WDC_LOG("\t\t\tstop adapting flow rate");
				set_storage_state(powerrate_to_adapt);
							// start adapting powerrate
			}
		}
		else if (notReached(spread, value_target))
		{
			if (fabs(get_result().Q_W) > accuracies.flowrate)
				adapt_flowrate();
			else if (get_result().storage_state != rates_reduced)
			{
				set_storage_state(target_not_achievable);
				// cannot adapt flowrate further (and powerrate is too low)
			}
		}
	}

	if (get_result().storage_state == powerrate_to_adapt)
		adapt_powerrate(); // continue adapting
				// iteration is checked by simulator
}

void WellScheme_2::estimate_flowrate()
{
	const double denominator = (operationType == WellDoubletControl::storing) ?
		volumetricHeatCapacity_HE * get_result().T_HE - volumetricHeatCapacity_UA * get_result().T_UA :
		volumetricHeatCapacity_UA * get_result().T_UA - volumetricHeatCapacity_HE * get_result().T_HE;

	double flowrate;
     	if (operationType == WellDoubletControl::storing)
		flowrate = (fabs(denominator) < DBL_MIN) ? accuracies.flowrate : get_result().Q_H / denominator;
	else
		flowrate = (fabs(denominator) < DBL_MIN) ? -accuracies.flowrate : get_result().Q_H / denominator;

	set_flowrate((operationType == WellDoubletControl::storing) ?
		wdc::make_confined(flowrate, accuracies.flowrate, value_threshold) :
		wdc::make_confined(flowrate, value_threshold, -accuracies.flowrate));
}

void WellScheme_2::adapt_flowrate()
{
	const double spread = get_result().T_HE  - get_result().T_UA;

	const double deltaQ_w = (operationType == WellDoubletControl::storing) ?
		(spread - value_target) / value_target : 
		(value_target - spread) / value_target;

	set_flowrate((operationType == WellDoubletControl::storing) ?
		wdc::make_confined(get_result().Q_W * (1 + deltaQ_w),
			accuracies.flowrate, value_threshold) :
		wdc::make_confined(get_result().Q_W * (1 - deltaQ_w), value_threshold, -accuracies.flowrate));
}

void WellScheme_2::adapt_powerrate()
{
	double spread = volumetricHeatCapacity_HE * get_result().T_HE - volumetricHeatCapacity_UA * get_result().T_UA;
	if (fabs(spread) < DBL_MIN) 
		spread =  (operationType == WellDoubletControl::storing) ? 1.e-10 : -1.e-10;

	const double powerrate = get_result().Q_H  -  fabs(get_result().Q_W) * c_powerrate_adaption_factor * 
				(spread - value_target * volumetricHeatCapacity_HE);

	set_powerrate(powerrate);

	if (operationType == storing && powerrate < accuracies.powerrate) 
	{
		set_powerrate(0.);
		set_flowrate(accuracies.flowrate);
		WDC_LOG("\t\t\tswitch off well");
	}
	else if	(operationType == extracting && powerrate > -accuracies.powerrate)
	{
		set_powerrate(0.);
		set_flowrate(-accuracies.flowrate);
		WDC_LOG("\t\t\tswitch off well");
	}
}



} // end namespace wdc
