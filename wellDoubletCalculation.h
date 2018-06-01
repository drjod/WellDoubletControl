#ifndef WELLDOUBLETCALCULATION_H
#define WELLDOUBLETCALCULATION_H

#include "parameter.h"

class WellSchemeAC;
class WellSchemeB;


class WellDoubletCalculation
{
        double Q_H, Q_w;  // power rate, flow rate
        double T1, T2;  // temperature at well 1 ("warm well") 
			// and 2 ("cold well")

        bool flag_powerrateAdapted;
        bool flag_flowrateAdapted;

public:
	double (WellDoubletCalculation::*value_aiming_at_target) () const;
		// used to differentiate between scheme A and C
		// not used in scheme B, where target value (flow rate)
		// is just set

	const double& powerrate() const { return Q_H; }
	const double& flowrate() const { return Q_w; }

	double temperature_well1() const { return T1; }
	const double& temperature_well2() const { return T2; }
	double temperature_difference() const { return T1 - T2; }

	const bool& powerrateAdapted() const { return flag_powerrateAdapted; }

        void adapt_powerrate(WellSchemeAC* scheme);
        void adapt_powerrate(WellSchemeB* scheme);

        void adapt_flowrate(WellSchemeAC* scheme);

        void estimate_flowrate(WellSchemeAC* scheme);
        void set_flowrate(WellSchemeB* scheme);

	friend class WellSchemeAC;
	friend class WellSchemeB;
	friend class WellDoubletControl;
};

#endif
