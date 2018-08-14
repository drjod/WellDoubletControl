#include "wellDoubletControl.h"
#include <stdexcept>
#include <iostream>
#include <fstream>
#include <iomanip>
#include <utility>
#include "wdc_config.h"
#include <math.h>  // for isnan()
#include <cfloat>  // for DBL_MIN

void WellDoubletControl::print_temperatures() const
{
	std::cout << "\t\t\tT1: " << result.T1 << " - T2: " <<
		result.T2 << std::endl;
}

void WellDoubletControl::configure(
	const double& _Q_H,
	const double& _value_target, const double& _value_threshold,
	const double& _T1, const double& _T2, 
	const double& _heatCapacity1, const double& _heatCapacity2) 
{
	set_heatFluxes(_T1, _T2, _heatCapacity1, _heatCapacity2);
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
	set_flowrate();  // an estimation for scheme A and a target for scheme B
}

void WellDoubletControl::set_heatFluxes(const double& _T1, const double& _T2, 
			const double& _heatCapacity1, const double& _heatCapacity2) 
{
	//if(std::isnan(_T1) || std::isnan(_T2))
	//	throw std::runtime_error(
	//		"WellDoubletControl: Simulator gave nan in temperatures");
	
	result.T1 = _T1;  // warm well
	result.T2 = _T2;  // cold well
	heatCapacity1 = _heatCapacity1;  // warm well
	heatCapacity2 = _heatCapacity2;  // cold well
	LOG("\t\t\tset temperatures\twarm: " << _T1 << "\t\tcold: " << _T2);
	LOG("\t\t\tset heat capacities\twarm: " << _heatCapacity1
					<< "\tcold: " << _heatCapacity2);
}

WellDoubletControl* WellDoubletControl::create_wellDoubletControl(
				const int& selection)
{
	switch(selection)
	{
		case  0:
			return new WellScheme_0(0);
			break;
		case  1:
			return new WellScheme_12(1);
			break;
		case  2:
			return new WellScheme_12(2);
			break;
		default:
			;//throw std::runtime_error("Well scheme not set");
	}
	return 0;
}


//////////////////////////////////////////////////////////


void WellScheme_0::configureScheme()
{
	if(operationType == storing)
	{
		LOG("\t\t\tconfigure scheme 0 for storing");
		beyond = wdc::Comparison(new wdc::Greater(0.));
	}
	else
	{
		LOG("\t\t\tconfigure scheme 0 for extracting");
		beyond = wdc::Comparison(new wdc::Smaller(0.));
	}
}

void WellScheme_0::evaluate_simulation_result(
		const double& _T1, const double& _T2,
		const double& _heatCapacity1, const double& _heatCapacity2)
{
	set_heatFluxes(_T1, _T2, _heatCapacity1, _heatCapacity2);

	if(beyond(result.T1, value_threshold) || result.flag_powerrateAdapted)
		adapt_powerrate();
}


void WellScheme_0::set_flowrate()
{
        result.Q_w = value_target;
        LOG("\t\t\tset flow rate\t" << result.Q_w);
}


void WellScheme_0::adapt_powerrate()
{
        result.Q_H -= fabs(result.Q_w) * heatCapacity1 * (
                                        result.T1 - value_threshold);

	if(operationType == storing && result.Q_H < 0.)
		result.Q_H = 0.;
	else if(operationType == extracting && result.Q_H > 0.)
		result.Q_H = 0.;

        LOG("\t\t\tadapt power rate\t" << result.Q_H);
        result.flag_powerrateAdapted = true;
}

////////////////////////////


