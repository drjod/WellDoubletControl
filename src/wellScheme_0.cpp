

void WellScheme_0::configure_scheme()
{
	if(operationType == storing)
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

	if(beyond(get_result().T_HE, value_threshold) || get_result().storage_state == powerrate_to_adapt)
		adapt_powerrate();
}


void WellScheme_0::estimate_flowrate()
{
        set_flowrate(value_target);
}


void WellScheme_0::adapt_powerrate()
{
        set_powerrate(get_result().Q_H - fabs(get_result().Q_W) * volumetricHeatCapacity_HE * (
                                        get_result().T_HE - value_threshold));
// combine these
	if(operationType == storing && get_result().Q_H < 0.)
		set_powerrate(0.);
	else if(operationType == extracting && get_result().Q_H > 0.)
		set_powerrate(0.);
}

