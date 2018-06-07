#ifndef WELLDOUBLETCALCULATION_H
#define WELLDOUBLETCALCULATION_H

#include "parameter.h"

class WellSchemeAC;
class WellSchemeB;


class WellDoubletCalculation
{
        double Q_H, Q_w;  // power rate Q_H (given input, potentially adapted),
				// flow rate Q_w either calculated
				// (schemes A and C) or set as input (Scheme B)
        double T1, T2;  // temperature at well 1 ("warm well") 
			// and 2 ("cold well") obtained from simulation results
        bool flag_powerrateAdapted;  // says if storage meets requirement or not
public:
	double (WellDoubletCalculation::*simulation_result_aiming_at_target)
									() const;
		// scheme A: pointing at temperature_Well1(),
		// scheme C: pointing at temperature_difference(),
		// not used in scheme B, where target value (flow rate)
		// is just set

	const double& powerrate() const { return Q_H; }
	const double& flowrate() const { return Q_w; }

	double temperature_well1() const { return T1; }  // to evaluate target
	const double& temperature_well2() const { return T2; }
	double temperature_difference_well1well2() const { return T1 - T2; }  
							// to evaluate target

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
