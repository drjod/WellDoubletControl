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
	result.Q_W = 0.;
	result.storage_state = on_demand;

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
	LOG("\t\t\tset temperatures\theat exchanger: " <<
			balancing_properties.T_HE << "\t\tupwind aquifer: " << balancing_properties.T_UA);
	LOG("\t\t\tset heat capacities\theat exchanger: " << balancing_properties.volumetricHeatCapacity_HE
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
			//throw std::runtime_error("WDC factory failed");
			std::cout << "WDC factory failed\n";
			abort();
	}
	return nullptr;
}


void WellScheme_0::configure_scheme()
{
	if (operationType == storing)
	{
		LOG("\t\t\tconfigure scheme 0 for storing");
		beyond.configure(new wdc::Greater(0.));
	}
	else
	{
		LOG("\t\t\tconfigure scheme 0 for extracting");
		beyond.configure(new wdc::Smaller(0.));
	}
}

void WellScheme_0::evaluate_simulation_result(const balancing_properties_t& balancing_properties)
{
	Q_H_old = get_result().Q_H;
	set_balancing_properties(balancing_properties);

	if (beyond(get_result().T_HE, value_threshold) || get_result().storage_state == powerrate_to_adapt)
		adapt_powerrate();
}


void WellScheme_0::estimate_flowrate()
{
	set_flowrate(value_target);
}


void WellScheme_0::adapt_powerrate()
{
	set_powerrate(get_result().Q_H - c_powerrate_adaption_factor * fabs(get_result().Q_W) * volumetricHeatCapacity_HE * (
		get_result().T_HE - value_threshold));
	// combine these
	if (operationType == storing && get_result().Q_H < 0.)
		set_powerrate(0.);
	else if (operationType == extracting && get_result().Q_H > 0.)
		set_powerrate(0.);
}


void WellScheme_1::configure_scheme()
{
	deltaTsign_stored = 0, flowrate_adaption_factor = c_flowrate_adaption_factor;

	LOG("\t\t\tconfigure scheme 1");
	/*if(operationType == storing)
	{
		simulation_result_aiming_at_target =   // in OGS actually the heat exchanger
			&WellScheme_1::temperature_well1;
	}
	else
	{
		simulation_result_aiming_at_target =  // in OGS actually the heat exchanger
			&WellScheme_1::temperature_well1;
	}*/


	if (operationType == storing)
	{
		LOG("\t\t\t\tfor storing");
		beyond.configure(new wdc::Greater(accuracies.temperature));
		notReached.configure(new wdc::Smaller(accuracies.temperature));
	}
	else
	{
		LOG("\t\t\t\tfor extracting");
		beyond.configure(new wdc::Smaller(accuracies.temperature));
		notReached.configure(new wdc::Greater(accuracies.temperature));
	}
}