void WellScheme_12::configureScheme()
{
	deltaTsign_stored = 0, flowrate_adaption_factor = FLOWRATE_ADAPTION_FACTOR;  
	flag_converged = false; 

	if(scheme_identifier == 1)  // T1 at warm well for storing and T2 for extracting
	{
		LOG("\t\t\tconfigure scheme 1");
		if(operationType == storing)
		{
			simulation_result_aiming_at_target =
				&WellScheme_12::temperature_well1;
		}
		else
		{
			simulation_result_aiming_at_target =
				&WellScheme_12::temperature_well2;
		}

	} 
	else if(scheme_identifier == 2)  // T1 - T2
	{
		LOG("\t\t\tconfigure scheme 2");
		simulation_result_aiming_at_target =
			&WellScheme_12::temperature_difference_well1well2;
	}

	if(operationType == storing)
	{
		LOG("\t\t\t\tfor storing");
		beyond = std::move(wdc::Comparison(
			new wdc::Greater(ACCURACY_TEMPERATURE)));
		notReached =  std::move(wdc::Comparison(
			new wdc::Smaller(ACCURACY_TEMPERATURE)));
	}
	else
	{
		LOG("\t\t\t\tfor extracting");
		beyond = std::move(wdc::Comparison(
			new wdc::Smaller(ACCURACY_TEMPERATURE)));
		notReached = std::move(wdc::Comparison(
			new wdc::Greater(ACCURACY_TEMPERATURE)));
	}
}

void WellScheme_12::evaluate_simulation_result(
		const double& _T1, const double& _T2,
		const double& _heatCapacity1, const double& _heatCapacity2)
{
	set_heatFluxes(_T1, _T2, _heatCapacity1, _heatCapacity2);
	flag_converged = false;  // for flowrate adaption
 
	double simulation_result_aiming_at_target = 
		(this->*(this->simulation_result_aiming_at_target))();
	// first adapt flow rate if temperature 1 at warm well is not
	// at target value
	if(!result.flag_powerrateAdapted)
	{	// do not put this after adapt_powerrate below since
		// powerrate must be adapted in this iteration if flow rate adaption fails
		// otherwise error calucaltion in iteration loop results in zero
		if(beyond(simulation_result_aiming_at_target, value_target))
		{
			if(fabs(result.Q_w - value_threshold) > ACCURACY_FLOWRATE)
				adapt_flowrate();
			else
			{  // cannot store / extract the heat
				LOG("\t\t\tstop adapting flow rate");
				result.flag_powerrateAdapted = true;
							// start adapting powerrate
			}
		}
		else if(notReached(
			simulation_result_aiming_at_target, value_target))
		{
			if(fabs(result.Q_w) > ACCURACY_FLOWRATE)
				adapt_flowrate();
			else
			{
				std::cout << "converged\n";
				flag_converged = true;
					// cannot adapt flowrate further
					// (and powerrate is too low)
			}
		}
	}

	if(result.flag_powerrateAdapted)
		adapt_powerrate(); // continue adapting
				// iteration is checked by simulator
}

void WellScheme_12::set_flowrate()
{
	double temp, denominator;
	if(scheme_identifier == 1)
	{
		if(operationType == WellDoubletControl::storing)
			denominator = heatCapacity1 * value_target - heatCapacity2 * result.T2;
		else
			denominator = heatCapacity1 * result.T1 - heatCapacity2 * value_target;
	}
	else  // scheme C - !!!!!takes same capacity from well1 for both wells
	{ 
		denominator= heatCapacity1 * value_target;
	}

	if(fabs(denominator) < DBL_MIN)
	{
		temp = ACCURACY_FLOWRATE;
	}
	else
	{
		temp = result.Q_H / denominator;
	}

		double well_interaction_factor = 1;
		if(operationType == WellDoubletControl::storing)
			well_interaction_factor = wdc::threshold(result.T2, value_target, THRESHOLD_DELTA_WELLINTERACTION, wdc::upper);
		//else
		//	well_interaction_factor = wdc::threshold(result.T1, value_target, THRESHOLD_DELTA_WELLINTERACTION, wdc::lower);

		//LOG("\t\tWELL INTERACTION FACTOR: " << well_interaction_factor);
		/*double well2_impact_factor = (operationType == storing) ?
			wdc::threshold(result.T2, value_target,
			std::fabs(value_target)*THRESHOLD_DELTA_FACTOR_WELL2,
			wdc::upper) : 1.;	// temperature at cold well 2 
		*/			// should not reach threshold of warm well 1
		if(well_interaction_factor < 1)
		{
			temp *= well_interaction_factor;
			result.Q_H *= well_interaction_factor;
        		LOG("\t\t\tAdjust wells - set power rate\t" << result.Q_H);
			result.flag_powerrateAdapted = true;
		}
	

	result.Q_w = (operationType == WellDoubletControl::storing) ?
		wdc::confined(temp , ACCURACY_FLOWRATE, value_threshold) :
		wdc::confined(temp, value_threshold, ACCURACY_FLOWRATE);
      	
	//if(std::isnan(result.Q_w))  // no check for -nan and inf
	//	throw std::runtime_error("WellDoubletControl: nan when setting Q_w");	
        LOG("\t\t\tset flow rate\t" << result.Q_w);

}

