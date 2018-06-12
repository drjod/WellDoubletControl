#include "fakeSimulator.h"
#include <string>

#define LOG(x) std::cout << x << std::endl

void FakeSimulator::create_wellDoubletControl(const char& selection)
{
	if(wellDoubletControl != 0)
		delete wellDoubletControl;  // from last timestep
	wellDoubletControl = 
		WellDoubletControl::create_wellDoubletControl(selection);
}


void FakeSimulator::initialize_temperatures()
{
	LOG("\tinitialize simulation");
	for(int i=0; i<GRID_SIZE; i++)
	{
		temperatures_old[i] = WELL1_TEMPERATURE_INITIAL; 
		temperatures[i] = WELL1_TEMPERATURE_INITIAL; 
					// as input for WellDoubletControl
	}
	LOG(*this);
}

void FakeSimulator::calculate_temperatures(const double& Q_H, 
						const double& Q_w)
{
	LOG("\t\tcalculate ");

	// update inlet node
	temperatures[0] = temperatures_old[0];

	for(int i=1; i<GRID_SIZE; i++)
	{  // grid spacing is one meter (just 1 D - not radial)
		// calculate temperatures on [1, GRIDSIZE)
		// use always fabs(Q_w) > 0 (for injection and inextraction)
		// (temperature at well 2 is fixed)
		temperatures[i] = temperatures_old[i] +
			TIMESTEPSIZE * fabs(Q_w) * POROSITY *
			(temperatures_old[i-1] - temperatures_old[i]);
	}

	// add source term
	temperatures[WELL1_NODE_NUMBER] += TIMESTEPSIZE * Q_H / HEAT_CAPACITY;
}

void FakeSimulator::update_temperatures()
{
	for(int i=0; i<GRID_SIZE; i++)
		temperatures_old[i] = temperatures[i];
}

void FakeSimulator::execute_timeStep(
	const char& wellDoubletControlScheme, const double& Q_H, 
	const double& value_target, const double& value_threshold)
{
	for(int i=0; i<NUMBER_OF_ITERATIONS; i++)
	{
		LOG("\titeration " << i);
		calculate_temperatures(wellDoubletControl->get_result().Q_H,
					wellDoubletControl->get_result().Q_w);
		if(wellDoubletControl->evaluate_simulation_result(
			temperatures[WELL1_NODE_NUMBER], WELL2_TEMPERATURE,
			HEAT_CAPACITY, HEAT_CAPACITY)
				 == WellDoubletControl::converged)
			{ break; }
	}
}

void FakeSimulator::simulate(const char& wellDoubletControlScheme,
			const double& Q_H, const double& value_target,
			const double& value_threshold)
{
	initialize_temperatures();

	for(int i=0; i<NUMBER_OF_TIMESTEPS; i++)
	{       
		LOG("time step " << i);

		create_wellDoubletControl(wellDoubletControlScheme);
		wellDoubletControl->configure(
			"wellDoublet1", i, i*TIMESTEPSIZE,
			Q_H, value_target, value_threshold,
			temperatures[WELL1_NODE_NUMBER], WELL2_TEMPERATURE,
			HEAT_CAPACITY, HEAT_CAPACITY); 

		execute_timeStep(wellDoubletControlScheme,
				Q_H, value_target, value_threshold);
		
		LOG(*this);
		update_temperatures();
	}
}

std::ostream& operator<<(std::ostream& stream, const FakeSimulator& simulator)
{
	stream << "\t\tATES temperature: ";
        for(int i=0; i<GRID_SIZE; ++i)
                stream << simulator.temperatures[i] << " ";
        return stream;
}
