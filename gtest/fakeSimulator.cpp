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

void FakeSimulator::print_temperatures()
{
	std::cout << "\t\t\t\t";
	for(int i=0; i<GRID_SIZE; i++)
		std::cout << temperatures[i] << " ";
	std::cout << std::endl;
}

void FakeSimulator::execute_timeStep(const double& Q_H,
		const double& value_target, const double& value_threshold, 
		const bool& flag_print)
{
	wellDoubletControl->set_constraints(Q_H,
			value_target, value_threshold);
LOG(Q_H);
		wellDoubletControl->calculate_flowrate();
	for(int i=0; i<NUMBER_OF_ITERATIONS; i++)
	{
		LOG("\titeration " + std::to_string(i));
		//wellDoubletControl->calculate_flowrate();

		calculate_temperatures(wellDoubletControl->get_Q_H(),
					wellDoubletControl->get_Q_w());
		wellDoubletControl->set_temperatures(
				temperatures[NODE_NUMBER_T1], TEMPERATURE_2);

		if(flag_print) wellDoubletControl->print_temperatures();

		flag_iterate = wellDoubletControl->check_result();
		if(!flag_iterate) { LOG("\t\t\tconverged"); break; }
	}


}

void FakeSimulator::simulate(const char& wellDoubletControlScheme,
			const double& Q_H, const double& value_target,
			const double& value_threshold, const bool& flag_print)
{
	initialize_temperatures();

	for(int i=0; i<NUMBER_OF_TIMESTEPS; i++)
	{       
		configure_wellDoubletControl(wellDoubletControlScheme);
		LOG("time step " + std::to_string(i));
		execute_timeStep(Q_H, value_target, value_threshold,
				flag_print);
		if(flag_print)
			print_temperatures();
		update_temperatures();
	}
}
