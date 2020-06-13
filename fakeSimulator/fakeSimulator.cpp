#include "fakeSimulator.h"
#include <string>
#include <fstream>
#include <ctime>
#include <iomanip>
#include "wdc_config.h"



void FakeSimulator::create_wellDoubletControl(const int& selection)
{
	if(wellDoubletControl != nullptr)
		delete wellDoubletControl;  // from last timestep
	wellDoubletControl = 
		wdc::WellDoubletControl::create_wellDoubletControl(selection, 10., // well_shutdown_temperature_range 
			{c_accuracy_temperature, c_accuracy_powerrate, c_accuracy_flowrate});
}


void FakeSimulator::initialize_temperatures()
{
	WDC_LOG("\tinitialize simulation");
	for(int i=0; i<c_gridSize; i++)
	{
		temperatures_previousTimestep[i] = c_temperature_storage_initial; 
		temperatures_previousIteration[i] = c_temperature_storage_initial; 
		temperatures[i] = c_temperature_storage_initial; 
					// as input for WellDoubletControl
	}
	WDC_LOG(*this);
}

void FakeSimulator::calculate_temperatures(const double& Q_H, 
						const double& Q_W)
{
	WDC_LOG("\t\tcalculate ");
	// update inlet node
	temperatures[0] = temperatures_previousTimestep[0];

	for(int i=1; i<c_gridSize; i++)
	{  // grid spacing is one meter (just 1 D - not radial)
		// calculate temperatures on [1, GRIDSIZE)
		// use always fabs(Q_W) > 0 (for injection and inextraction)
		// (temperature at well 2 is fixed)
		temperatures[i] = temperatures_previousTimestep[i] +
			c_timeStepSize * fabs(Q_W) * c_porosity *
			(temperatures_previousTimestep[i-1] - temperatures_previousTimestep[i]);
	}

	// add source term
	temperatures[c_heatExchanger_nodeNumber] += c_timeStepSize * Q_H / c_heatCapacity;
}

void FakeSimulator::update_temperatures()
{
	for(int i=0; i<c_gridSize; i++)
	{
		temperatures_previousTimestep[i] = temperatures[i];
		temperatures_previousIteration[i] = temperatures[i];
	}
}

void FakeSimulator::execute_timeStep(const double& well2_temperature)
{
	int i;
	for(i=0; i<c_maxNumberOfIterations; i++)
	{
		WDC_LOG("\titeration " << i);
		calculate_temperatures(wellDoubletControl->get_result().Q_H,
					wellDoubletControl->get_result().Q_W);
		wellDoubletControl->evaluate_simulation_result(
			{ temperatures[c_heatExchanger_nodeNumber], well2_temperature,
				c_heatCapacity, c_heatCapacity });
		if(i >= c_minNumberOfIterations-2 &&
			(calculate_error() < c_accuracy_temperature && 
			wellDoubletControl->converged()))
		{
			//std::cout << "break\n";
			break;
		}
	}
	log_file("\tIterations: " + std::to_string(i));
}

void FakeSimulator::simulate(const int& wellDoubletControlScheme,
			const double& Q_H, const double& value_target,
			const double& value_threshold)
{
	const double well2_temperature = (Q_H>0)? 
			c_temperature_upwindAquifer_storing : c_temperature_upwindAquifer_extracting;
 
	initialize_temperatures();

	std::fstream fout("timer.txt", std::ofstream::out | std::ios::app);
	Timer<std::fstream> timer(std::string(
			std::to_string(wellDoubletControlScheme) + " " +
			std::to_string(Q_H) + " " +
			std::to_string(value_target) + " " +
			std::to_string(value_threshold)).c_str(), fout);
	for(int i=0; i<c_numberOfTimeSteps; i++)
	{       
		WDC_LOG("time step " << i);
		log_file("Time step: " + std::to_string(i) + "\t" +
			std::to_string(wellDoubletControlScheme) + " " +
			std::to_string(Q_H) + " " +
			std::to_string(value_target) + " " +
			std::to_string(value_threshold));
		create_wellDoubletControl(wellDoubletControlScheme);
		wellDoubletControl->configure(
			Q_H, value_target, value_threshold,
			{ temperatures[c_heatExchanger_nodeNumber], well2_temperature,
			c_heatCapacity, c_heatCapacity }); 

		execute_timeStep(well2_temperature);
		
		WDC_LOG(*this);
		update_temperatures();
	}
}

double FakeSimulator::calculate_error()
{	// error is temperatures
	// error is powerrate and flowrate given by WDC
	double error = 0.;

	for(int i=0; i < c_gridSize; i++)
	{
		error = std::max(error,
			std::fabs(temperatures[i] - temperatures_previousIteration[i]));
		temperatures_previousIteration[i] = temperatures[i];
	}

	WDC_LOG("\t\terror: " << error);
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
        for(int i=0; i<c_gridSize; ++i)
                stream << simulator.temperatures[i] << " ";
        return stream;
}
