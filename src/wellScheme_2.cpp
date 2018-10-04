
void WellScheme_2::configure_scheme()
{
	deltaTsign_stored = 0, flowrate_adaption_factor = c_flowrate_adaption_factor;  
	//flag_converged = false; 

	LOG("\t\t\tconfigure scheme 2");
	//simulation_result_aiming_at_target =
	//	&WellScheme_2::temperature_difference_well1well2;

	if(operationType == storing)
	{
		LOG("\t\t\t\tfor storing");
		//beyond = std::move(wdc::Comparison(
		//	new wdc::Greater(c_accuracy_temperature)));
		//notReached =  std::move(wdc::Comparison(
		//	new wdc::Smaller(c_accuracy_temperature)));
	}
	else
	{
		LOG("\t\t\t\tfor extracting");
		//beyond = std::move(wdc::Comparison(
		//	new wdc::Smaller(c_accuracy_temperature)));
		//notReached = std::move(wdc::Comparison(
		//	new wdc::Greater(c_accuracy_temperature)));
	}
}

void WellScheme_2::evaluate_simulation_result(const balancing_properties_t& balancing_properties)
{
	set_balancing_properties(balancing_properties);
	//flag_converged = false;  // for flowrate adaption
 
	//double simulation_result_aiming_at_target = 
	//	(this->*(this->simulation_result_aiming_at_target))();
	// first adapt flow rate if temperature 1 at warm well is not
	// at target value
	/*if(!get_result().flag_powerrateAdapted)
	{	// do not put this after adapt_powerrate below since
		// powerrate must be adapted in this iteration if flow rate adaption fails
		// otherwise error calucaltion in iteration loop results in zero
		if(beyond(simulation_result_aiming_at_target, value_target))
		{
			if(fabs(get_result().Q_W - value_threshold) > c_accuracy_flowrate)
				adapt_flowrate();
			else
			{  // cannot store / extract the heat
				LOG("\t\t\tstop adapting flow rate");
				set_powerrate(get_result().Q_H);  // to set flag
							// start adapting powerrate
			}
		}
		else if(notReached(
			simulation_result_aiming_at_target, value_target))
		{
			if(fabs(get_result().Q_W) > c_accuracy_flowrate)
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
	*/
	if(get_result().storage_state == powerrate_to_adapt)
		adapt_powerrate(); // continue adapting
				// iteration is checked by simulator
}

void WellScheme_2::estimate_flowrate()
{
/*
	double temp, denominator;
	
	denominator= volumetricHeatCapacity_HE * value_target;

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
			well_interaction_factor = wdc::threshold(get_result().T_HE, value_target,
							c_well_shutdown_temperature_range, wdc::lower);

		//LOG("\t\tWELL INTERACTION FACTOR: " << well_interaction_factor);
		//double well2_impact_factor = (operationType == storing) ?
		//	wdc::threshold(result.T_UA, value_target,
		//	std::fabs(value_target)*THRESHOLD_DELTA_FACTOR_WELL2,
		//	wdc::upper) : 1.;	// temperature at cold well 2 
		//			// should not reach threshold of warm well 1
		if(well_interaction_factor < 1)
		{
			temp *= well_interaction_factor;
			set_powerrate(get_result().Q_H * well_interaction_factor);
        		LOG("\t\t\tAdjust wells - set power rate\t" << get_result().Q_H);
		}
	
	// ?????
	//set_flowrate(operationType == WellDoubletControl::storing) ?
	//	wdc::confined(temp , c_accuracy_flowrate, value_threshold) :
	//	wdc::confined(temp, value_threshold, c_accuracy_flowrate));
      	
	//if(std::isnan(result.Q_W))  // no check for -nan and inf
	//	throw std::runtime_error("WellDoubletControl: nan when setting Q_W");	
*/
}

void WellScheme_2::adapt_flowrate()
{
/*
	double deltaT; 
	// =  (this->*(this->simulation_result_aiming_at_target))() -
	//			value_target;
  	//
	//deltaT /= (this->*(this->simulation_result_aiming_at_target))();
	
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
		well_interaction_factor = wdc::threshold(get_result().T_HE, value_target, 
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
			wdc::confined(well_interaction_factor * get_result().Q_W *
				(1 + flowrate_adaption_factor * deltaT),
						c_accuracy_flowrate, value_threshold) :
			wdc::confined(well_interaction_factor * get_result().Q_W *
				(1 - flowrate_adaption_factor * deltaT),
						value_threshold, - c_accuracy_flowrate));
	
	//if(std::isnan(result.Q_W))
	//	throw std::runtime_error(
	//		"WellDoubletControl: nan when adapting Q_W");	
*/
}

void WellScheme_2::adapt_powerrate()
{
        /*set_powerrate(get_result().Q_H - c_powerrate_adaption_factor * fabs(get_result().Q_W) * volumetricHeatCapacity_HE * (
                (this->*(this->simulation_result_aiming_at_target))() -
                        // Scheme A: T_HE, Scheme C: T_HE - T_UA
			// should take actually also volumetricHeatCapacity_UA 
                value_target));
 	*/
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
	//if(std::isnan(result.Q_H))
	//	throw std::runtime_error("WellDoubletControl: nan when adapting Q_H");	
}

