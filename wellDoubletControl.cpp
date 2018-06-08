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
	result.flag_powerrateAdapted = false;

	value_target = _value_target;
	value_threshold = _value_threshold;
	
	if(_Q_H > 0.)
	{
		LOG("\t\t\tset power rate\t\t" << std::to_string(_Q_H) <<
			+ " - storing");
		operationType = storing;
	}
	else
	{
		LOG("\t\t\tset power rate\t\t" << std::to_string(_Q_H) <<
			+ " - extracting");
		operationType = extracting;
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
	iterationState = searchingFlowrate;
	deltaTsign_stored = 0, factor = 0.1;  
		// initialization - for adapting flowrate

	if(scheme_identifier == 'A')  // T1 at warm well
	{
		LOG("\t\t\tconfigure scheme A");
		simulation_result_aiming_at_target =
			&WellSchemeAC::temperature_well1;
	} 
	else if(scheme_identifier == 'C')  // T1 - T2
	{
		LOG("\t\t\tconfigure scheme C");
		simulation_result_aiming_at_target =
			&WellSchemeAC::temperature_difference_well1well2;
	}

	if(operationType == storing)
	{
		LOG("\t\t\t\tfor storing");
		beyond = wdc::Comparison(
			new wdc::Greater(ACCURACY_TEMPERATURE_THRESHOLD));
		notReached =  wdc::Comparison(
			new wdc::Smaller(ACCURACY_TEMPERATURE_THRESHOLD));
	}
	else
	{
		LOG("\t\t\t\tfor extracting");
		beyond = wdc::Comparison(
			new wdc::Smaller(ACCURACY_TEMPERATURE_THRESHOLD));
		notReached = wdc::Comparison(
			new wdc::Greater(ACCURACY_TEMPERATURE_THRESHOLD));
	}
}

void WellSchemeAC::evaluate_simulation_result()
{
	double simulation_result_aiming_at_target = 
		(this->*(this->simulation_result_aiming_at_target))();
	// first adapt flow rate if temperature 1 at warm well is not
	// at target value
	if(iterationState == searchingFlowrate)
	{
		if(beyond(simulation_result_aiming_at_target, value_target))
		{
			// storing: T_1 > T_1^threshold + epsilon
			// extracting: T_1 < T_1^threshold - epsilon
			if(fabs(result.Q_w - value_threshold) < ACCURACY_FLOWRATE_TARGET)
			//if(wdc::at_maximum(result.Q_w, value_threshold,
			//	ACCURACY_FLOWRATE_TARGET))
        		{  // cannot store / extract the heat
                		iterationState = searchingPowerrate;  
                		LOG("\t\t\tstop adapting flow rate");
			}
			else
			{
				adapt_flowrate();
			}
		}
		else if(notReached(
			simulation_result_aiming_at_target, value_target))
		{
			// storing: T_1 < T_1^threshold - epsilon
			// extracting: T_1 > T_1^threshold + epsilon
			/*if(wdc::at_minimum(result.Q_w, value_threshold,
				ACCURACY_FLOWRATE_TARGET))
*/
			if(fabs(result.Q_w) < ACCURACY_FLOWRATE_TARGET)
          		{  // limited by flowrate
                		iterationState = converged;  
                		LOG("\t\t\tstop adapting flow rate");
			}
			else
			{
				adapt_flowrate();
			}
		}
		else  // T1 at threshold
			iterationState = converged;
	}

	if(iterationState == searchingPowerrate)
	{	
		// then adapt power rate if temperature 1 at warm well is beyond target
		// (and flow rate adaption as not succedded before)
		if(beyond(simulation_result_aiming_at_target, value_target))
		{
			adapt_powerrate();
		}
		else
			iterationState = converged;
	}
}

void WellSchemeAC::set_flowrate()
{
	double temp, denominator = 
		simulator->get_heatcapacity() * (result.T1 - result.T2);

	if(fabs(result.T1 - result.T2) < 1.e-10)
	{
		std::cout << "WARNING - well 1 is not warmer than well 2\n";
		if(operationType == storing)
		{
			std::cout << "\tset flowrate zero" << std::endl;
			result.Q_w = 0.;
		}
		else
		{
			std::cout << "\tset flowrate to threshold value"
				<< std::endl;
			result.Q_w = value_threshold;
		}
		return;
	}

	temp = result.Q_H / denominator;

        if(operationType == WellDoubletControl::storing)
		result.Q_w = wdc::confined(temp , 0., value_threshold);
	else
		result.Q_w = wdc::confined(temp, value_threshold, 0.);
      	LOG("\t\t\testimate flow rate\t" + std::to_string(result.Q_w));
}

void WellSchemeAC::adapt_flowrate()
{
        double deltaT =  
		((this->*(this->simulation_result_aiming_at_target))() -
				value_target);
  
	// avoid that T1 jumps around threshold (deltaT flips sign)
	if(deltaTsign_stored != 0  // == 0: take initial value for factor
			&& deltaTsign_stored != wdc::sign(deltaT))
		factor *= 0.7;

	deltaTsign_stored = wdc::sign(deltaT);

        if(operationType == WellDoubletControl::storing)
        {
                result.Q_w = wdc::confined(result.Q_w * (1 + factor * deltaT),
							0., value_threshold);
        }
        else
        {	// decrease absolute value of flow rate
                result.Q_w = wdc::confined(result.Q_w * (1 - factor * deltaT),
							value_threshold, 0.);
        }
        LOG("\t\t\tadapt flow rate to\t" + std::to_string(result.Q_w));
}

void WellSchemeAC::adapt_powerrate()
{
        result.Q_H -= fabs(result.Q_w) * simulator->get_heatcapacity() * (
                (this->*(this->simulation_result_aiming_at_target))() -
                        // Scheme A: T1, Scheme C: T1 - T2 
                value_target); 
        LOG("\t\t\tadapt power rate to\t" + std::to_string(result.Q_H));
        result.flag_powerrateAdapted = true;
}

void WellSchemeB::configure()
{
	iterationState = searchingPowerrate;  // not used right now

	if(operationType == storing)
	{
		LOG("\t\t\tconfigure scheme B for storing");
		beyond = wdc::Comparison(new wdc::Greater(0.));
	}
	else
	{
		LOG("\t\t\tconfigure scheme B for extracting");
		beyond = wdc::Comparison(new wdc::Smaller(0.));
	}
}

void WellSchemeB::evaluate_simulation_result()
{
	if(beyond(result.T1, value_threshold))
	{
		adapt_powerrate();
	}
	else
		iterationState = converged;
}


void WellSchemeB::set_flowrate()
{
        result.Q_w = value_target;
        LOG("\t\t\tset flow rate\t" + std::to_string(result.Q_w));
}


void WellSchemeB::adapt_powerrate()
{
        result.Q_H -= fabs(result.Q_w) * simulator->get_heatcapacity() * (
                                        result.T1 - value_threshold);
        LOG("\t\t\tadapt power rate\t" + std::to_string(result.Q_H));
        result.flag_powerrateAdapted = true;
}


