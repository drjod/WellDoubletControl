
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


	if(operationType == storing)
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
	if(get_result().storage_state == on_demand)
	{	// do not put this after adapt_powerrate below since
		// powerrate must be adapted in this iteration if flow rate adaption fails
		// otherwise error calucaltion in iteration loop results in zero
		if(beyond(get_result().T_HE, value_target))
		{
			if(fabs(get_result().Q_W - value_threshold) > accuracies.flowrate)
				adapt_flowrate();
			else
			{  // cannot store / extract the heat
				LOG("\t\t\tstop adapting flow rate");
				set_storage_state(powerrate_to_adapt);
				//set_powerrate(get_result().Q_H);  // to set flag
							// start adapting powerrate
			}
		}
		else if(notReached(get_result().T_HE, value_target))
		{
			if(fabs(get_result().Q_W) > accuracies.flowrate)
				adapt_flowrate();
			else
			{
				set_storage_state(target_not_achievable);
					// cannot adapt flowrate further (and powerrate is too low)
			}
		}
	}

	if(get_result().storage_state == powerrate_to_adapt)
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
		temp = accuracies.flowrate;
	}
	else
	{
		temp = get_result().Q_H / denominator;
	}

	double operability = 1;
	if(operationType == WellDoubletControl::storing)
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
	if(operability < 1)
	{
		temp *= operability;
		set_powerrate(get_result().Q_H * operability);
		//LOG("\t\t\tAdjust wells - set power rate\t" << get_result().Q_H);
	}


	set_flowrate((operationType == WellDoubletControl::storing) ?
		wdc::make_confined(temp , accuracies.flowrate, value_threshold) :
		wdc::make_confined(temp, value_threshold, accuracies.flowrate));
      	
	//if(std::isnan(get_result().Q_W))  // no check for -nan and inf
	//	throw std::runtime_error("WellDoubletControl: nan when setting Q_W");	

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

	double operability = 1;  // [0, 1]

 	if(operationType == WellDoubletControl::storing)
		operability = wdc::make_threshold_factor(get_result().T_UA, value_target,
						well_shutdown_temperature_range, wdc::upper);
	else
		operability = wdc::make_threshold_factor(get_result().T_UA, value_target, 
						well_shutdown_temperature_range, wdc::lower);
				// temperature at cold well 2 
				// should not reach threshold of warm well 1
	LOG("\t\tOperability: " << operability);

	if(operability < 1)
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
 
	if(operationType == storing && powerrate < 0.)
	{
		set_powerrate(0.);
		set_flowrate(0.);
		LOG("\t\t\tswitch off well");
	}
	else if(operationType == extracting && powerrate > 0.)
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

