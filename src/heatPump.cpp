#include "heatPump.h"
#include <iostream>

namespace wdc
{

double CarnotHeatPump::calculate_heat_source(const double& _heat_sink,
		const double& T_source_in, const double& T_source_out)
{
	const double conversion = (T_sink>200)? 0: 273.15; // °C->K
	COP = eta * (T_sink + conversion) / (T_sink - T_source_in);
	if(COP < 0.)
	{
		COP = -1;
		return _heat_sink;
	}
	else
	{
		heat_sink = _heat_sink;
		WDC_LOG("Carnot COP: " << COP);
		return _heat_sink * (COP-1) / COP;
	}
}


double NoHeatPump::calculate_heat_source(const double& _heat_sink,
		const double& T_source_in, const double& T_source_out)
{
	heat_sink = _heat_sink;
	COP = -1.;
	return _heat_sink;
}

}
