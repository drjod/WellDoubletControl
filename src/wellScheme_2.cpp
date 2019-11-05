
void WellScheme_2::configure_scheme()
{
	deltaTsign_stored = 0, flowrate_adaption_factor = c_flowrate_adaption_factor;  

	LOG("\t\t\tconfigure scheme 1");

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
	if(get_result().storage_state == on_demand)
	{	// do not put this after adapt_powerrate below since
		// powerrate must be adapted in this iteration if flow rate adaption fails
		// otherwise error calucaltion in iteration loop results in zero
		if(beyond(value, value_target))
		{
std::cout << "value_threshold: " << value_threshold << "\n";
//std::cout << "Q_w: " << get_result().Q_W << "\n";
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
		else if(notReached(value, value_target))
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

void WellScheme_2::estimate_flowrate()
{
	double temp, denominator, flowrate;

	if(operationType == WellDoubletControl::storing)
		denominator = volumetricHeatCapacity_HE * get_result().T_HE - volumetricHeatCapacity_UA * get_result().T_UA;
	else // just -
		denominator = - (volumetricHeatCapacity_HE * get_result().T_HE - volumetricHeatCapacity_UA * get_result().T_UA);
	LOG("\t\tdenom: " << denominator);

	if(fabs(denominator) < DBL_MIN)
	{
		temp = accuracies.flowrate;
	}
	else
	{
		temp = get_result().Q_H / denominator;
	}

//temp = value_threshold / 2;

	flowrate = (operationType == WellDoubletControl::storing) ?
		wdc::make_confined(temp , accuracies.flowrate, value_threshold) :
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
  
	if(operationType == WellDoubletControl::storing)
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

	if(operationType == WellDoubletControl::storing)
	{
		spread = volumetricHeatCapacity_HE * get_result().T_HE - volumetricHeatCapacity_UA * get_result().T_UA; 
		if(fabs(spread) < 1.e-10) spread = 1.e-10;

		powerrate = get_result().Q_H * (1 - c_powerrate_adaption_factor * 
			( spread - value_target) / spread);
 	}
	else
	{
		spread = volumetricHeatCapacity_UA * get_result().T_UA - volumetricHeatCapacity_HE * get_result().T_HE; 
		if(fabs(spread) < 1.e-10) spread = 1.e-10;

		powerrate = get_result().Q_H * (1 - c_powerrate_adaption_factor * 
			( value_target + spread) / spread);
	}
std::cout << "spread: " << spread << '\n';

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

