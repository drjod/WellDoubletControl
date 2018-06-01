#include "wellDoubletCalculation.h"
#include "wellDoubletControl.h"
#include "fakeSimulator.h"



void WellDoubletCalculation::adapt_powerrate(WellSchemeAC* scheme)
{
        Q_H -= Q_w * scheme->simulator->get_heatcapacity() * (
		(this->*simulation_result_aiming_at_target)() -
			// Scheme A: T1, Scheme C: T1 - T2 
		scheme->value_target);
        LOG("\t\t\tadapt power rate\t" + std::to_string(Q_H));
        flag_powerrateAdapted = true;
}

void WellDoubletCalculation::adapt_powerrate(WellSchemeB* scheme)
{
        Q_H -= Q_w * scheme->simulator->get_heatcapacity() * (
                                        T1 - scheme->value_threshold);
        LOG("\t\t\tadapt power rate\t" + std::to_string(Q_H));
        flag_powerrateAdapted = true;
}

void WellDoubletCalculation::estimate_flowrate(WellSchemeAC* scheme)
{
        if(scheme->flag_storing)
                Q_w = fmin(Q_H / (scheme->simulator->get_heatcapacity()  *
                        (2 * scheme->value_target - T1- T2)), scheme->value_threshold);
        else
                Q_w = fmax(Q_H / (scheme->simulator->get_heatcapacity()  *
                        (2 * scheme->value_target - T1 - T2)), scheme->value_threshold);
        LOG("\t\t\testimate flow rate\t" + std::to_string(Q_w));
}

void WellDoubletCalculation::set_flowrate(WellSchemeB* scheme)
{
        Q_w = scheme->value_target;
        LOG("\t\t\tset flow rate\t" + std::to_string(Q_w));
}

void WellDoubletCalculation::adapt_flowrate(WellSchemeAC* scheme)
{
	double delta = 4 * ((this->*simulation_result_aiming_at_target)() - 
			scheme->value_target) / scheme->value_target;

        if(scheme->flag_storing)
	{
		Q_w = fmin( Q_w * (1 + delta), scheme->value_threshold);
		Q_w = fmax(Q_w, 0.); // else extracting
	}
	else
	{
		Q_w = fmax( Q_w * (1 + delta), scheme->value_threshold);
		Q_w = fmin(Q_w, 0.);  // else storing
	}

        LOG("\t\t\tadapt flow rate\t\t" + std::to_string(Q_w));
}
