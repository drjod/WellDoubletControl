#ifndef WELLDOUBLETCONTROL_H
#define WELLDOUBLETCONTROL_H

#define THRESHOLD_DELTA_FACTOR_WELL2 0.01
// wells are switched of if temperature at cold well2 get
// close to temperature at warm well 1, i.e. temperature at well 2 exceeds
// threshold_value *(1 - THRESHOLD_DELTA_FACTOR_WELL2)
#define POWERRATE_ADAPTION_FACTOR 0.5
// used in schemes A/C - try using 1 to reduce number of iterations
#define FLOWRATE_ADAPTION_FACTOR .9
// used in schemes A/C - it is modified during iteration with a mutable variable
// Q_w = Q_w (1 +/- a (T_1 - value_target) / (T1 - T2)) for scheme A 
// it is multiplied with itself, if a threshold is hit
// therefore: DO NOT USE 1 as value

#define ACCURACY_FLOWRATE 1.e-5
// used for comparison with threshold and zero, it is also minimum flowrate
#define ACCURACY_TEMPERATURE 1.e-1
// used for thresholds

#include "comparison.h"
#include <string>

class WellDoubletControl
{
public:
	struct result_t
	{
		double Q_H, Q_w;  // power rate Q_H (given input, potentially adapted),
                                // flow rate Q_w either calculated
                                // (schemes A and C) or set as input (Scheme B)
        	double T1, T2;  // temperature at well 1 ("warm well") 
                        // and 2 ("cold well") obtained from simulation results
        	bool flag_powerrateAdapted;  // says if storage meets requirement or not
	};
protected:
	result_t result;
	double heatCapacity1, heatCapacity2;  // Schemes A/B: 1: at warm well, 2: ar cold well
				// Scheme C: 1 for both wells

	char scheme_identifier; // 'A', 'B', or 'C'	
	double value_target, value_threshold;
	// A: T1_target, Q_w_max 
	// B: Q_w_target, T1_max 
	// C: DT_target, Q_w_max

	enum {storing, extracting} operationType;

	wdc::Comparison beyond, notReached;

	void set_heatFluxes(const double& _T1, const double& _T2,
				const double& _heatCapacity1, const double& _heatCapacity2);  
					// called in evaluate_simulation_result
	virtual void set_flowrate() = 0;
	void write_outputFile() const;
public:
	WellDoubletControl() : value_target(0.) { result.Q_w = 0.; }

	virtual ~WellDoubletControl() = default;

	const WellDoubletControl::result_t& get_result() const { return result; }

	virtual void configureScheme() = 0;
	virtual void evaluate_simulation_result(const double& _T1, const double& _T2,
			const double& _heatCapacity1, const double& _heatCapacity2) = 0;
	
	void configure(const double& _Q_H,  
		const double& _value_target, const double& _value_threshold,
		const double& _T1, const double& _T2,
		const double& _heatCapacity1, const double& _heatCapacity2);
			// constraints are set at beginning of time step

	static WellDoubletControl* create_wellDoubletControl(
				const char& selection);
			// instance is created before time-stepping
	
	void print_temperatures() const;
	const char& get_schemeIdentifier() const { return scheme_identifier; }
	virtual bool converged(double _T1, double accuracy) const = 0;
};


class WellSchemeAC : public WellDoubletControl
{
        double temperature_well1() const { return result.T1; }  // to evaluate target
        double temperature_difference_well1well2() const { return result.T1 - result.T2; }
                                                        // to evaluate target
        double (WellSchemeAC::*simulation_result_aiming_at_target) () const;
	// to differentiate between scheme A and C
        // scheme A: pointing at temperature_Well1(),
        // scheme C: pointing at temperature_difference(),
	double flowrate_adaption_factor, deltaTsign_stored;
	// to store values from the last interation when adapting flowrate
        void set_flowrate();
       	void adapt_flowrate();
        void adapt_powerrate();
	void configureScheme();
	bool flag_converged; // for the case that target cannot be reached by adapting flow rate
public:
	WellSchemeAC(const char& _scheme_identifier)
	{ scheme_identifier = _scheme_identifier; }

	void evaluate_simulation_result(const double& _T1, const double& _T2,
			const double& _heatCapacity1, const double& _heatCapacity2);
	bool converged(double _T1, double accuracy) const { return (fabs(_T1 - value_target) < accuracy || flag_converged); }
};


class WellSchemeB : public WellDoubletControl
{
	void configureScheme();
public:
	WellSchemeB(const char& _scheme_identifier)
	{ scheme_identifier = _scheme_identifier; }

	void evaluate_simulation_result(const double& _T1, const double& _T2,
			const double& _heatCapacity1, const double& _heatCapacity2);

        void set_flowrate();
        void adapt_powerrate();
	bool converged(double _T1, double accuracy) const { return false; }
		// convergence exclusively decided by simulator
};

#endif
