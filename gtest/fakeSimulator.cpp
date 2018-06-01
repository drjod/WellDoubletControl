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
	temperatures_old[0] = TEMPERATURE_2;
	for(int i=1; i<GRID_SIZE; i++)
		temperatures_old[i] = 10;
}

void FakeSimulator::calculate_temperatures(const double& Q_H, 
						const double& Q_w)
{
	LOG("\t\tcalculate ");
	
	temperatures[0] = TEMPERATURE_2;
	for(int i=1; i<GRID_SIZE; i++)
	{  // grid spacing is one meter (just 1 D - not radial)
		temperatures[i] = temperatures_old[i] +
			TIMESTEPSIZE * Q_w * POROSITY *
			(temperatures_old[i-1] - temperatures_old[i]);
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
				temperatures[NODE_NUMBER_T1], TEMPERATURE_2);

		flag_iterate = wellDoubletControl->evaluate_simulation_result();
		if(!flag_iterate) { LOG("\tconverged"); break; }
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

