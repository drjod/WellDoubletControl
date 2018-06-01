#include "wellDoubletCalculation.h"
#include "wellDoubletControl.h"
#include "fakeSimulator.h"



void WellDoubletCalculation::adapt_powerrate(WellSchemeA* scheme)
{
        Q_H -= Q_w * scheme->simulator->get_heatcapacity() * 1* (
					T1 - scheme->value_target);
        LOG("\t\t\tadapt power rate to " + std::to_string(Q_H));
        flag_powerrateAdapted = true;

}

void WellDoubletCalculation::adapt_powerrate(WellSchemeB* scheme)
{
        Q_H -= Q_w * scheme->simulator->get_heatcapacity() * (
                                        T1 - scheme->value_threshold);
        LOG("\t\t\tadapt power rate to " + std::to_string(Q_H));
        flag_powerrateAdapted = true;
}

void WellDoubletCalculation::adapt_powerrate(WellSchemeC* scheme)
{
        Q_H = Q_w * scheme->simulator->get_heatcapacity() * (
                                        scheme->value_threshold - T2);
        LOG("\t\t\tadapt power rate to " + std::to_string(Q_H));
        flag_powerrateAdapted = true;

}

void WellDoubletCalculation::calculate_flowrate(WellSchemeA* scheme)
{
if(flag_powerrateAdapted)
return;
        if(scheme->flag_storing)
                Q_w = fmin(Q_H / (scheme->simulator->get_heatcapacity()  *
                        (2*scheme->value_target - T1- T2)), scheme->value_threshold);
        else
                Q_w = fmax(Q_H / (scheme->simulator->get_heatcapacity()  *
                        (scheme->value_target - T2)), scheme->value_threshold);
LOG(Q_w);
}

void WellDoubletCalculation::calculate_flowrate(WellSchemeB* scheme)
{
        Q_w = scheme->value_target;
}

void WellDoubletCalculation::calculate_flowrate(WellSchemeC* scheme)
{
        // no range check
        if(scheme->flag_storing)
                Q_w = Q_H / (scheme->simulator->get_heatcapacity() *
                                                        scheme->value_target);
        else
                Q_w = Q_H / (scheme->simulator->get_heatcapacity() *
                                                        scheme->value_target);
}


