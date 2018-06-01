#ifndef WELLDOUBLETCALCULATION_H
#define WELLDOUBLETCALCULATION_H

#include "parameter.h"

class WellSchemeA;
class WellSchemeB;
class WellSchemeC;


struct WellDoubletCalculation
{
        double Q_H, Q_w;  // power rate, flow rate
        double T1, T2;  // temperature at well 1 ("warm well") and 2 ("cold well")

        bool flag_powerrateAdapted;
        bool flag_flowrateAdapted;

        void adapt_powerrate(WellSchemeA* scheme);
        void adapt_powerrate(WellSchemeB* scheme);
        void adapt_powerrate(WellSchemeC* scheme);

        void calculate_flowrate(WellSchemeA* scheme);
        void calculate_flowrate(WellSchemeB* scheme);
        void calculate_flowrate(WellSchemeC* scheme);
};

#endif