void WellScheme_12::adapt_flowrate()
{
	double deltaT =  
		(this->*(this->simulation_result_aiming_at_target))() -
				value_target;
  
	if(scheme_identifier == 1)
		deltaT /= std::max(result.T1 - result.T2, 1.);
	else
		deltaT /= (this->*(this->simulation_result_aiming_at_target))();
	
	// decreases flowrate_adaption_factor to avoid that T1 jumps 
	// around threshold (deltaT flips sign)
	if(deltaTsign_stored != 0  // == 0: take initial value for factor
			&& deltaTsign_stored != wdc::sign(deltaT))
		flowrate_adaption_factor = (FLOWRATE_ADAPTION_FACTOR == 1)?
			flowrate_adaption_factor * 0.9 :
			flowrate_adaption_factor * FLOWRATE_ADAPTION_FACTOR;

	deltaTsign_stored = wdc::sign(deltaT);

	double well_interaction_factor = 1; 

	if(operationType == WellDoubletControl::storing)
		well_interaction_factor = wdc::threshold(result.T2, value_target, THRESHOLD_DELTA_WELLINTERACTION, wdc::upper);
	//else
	//	well_interaction_factor = wdc::threshold(result.T1, value_target, THRESHOLD_DELTA_WELLINTERACTION, wdc::lower);
				// temperature at cold well 2 
				// should not reach threshold of warm well 1
	//LOG("\t\tWELL INTERACTION FACTOR: " << well_interaction_factor);

	if(well_interaction_factor < 1)
	{	// temperature at cold well 2 is close to maximum of well 1
		result.Q_H *=  well_interaction_factor;
        	LOG("\t\t\tAdjust wells - set power rate\t" << result.Q_H);
		result.flag_powerrateAdapted = true;
	}

	result.Q_w = (operationType == WellDoubletControl::storing) ?
			wdc::confined(well_interaction_factor * result.Q_w *
				(1 + flowrate_adaption_factor * deltaT),
						ACCURACY_FLOWRATE, value_threshold) :
			wdc::confined(well_interaction_factor * result.Q_w *
				(1 - flowrate_adaption_factor * deltaT),
						value_threshold, -ACCURACY_FLOWRATE);
	
	//if(std::isnan(result.Q_w))
	//	throw std::runtime_error(
	//		"WellDoubletControl: nan when adapting Q_w");	
	LOG("\t\t\tadapt flow rate to\t" << result.Q_w);
}

void WellScheme_12::adapt_powerrate()
{
        result.Q_H -= POWERRATE_ADAPTION_FACTOR * fabs(result.Q_w) * heatCapacity1 * (
                (this->*(this->simulation_result_aiming_at_target))() -
                        // Scheme A: T1, Scheme C: T1 - T2
			// should take actually also heatCapacity2 
                value_target);
 
	if(operationType == storing && result.Q_H < 0.)
	{
		result.Q_H = 0.;
		result.Q_w = 0.;
		LOG("\t\t\tswitch off well");
	}
	else if(operationType == extracting && result.Q_H > 0.)
	{
		result.Q_H = 0.;
		result.Q_w = 0.;
		LOG("\t\t\tswitch off well");
	}
        LOG("\t\t\tadapt power rate to\t" << result.Q_H);
        result.flag_powerrateAdapted = true;
	//if(std::isnan(result.Q_H))
	//	throw std::runtime_error("WellDoubletControl: nan when adapting Q_H");	
}

