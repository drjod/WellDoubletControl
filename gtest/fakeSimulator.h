#ifndef FAKESIMULATOR_H
#define FAKESIMULATOR_H

#include <string>
#include "wellDoubletControl.h"
#include "parameter.h"


class WellDoubletControl;
class WellDoubletCalculation;


class FakeSimulator
{
        double temperatures[GRID_SIZE];  //  temperatures at warm well 
				// (distance increases with node number) 
        double temperatures_old[GRID_SIZE];
	double heatcapacity;  // stands for parameters accecible to 
				// wellDoubletControl

        WellDoubletControl* wellDoubletControl;
        bool flag_iterate;  // to convert threshold value into a target value

public:
	FakeSimulator() : heatcapacity(HEATCAPACITY), wellDoubletControl(0) {}
	~FakeSimulator() 
	{ if(wellDoubletControl != 0) delete wellDoubletControl; }
			// a wellDoubletControl instance is constructed 
			// (and destructed) each time step

	double& get_heatcapacity() { return heatcapacity; }
	bool& get_flag_iterate() { return flag_iterate; }
	WellDoubletControl* get_wellDoubletControl()
	{ return wellDoubletControl; }

	void configure_wellDoubletControl(const char& selection);
				// is done at the begiining of each time step
        void initialize_temperatures();

	void calculate_temperatures(const double& Q_H, const double& Q_w);
						// solves advection equation
        void update_temperatures();
        void print_temperatures();
	void execute_timeStep(const double& Q_H, const double& value_target,
		const double& value_threshold, const bool& flag_print=true);
	void simulate(const char& wellDoubletControlScheme, const double& Q_H, 
		const double& value_target, const double& value_threshold, 
		const bool& flag_print=true);
		// values are passed to execute_timeStep(), they are constant
		// now but will be timestep-dependent in a real application
};

#endif
