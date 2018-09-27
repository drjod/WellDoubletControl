#ifndef WELL_DOUBLET_CONTROL_H
#define WELL_DOUBLET_CONTROL_H

const double c_well_shutdown_temperature_range = 10.;
// for schemes 1, 2
// well doublet is shutdown if storage becomes full (when storing) or empty (when extracting) 
// i.e.  flow and powerrate gruadually become zero if
// 	storing:  T_UA (cold well) exceeds value_threshold - c_well_shutdown_temperature_range
//	extracting: T_HE (warm well) falls below value_threshlod + c_well_shutdown_temperature_range 
const double c_powerrate_adaption_factor = 0.9;
// used in schemes 1 and 2 - try using 1 to reduce number of iterations
const double c_flowrate_adaption_factor = .5;
// used in schemes 1 2 - it is modified during iteration with a mutable variable
// Q_w = Q_w (1 +/- a (T_1 - value_target) / (T_HE - T_UA)) for scheme 1
// it is multiplied with itself, if a threshold is hit
// therefore: DO NOT USE 1 as value

const double c_accuracy_flowrate = 1.e-5;
// used for comparison with threshold and zero, it is also minimum absolute flowrate
const double c_accuracy_temperature = 1.e-1;
// used for thresholds

#include "wdc_config.h"
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
        	double T_HE, T_UA;  // temperature at heat exchanger and in upwind aquifer
        	bool flag_powerrateAdapted;  // says if storage meets requirement or not
	};

	struct balancing_properties_t  // helper struct to pass properties around
	{
		double T_HE, T_UA, volumetricHeatCapacity_HE, volumetricHeatCapacity_UA;
	};
private:
	result_t result;  // for the client
protected:
	void set_flowrate(const double& _Q_w)
	{ 
		result.Q_w = _Q_w; 
		LOG("\t\t\tset flow rate\t" << _Q_w);
	}
	void set_powerrate(const double& _Q_H) 
	{ 
		result.Q_H = _Q_H;
		result.flag_powerrateAdapted = true;
		LOG("\t\t\tadapt power rate\t" << _Q_H);
	}

	//  TO REMOVE
        double temperature_well1() const { return result.T_HE; }  // to evaluate target
        double temperature_well2() const { return result.T_UA; }  // to evaluate target
        double temperature_difference_well1well2() const { return result.T_HE - result.T_UA; }
                                                        // to evaluate target
	double volumetricHeatCapacity_HE, volumetricHeatCapacity_UA;  // Schemes A/B: 1: at warm well, 2: ar cold well
				// Scheme C: 1 for both wells

	double value_target, value_threshold;
	// A: T_HE_target, Q_w_max 
	// B: Q_w_target, T_HE_max 
	// C: DT_target, Q_w_max

	enum {storing, extracting} operationType;

	wdc::Comparison beyond, notReached;

	void set_balancing_properties(const balancing_properties_t& balancing_properites);
					// called in evaluate_simulation_result
	virtual void estimate_flowrate() = 0;
	void write_outputFile() const;
public:
	WellDoubletControl() : value_target(0.) { result.Q_w = 0.; }
	virtual int scheme_identifier() const = 0;

	virtual ~WellDoubletControl() = default;

	result_t get_result() const { return result; }
	virtual void configureScheme() = 0;
	virtual void evaluate_simulation_result(const balancing_properties_t& balancing_properites) = 0;
	
	void configure(const double& _Q_H,  
		const double& _value_target, const double& _value_threshold, 
		const balancing_properties_t& balancing_properites);
			// constraints are set at beginning of time step

	static WellDoubletControl* create_wellDoubletControl(
				const int& selection);
			// instance is created before time-stepping
	
	void print_temperatures() const;
	virtual bool converged(double _T_HE, double accuracy) const = 0;
};


class WellScheme_0 : public WellDoubletControl
{
	void configureScheme() override;
public:
	WellScheme_0() = default;
	int scheme_identifier() const override { return 0; }

	void evaluate_simulation_result(const balancing_properties_t& balancing_properites) override;

        void estimate_flowrate() override;
        void adapt_powerrate();
	bool converged(double, double) const override { return false; }
		// convergence exclusively decided by simulator
};

class WellScheme_1 : public WellDoubletControl
{
        //double temperature_well1() const { return result.T_HE; }  // to evaluate target
        //double temperature_well2() const { return result.T_UA; }  // to evaluate target
        //double temperature_difference_well1well2() const { return result.T_HE - result.T_UA; }
                                                        // to evaluate target
        double (WellScheme_1::*simulation_result_aiming_at_target) () const;
	// to differentiate between scheme A and C
        // scheme A: pointing at temperature_Well1(),
        // scheme C: pointing at temperature_difference(),
	double flowrate_adaption_factor, deltaTsign_stored;
	// to store values from the last interation when adapting flowrate
        void estimate_flowrate() override;
       	void adapt_flowrate();
        void adapt_powerrate();
	void configureScheme() override;
	bool flag_converged; // for the case that target cannot be reached by adapting flow rate
public:
	int scheme_identifier() const override { return 1; }
	WellScheme_1() = default;

	void evaluate_simulation_result(const balancing_properties_t& balancing_properites) override;
	bool converged(double _T_HE, double accuracy) const override { return (fabs(_T_HE - value_target) < accuracy || flag_converged); }
};


class WellScheme_2 : public WellDoubletControl
{
        double (WellScheme_2::*simulation_result_aiming_at_target) () const;
	// to differentiate between scheme A and C
        // scheme A: pointing at temperature_Well1(),
        // scheme C: pointing at temperature_difference(),
	double flowrate_adaption_factor, deltaTsign_stored;
	// to store values from the last interation when adapting flowrate
        void estimate_flowrate() override;
       	void adapt_flowrate();
        void adapt_powerrate();
	void configureScheme() override;
	bool flag_converged; // for the case that target cannot be reached by adapting flow rate
public:
	int scheme_identifier() const override { return 2; }
	WellScheme_2() = default;

	void evaluate_simulation_result(const balancing_properties_t& balancing_properites) override;
	bool converged(double _T_HE, double accuracy) const override { return (fabs(_T_HE - value_target) < accuracy || flag_converged); }
};

#endif
