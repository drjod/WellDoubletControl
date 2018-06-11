#ifndef FAKESIMULATOR_H
#define FAKESIMULATOR_H

#include <string>
#include "wellDoubletControl.h"
#include "parameter.h"


class WellDoubletControl;

struct Simulator
{
	virtual ~Simulator() {}
	
	virtual const bool& get_flag_iterate() const = 0;
	virtual const WellDoubletControl* get_wellDoubletControl() const = 0;
	virtual void create_wellDoubletControl(const char& selection) = 0;

        virtual void initialize_temperatures() = 0;
	virtual void calculate_temperatures(
				const double& Q_H, const double& Q_w) = 0;
        virtual void update_temperatures() = 0;

	virtual void execute_timeStep(
		const char& wellDoubletControlScheme, const double& Q_H, 
		const double& value_target, const double& value_threshold) = 0;
	virtual void simulate(
		const char& wellDoubletControlScheme, const double& Q_H, 
		const double& value_target, const double& value_threshold) = 0;
};


class FakeSimulator : public Simulator
{
        double temperatures[GRID_SIZE];  //  temperatures at warm well 
				// (distance increases with node number) 
        double temperatures_old[GRID_SIZE];

        WellDoubletControl* wellDoubletControl;
        bool flag_iterate;  // to convert threshold value into a target value

public:
	FakeSimulator() : wellDoubletControl(0) {}
	~FakeSimulator() 
	{ if(wellDoubletControl != 0) delete wellDoubletControl; }
			// a wellDoubletControl instance is constructed 
			// (and destructed) each time step

	const bool& get_flag_iterate() const { return flag_iterate; }
	const WellDoubletControl* get_wellDoubletControl() const
	{ return wellDoubletControl; }
	void create_wellDoubletControl(const char& selection);
				// is done at the begiining of each time step

        void initialize_temperatures();
	void calculate_temperatures(const double& Q_H, const double& Q_w);
						// solves advection equation
        void update_temperatures();

	void execute_timeStep(const char& wellDoubletControlScheme, const double& Q_H, 
		const double& value_target, const double& value_threshold);
	void simulate(const char& wellDoubletControlScheme, const double& Q_H, 
		const double& value_target, const double& value_threshold);
		// values are passed to execute_timeStep(), they are constant
		// now but will be timestep-dependent in a real application

	friend std::ostream& operator<<(std::ostream& stream,
					const FakeSimulator& simulator);
};


#endif
