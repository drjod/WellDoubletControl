
void WellScheme_1::configureScheme()
{
	deltaTsign_stored = 0, flowrate_adaption_factor = c_flowrate_adaption_factor;  
	flag_converged = false; 

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


	if(operationType == storing)
	{
		LOG("\t\t\t\tfor storing");
		beyond = std::move(wdc::Comparison(
			new wdc::Greater(c_accuracy_temperature)));
		notReached =  std::move(wdc::Comparison(
			new wdc::Smaller(c_accuracy_temperature)));
	}
	else
	{
		LOG("\t\t\t\tfor extracting");
		beyond = std::move(wdc::Comparison(
			new wdc::Smaller(c_accuracy_temperature)));
		notReached = std::move(wdc::Comparison(
			new wdc::Greater(c_accuracy_temperature)));
	}
}

void WellScheme_1::evaluate_simulation_result(const balancing_properties_t& balancing_properties)
{
	set_balancing_properties(balancing_properties);
	flag_converged = false;  // for flowrate adaption
 
	//double simulation_result_aiming_at_target = 
	//	(this->*(this->simulation_result_aiming_at_target))();
	// first adapt flow rate if temperature 1 at warm well is not
	// at target value
	if(!get_result().flag_powerrateAdapted)
	{	// do not put this after adapt_powerrate below since
		// powerrate must be adapted in this iteration if flow rate adaption fails
		// otherwise error calucaltion in iteration loop results in zero
		if(beyond(get_result().T_HE, value_target))
		{
			if(fabs(get_result().Q_w - value_threshold) > c_accuracy_flowrate)
				adapt_flowrate();
			else
			{  // cannot store / extract the heat
				LOG("\t\t\tstop adapting flow rate");
				set_powerrate(get_result().Q_H);  // to set flag
							// start adapting powerrate
			}
		}
		else if(notReached(get_result().T_HE, value_target))
		{
			if(fabs(get_result().Q_w) > c_accuracy_flowrate)
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

	if(get_result().flag_powerrateAdapted)
		adapt_powerrate(); // continue adapting
				// iteration is checked by simulator
}

void WellScheme_1::estimate_flowrate()
{
	double temp, denominator;

	if(operationType == WellDoubletControl::storing)
		denominator = volumetricHeatCapacity_HE * value_target - volumetricHeatCapacity_UA * get_result().T_UA;
	else // just -
		denominator = volumetricHeatCapacity_UA * get_result().T_UA - volumetricHeatCapacity_HE * value_target;

	if(fabs(denominator) < DBL_MIN)
	{
		temp = c_accuracy_flowrate;
	}
	else
	{
		temp = get_result().Q_H / denominator;
	}

	double well_interaction_factor = 1;
	if(operationType == WellDoubletControl::storing)
		well_interaction_factor = wdc::threshold(get_result().T_UA, value_target,
						c_well_shutdown_temperature_range, wdc::upper);
	else
		well_interaction_factor = wdc::threshold(get_result().T_UA, value_target,
						c_well_shutdown_temperature_range, wdc::lower);

	LOG("\t\tWELL INTERACTION FACTOR: " << well_interaction_factor);
	//double well2_impact_factor = (operationType == storing) ?
	//	wdc::threshold(get_result().T_UA, value_target,
	//	std::fabs(value_target)*THRESHOLD_DELTA_FACTOR_WELL2,
	//	wdc::upper) : 1.;	// temperature at cold well 2 
				// should not reach threshold of warm well 1
	if(well_interaction_factor < 1)
	{
		temp *= well_interaction_factor;
		set_powerrate(get_result().Q_H * well_interaction_factor);
		//LOG("\t\t\tAdjust wells - set power rate\t" << get_result().Q_H);
	}
	

	set_flowrate((operationType == WellDoubletControl::storing) ?
		wdc::confined(temp , c_accuracy_flowrate, value_threshold) :
		wdc::confined(temp, value_threshold, c_accuracy_flowrate));
      	
	//if(std::isnan(get_result().Q_w))  // no check for -nan and inf
	//	throw std::runtime_error("WellDoubletControl: nan when setting Q_w");	

}

void WellScheme_1::adapt_flowrate()
{
	double deltaT = get_result().T_HE - value_target;
  
	if(operationType == WellDoubletControl::storing)
		deltaT /= std::max(get_result().T_HE - get_result().T_UA, 1.);
	else
		deltaT /= std::max(get_result().T_UA - get_result().T_HE, 1.);
	
	// decreases flowrate_adaption_factor to avoid that T_HE jumps 
	// around threshold (deltaT flips sign)
	if(deltaTsign_stored != 0  // == 0: take initial value for factor
			&& deltaTsign_stored != wdc::sign(deltaT))
		flowrate_adaption_factor = (c_flowrate_adaption_factor == 1)?
			flowrate_adaption_factor * 0.9 :  // ?????
			flowrate_adaption_factor * c_flowrate_adaption_factor;

	deltaTsign_stored = wdc::sign(deltaT);

	double well_interaction_factor = 1; 

	if(operationType == WellDoubletControl::storing)
		well_interaction_factor = wdc::threshold(get_result().T_UA, value_target,
						c_well_shutdown_temperature_range, wdc::upper);
	else
		well_interaction_factor = wdc::threshold(get_result().T_UA, value_target, 
						c_well_shutdown_temperature_range, wdc::lower);
				// temperature at cold well 2 
				// should not reach threshold of warm well 1
	LOG("\t\tWELL INTERACTION FACTOR: " << well_interaction_factor);

	if(well_interaction_factor < 1)
	{	// storing: temperature at cold well 2 is close to maximum of well 1
		// extracting: temperature at warm well 1 is close to minumum
		set_powerrate(get_result().Q_H * well_interaction_factor);
        	//LOG("\t\t\tAdjust wells - set power rate\t" << get_result().Q_H);
	}

	set_flowrate((operationType == WellDoubletControl::storing) ?
			wdc::confined(well_interaction_factor * get_result().Q_w *
				(1 + flowrate_adaption_factor * deltaT),
						c_accuracy_flowrate, value_threshold) :
			wdc::confined(well_interaction_factor * get_result().Q_w *
				(1 - flowrate_adaption_factor * deltaT),
						value_threshold, -c_accuracy_flowrate));
	
	//if(std::isnan(get_result().Q_w))
	//	throw std::runtime_error(
	//		"WellDoubletControl: nan when adapting Q_w");	
}

void WellScheme_1::adapt_powerrate()
{
        set_powerrate(get_result().Q_H - c_powerrate_adaption_factor * fabs(get_result().Q_w) * volumetricHeatCapacity_HE * (
                get_result().T_HE -
                        // Scheme A: T_HE, Scheme C: T_HE - T_UA
			// should take actually also volumetricHeatCapacity_UA 
                value_target));
 
	if(operationType == storing && get_result().Q_H < 0.)
	{
		set_powerrate(0.);
		set_flowrate(0.);
		LOG("\t\t\tswitch off well");
	}
	else if(operationType == extracting && get_result().Q_H > 0.)
	{
		set_powerrate(0.);
		set_flowrate(0.);
		LOG("\t\t\tswitch off well");
	}
	//if(std::isnan(get_result().Q_H))
	//	throw std::runtime_error("WellDoubletControl: nan when adapting Q_H");	
}