void WellScheme_1::evaluate_simulation_result(const balancing_properties_t& balancing_properties)
{
	set_balancing_properties(balancing_properties);
	Q_H_old = get_result().Q_H;
	Q_W_old = get_result().Q_W;

	//double simulation_result_aiming_at_target = 
	//	(this->*(this->simulation_result_aiming_at_target))();
	// first adapt flow rate if temperature 1 at warm well is not
	// at target value
	//std::cout << "here " << get_result().storage_state << " \n";
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
				LOG("\t\t\tstop adapting flow rate");
				set_storage_state(powerrate_to_adapt);
				//set_powerrate(get_result().Q_H);  // to set flag
							// start adapting powerrate
			}
		}
		else if (notReached(get_result().T_HE, value_target))
		{
			if (fabs(get_result().Q_W) > accuracies.flowrate)
				adapt_flowrate();
			else
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

void WellScheme_1::estimate_flowrate()
{
	double temp, denominator;

	if (operationType == WellDoubletControl::storing)
		denominator = volumetricHeatCapacity_HE * value_target - volumetricHeatCapacity_UA * get_result().T_UA;
	else // just -
		denominator = volumetricHeatCapacity_UA * get_result().T_UA - volumetricHeatCapacity_HE * value_target;

	if (fabs(denominator) < DBL_MIN)
	{
		temp = accuracies.flowrate;
	}
	else
	{
		temp = get_result().Q_H / denominator;
	}

	double operability = 1;
	if (operationType == WellDoubletControl::storing)
		operability = wdc::make_threshold_factor(get_result().T_UA, value_target,
			well_shutdown_temperature_range, wdc::upper);
	else
		operability = wdc::make_threshold_factor(get_result().T_UA, value_target,
			well_shutdown_temperature_range, wdc::lower);

	//LOG("\t\twstr: " << well_shutdown_temperature_range);
	LOG("\t\tOperability: " << operability);
	//double well2_impact_factor = (operationType == storing) ?
	//	wdc::threshold(get_result().T_UA, value_target,
	//	std::fabs(value_target)*THRESHOLD_DELTA_FACTOR_WELL2,
	//	wdc::upper) : 1.;	// temperature at cold well 2 
				// should not reach threshold of warm well 1
	if (operability < 1)
	{
		temp *= operability;
		set_powerrate(get_result().Q_H * operability);
		//LOG("\t\t\tAdjust wells - set power rate\t" << get_result().Q_H);
	}


	set_flowrate((operationType == WellDoubletControl::storing) ?
		wdc::make_confined(temp, accuracies.flowrate, value_threshold) :
		wdc::make_confined(temp, value_threshold, accuracies.flowrate));

	//if(std::isnan(get_result().Q_W))  // no check for -nan and inf
	//	throw std::runtime_error("WellDoubletControl: nan when setting Q_W");	

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

	double operability = 1;  // [0, 1]

	if (operationType == WellDoubletControl::storing)
		operability = wdc::make_threshold_factor(get_result().T_UA, value_target,
			well_shutdown_temperature_range, wdc::upper);
	else
		operability = wdc::make_threshold_factor(get_result().T_UA, value_target,
			well_shutdown_temperature_range, wdc::lower);
	// temperature at cold well 2 
	// should not reach threshold of warm well 1
	LOG("\t\tOperability: " << operability);

	if (operability < 1)
	{	// storing: temperature at cold well 2 is close to maximum of well 1
		// extracting: temperature at warm well 1 is close to minumum
		set_powerrate(get_result().Q_H * operability);
		//LOG("\t\t\tAdjust wells - set power rate\t" << get_result().Q_H);
	}

	set_flowrate((operationType == WellDoubletControl::storing) ?
		wdc::make_confined(operability * get_result().Q_W *
		(1 + flowrate_adaption_factor * deltaT),
			accuracies.flowrate, value_threshold) :
		wdc::make_confined(operability * get_result().Q_W *
		(1 - flowrate_adaption_factor * deltaT),
			value_threshold, -accuracies.flowrate));

	//if(std::isnan(get_result().Q_W))
	//	throw std::runtime_error(
	//		"WellDoubletControl: nan when adapting Q_W");	
}

void WellScheme_1::adapt_powerrate()
{
	double powerrate = get_result().Q_H - c_powerrate_adaption_factor * fabs(get_result().Q_W) * volumetricHeatCapacity_HE * (
		get_result().T_HE -
		// Scheme A: T_HE, Scheme C: T_HE - T_UA
// should take actually also volumetricHeatCapacity_UA 
value_target);

	if (operationType == storing && powerrate < 0.)
	{
		set_powerrate(0.);
		set_flowrate(0.);
		LOG("\t\t\tswitch off well");
	}
	else if (operationType == extracting && powerrate > 0.)
	{
		set_powerrate(0.);
		set_flowrate(0.);
		LOG("\t\t\tswitch off well");
	}
	else
		set_powerrate(powerrate);
	//if(std::isnan(get_result().Q_H))
	//	throw std::runtime_error("WellDoubletControl: nan when adapting Q_H");	
}


void WellScheme_2::configure_scheme()
{
	deltaTsign_stored = 0, flowrate_adaption_factor = c_flowrate_adaption_factor;

	LOG("\t\t\tconfigure scheme 1");

	if (operationType == storing)
	{
		LOG("\t\t\t\tfor storing");
		beyond.configure(new wdc::Greater(accuracies.temperature));
		notReached.configure(new wdc::Smaller(accuracies.temperature));
	}
	else
	{
		LOG("\t\t\t\tfor extracting");
		beyond.configure(new wdc::Smaller(accuracies.temperature));
		notReached.configure(new wdc::Greater(accuracies.temperature));
	}
}

void WellScheme_2::evaluate_simulation_result(const balancing_properties_t& balancing_properties)
{
	set_balancing_properties(balancing_properties);
	Q_H_old = get_result().Q_H;
	Q_W_old = get_result().Q_W;

	double value = volumetricHeatCapacity_HE * get_result().T_HE - volumetricHeatCapacity_UA * get_result().T_UA;
	//if(operationType == extracting)
	//	value = -value;

	std::cout << "value: " << value << "\n";
	std::cout << "value_target: " << value_target << "\n";
	//double simulation_result_aiming_at_target = 
	//	(this->*(this->simulation_result_aiming_at_target))();
	// first adapt flow rate if temperature 1 at warm well is not
	// at target value
	//std::cout << "here " << get_result().storage_state << " \n";
	if (get_result().storage_state == on_demand)
	{	// do not put this after adapt_powerrate below since
		// powerrate must be adapted in this iteration if flow rate adaption fails
		// otherwise error calucaltion in iteration loop results in zero
		if (beyond(value, value_target))
		{
			std::cout << "value_threshold: " << value_threshold << "\n";
			//std::cout << "Q_w: " << get_result().Q_W << "\n";
			if (fabs(get_result().Q_W - value_threshold) > accuracies.flowrate)
				adapt_flowrate();
			else
			{  // cannot store / extract the heat
				LOG("\t\t\tstop adapting flow rate");
				set_storage_state(powerrate_to_adapt);
				//set_powerrate(get_result().Q_H);  // to set flag
							// start adapting powerrate
			}
		}
		else if (notReached(value, value_target))
		{
			if (fabs(get_result().Q_W) > accuracies.flowrate)
				adapt_flowrate();
			else
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
	double temp, denominator, flowrate;

	if (operationType == WellDoubletControl::storing)
		denominator = volumetricHeatCapacity_HE * get_result().T_HE - volumetricHeatCapacity_UA * get_result().T_UA;
	else // just -
		denominator = -(volumetricHeatCapacity_HE * get_result().T_HE - volumetricHeatCapacity_UA * get_result().T_UA);
	LOG("\t\tdenom: " << denominator);

	if (fabs(denominator) < DBL_MIN)
	{
		temp = accuracies.flowrate;
	}
	else
	{
		temp = get_result().Q_H / denominator;
	}

	//temp = value_threshold / 2;

	flowrate = (operationType == WellDoubletControl::storing) ?
		wdc::make_confined(temp, accuracies.flowrate, value_threshold) :
		wdc::make_confined(temp, value_threshold, -accuracies.flowrate);

	LOG("\t\testimated flow rate: " << flowrate);

	set_flowrate(flowrate);

	//if(std::isnan(get_result().Q_W))  // no check for -nan and inf
	//	throw std::runtime_error("WellDoubletControl: nan when setting Q_W");	

}

void WellScheme_2::adapt_flowrate()
{
	double Q_T;// = volumetricHeatCapacity_HE * get_result().T_HE  - volumetricHeatCapacity_UA * get_result().T_UA;
	double deltaQ_w;// = (Q_T - value_target) / value_target;

	if (operationType == WellDoubletControl::storing)
	{
		Q_T = volumetricHeatCapacity_HE * get_result().T_HE - volumetricHeatCapacity_UA * get_result().T_UA;
		deltaQ_w = (Q_T - value_target) / value_target;
	}
	else  // -
	{
		Q_T = volumetricHeatCapacity_UA * get_result().T_UA - volumetricHeatCapacity_HE * get_result().T_HE;
		deltaQ_w = (value_target + Q_T) / value_target;
	}
	std::cout << "deltaQ_w: " << deltaQ_w << "\n";

	set_flowrate((operationType == WellDoubletControl::storing) ?
		wdc::make_confined(get_result().Q_W * (1 + deltaQ_w),
			accuracies.flowrate, value_threshold) :
		wdc::make_confined(get_result().Q_W * (1 - deltaQ_w), value_threshold, -accuracies.flowrate));

}

void WellScheme_2::adapt_powerrate()
{

	double spread, powerrate;

	if (operationType == WellDoubletControl::storing)
	{
		spread = volumetricHeatCapacity_HE * get_result().T_HE - volumetricHeatCapacity_UA * get_result().T_UA;
		if (fabs(spread) < 1.e-10) spread = 1.e-10;

		powerrate = get_result().Q_H * (1 - c_powerrate_adaption_factor *
			(spread - value_target) / spread);
	}
	else
	{
		spread = volumetricHeatCapacity_UA * get_result().T_UA - volumetricHeatCapacity_HE * get_result().T_HE;
		if (fabs(spread) < 1.e-10) spread = 1.e-10;

		powerrate = get_result().Q_H * (1 - c_powerrate_adaption_factor *
			(value_target + spread) / spread);
	}
	std::cout << "spread: " << spread << '\n';

	if (operationType == storing && powerrate < 0.)
	{
		set_powerrate(0.);
		set_flowrate(0.);
		LOG("\t\t\tswitch off well");
	}
	else if (operationType == extracting && powerrate > 0.)
	{
		set_powerrate(0.);
		set_flowrate(0.);
		LOG("\t\t\tswitch off well");
	}
	else
		set_powerrate(powerrate);
	//if(std::isnan(get_result().Q_H))
	//	throw std::runtime_error("WellDoubletControl: nan when adapting Q_H");	

}



} // end namespace wdc
