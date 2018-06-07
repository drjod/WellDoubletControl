#include "fakeSimulator.h"
#include <string>


void FakeSimulator::configure_wellDoubletControl(const char& selection)
{
	if(wellDoubletControl != 0)
		delete wellDoubletControl;  // from last timestep
	wellDoubletControl = WellDoubletControl::createWellDoubletControl(
							selection, this);
}


void FakeSimulator::initialize_temperatures()
{
	LOG("\t\tset initialize ATES temperature");
	// temperature decreases linearly
	temperatures_old[NODE_NUMBER_T2] = INITIAL_TEMPERATURE_LOW;  // cold weöö
	for(int i=1; i<GRID_SIZE; i++)
	{
		//temperatures_old[i] = INITIAL_TEMPERATURE_LOW; 
		temperatures_old[i] = INITIAL_TEMPERATURE_HIGH +
			(INITIAL_TEMPERATURE_LOW - INITIAL_TEMPERATURE_HIGH) * i / (GRID_SIZE-1);
	}
	for(int i=0; i<GRID_SIZE; i++)
		std::cout << temperatures_old[i] << " ";
}

void FakeSimulator::calculate_temperatures(const double& Q_H, 
						const double& Q_w)
{
	LOG("\t\tcalculate ");
	int ii, upwind_node;

	// update inlet node
	if(Q_w>0)
		temperatures[0] = temperatures_old[0];
	else
		temperatures[GRID_SIZE-1] = temperatures_old[GRID_SIZE-1];

	for(int i=1; i<GRID_SIZE; i++)
	{  // grid spacing is one meter (just 1 D - not radial)
		// calculate temperatures on [1, GRIDSIZE) for Q_w > 0 (injection)
		// and on [0, GRIDSIZE-1) for Q_w < 0 (extraction)
		if(Q_w >0) { ii=i; upwind_node=ii-1; }
		else { ii=i-1; upwind_node = ii+1; }
		temperatures[ii] = temperatures_old[ii] +
			TIMESTEPSIZE * fabs(Q_w) * POROSITY *
			(temperatures_old[upwind_node] - temperatures_old[ii]);
	}

	temperatures[NODE_NUMBER_T1] += TIMESTEPSIZE * Q_H / heatcapacity;
}

void FakeSimulator::update_temperatures()
{
	for(int i=0; i<GRID_SIZE; i++)
		temperatures_old[i] = temperatures[i];
}

void FakeSimulator::execute_timeStep(const double& Q_H,
		const double& value_target, const double& value_threshold)
{
	wellDoubletControl->set_constraints(Q_H,
			value_target, value_threshold);
	wellDoubletControl->provide_flowrate();  
		// an estimation in scheme A and a target value in scheme B

	for(int i=0; i<NUMBER_OF_ITERATIONS; i++)
	{
		LOG("\titeration " + std::to_string(i));

		calculate_temperatures(wellDoubletControl->get_result().powerrate(),
					wellDoubletControl->get_result().flowrate());
		wellDoubletControl->set_temperatures(
				temperatures[NODE_NUMBER_T1], INITIAL_TEMPERATURE_LOW);//temperatures[NODE_NUMBER_T2]);

		wellDoubletControl->evaluate_simulation_result();

		if(wellDoubletControl->get_iterationState() == WellDoubletControl::converged)
			{ LOG("\tconverged"); break; }
	}


}

void FakeSimulator::simulate(const char& wellDoubletControlScheme,
			const double& Q_H, const double& value_target,
			const double& value_threshold)
{
	initialize_temperatures();

	for(int i=0; i<NUMBER_OF_TIMESTEPS; i++)
	{       
		configure_wellDoubletControl(wellDoubletControlScheme);
		LOG("time step " + std::to_string(i));
		execute_timeStep(Q_H, value_target, value_threshold);
		
		LOG(*this);
		update_temperatures();
	}
}


std::ostream& operator<<(std::ostream& stream, const FakeSimulator& simulator)
{
	stream << "\t\tATES temperatures: ";
        for(int i=0; i<GRID_SIZE; ++i)
                stream << simulator.temperatures[i] << " ";
        return stream;
}

