#include "fakeSimulator.h"
#include <string>
#include <fstream>
#include <ctime>
#include <iomanip>
#include "wdc_config.h"


void FakeSimulator::create_wellDoubletControl(const char& selection)
{
	if(wellDoubletControl != nullptr)
		delete wellDoubletControl;  // from last timestep
	wellDoubletControl = 
		WellDoubletControl::create_wellDoubletControl(selection);
}


void FakeSimulator::initialize_temperatures()
{
	LOG("\tinitialize simulation");
	for(int i=0; i<GRID_SIZE; i++)
	{
		temperatures_previousTimestep[i] = WELL1_TEMPERATURE_INITIAL; 
		temperatures_previousIteration[i] = WELL1_TEMPERATURE_INITIAL; 
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
	temperatures[0] = temperatures_previousTimestep[0];

	for(int i=1; i<GRID_SIZE; i++)
	{  // grid spacing is one meter (just 1 D - not radial)
		// calculate temperatures on [1, GRIDSIZE)
		// use always fabs(Q_w) > 0 (for injection and inextraction)
		// (temperature at well 2 is fixed)
		temperatures[i] = temperatures_previousTimestep[i] +
			TIMESTEPSIZE * fabs(Q_w) * POROSITY *
			(temperatures_previousTimestep[i-1] - temperatures_previousTimestep[i]);
	}

	// add source term
	temperatures[WELL1_NODE_NUMBER] += TIMESTEPSIZE * Q_H / HEAT_CAPACITY;
}

void FakeSimulator::update_temperatures()
{
	for(int i=0; i<GRID_SIZE; i++)
	{
		temperatures_previousTimestep[i] = temperatures[i];
		temperatures_previousIteration[i] = temperatures[i];
	}
}

void FakeSimulator::execute_timeStep(
	const char& wellDoubletControlScheme, const double& Q_H, 
	const double& value_target, const double& value_threshold)
{
	int i;
	for(i=0; i<MAX_NUMBER_OF_ITERATIONS; i++)
	{
		LOG("\titeration " << i);
		calculate_temperatures(wellDoubletControl->get_result().Q_H,
					wellDoubletControl->get_result().Q_w);
		wellDoubletControl->evaluate_simulation_result(
			temperatures[WELL1_NODE_NUMBER], WELL2_TEMPERATURE,
			HEAT_CAPACITY, HEAT_CAPACITY);
		
		if(i > MIN_NUMBER_OF_ITERATIONS && (calculate_error() < ACCURACY ||
			wellDoubletControl->converged(temperatures[WELL1_NODE_NUMBER], ACCURACY)))
				break;
	}
	log_file("\tIterations: " + std::to_string(i));
}

void FakeSimulator::simulate(const char& wellDoubletControlScheme,
			const double& Q_H, const double& value_target,
			const double& value_threshold)
{
	initialize_temperatures();

	for(int i=0; i<NUMBER_OF_TIMESTEPS; i++)
	{       
		LOG("time step " << i);
		log_file("Time step: " + std::to_string(i) + "\t" +
			wellDoubletControlScheme + " " +
			std::to_string(Q_H) + " " +
			std::to_string(value_target) + " " +
			std::to_string(value_threshold));
		create_wellDoubletControl(wellDoubletControlScheme);
		wellDoubletControl->configure(
			Q_H, value_target, value_threshold,
			temperatures[WELL1_NODE_NUMBER], WELL2_TEMPERATURE,
			HEAT_CAPACITY, HEAT_CAPACITY); 

		execute_timeStep(wellDoubletControlScheme,
				Q_H, value_target, value_threshold);
		
		LOG(*this);
		update_temperatures();
	}
}

double FakeSimulator::calculate_error()
{
	double error = 0.;

	for(int i=0; i < GRID_SIZE; i++)
	{
		error = std::max(error,
			std::fabs(temperatures[i] - temperatures_previousIteration[i]));
		temperatures_previousIteration[i] = temperatures[i];
	}

	LOG("\t\terror: " << error);
	return error;
}

template <typename T>
void FakeSimulator::log_file(T toLog)
{
	auto now = std::time(nullptr);
	//stream.imbue(std::locale)
	std::ofstream stream("logging.txt", std::ios::app);
	stream << std::put_time(std::localtime(&now), "%c") << ": "  << toLog << std::endl;

}

std::ostream& operator<<(std::ostream& stream, const FakeSimulator& simulator)
{
	stream << "\t\tATES temperature: ";
        for(int i=0; i<GRID_SIZE; ++i)
                stream << simulator.temperatures[i] << " ";
        return stream;
}
