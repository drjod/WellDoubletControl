#ifndef FAKESIMULATOR_H
#define FAKESIMULATOR_H

// T_1 is initial temperature (warm well 1), T_2 is kept constant (cold well 2)
#define TEMPERATURE_1 60
#define TEMPERATURE_2 10
#define GRID_SIZE 10
#define NODE_T1 5
#define NUMBER_OF_ITERATIONS 10
#define NUMBER_OF_TIMESTEPS 10


class FakeSimulator
{
        double temperatures[GRID_SIZE];
        double temperatures_old[GRID_SIZE];
        WellDoubletControl* wellDoubletControl;
        bool flag_iterate;
public:
        FakeSimulator() {}
        FakeSimulator(WellDoubletControl* _wellDoubletControl) : wellDoubletControl(_wellDoubletControl)
        { /* set accuracy (epsilon) and fluid thermal capacity */ }
        ~FakeSimulator()
        {
                delete wellDoubletControl;
        }

        void initialize_temperatures()
        {
                temperatures_old[0] = TEMPERATURE_2;
                for(int i=1; i<GRID_SIZE; i++)
                        temperatures_old[i] = 10;
        }

	void calculate_temperatures(double Q_H, double Q_w)
        {
                std::cout << "     	  calculate " << std::endl;
                double dt = 1.e2 ;
                temperatures[0] = TEMPERATURE_2;
                for(int i=1; i<GRID_SIZE; i++)
                {
                        temperatures[i] = temperatures_old[i] +
				dt * Q_w * (temperatures_old[i-1] - temperatures_old[i])/2;  // phi = 0.5
                }
                temperatures[NODE_T1] += dt * Q_H / 5.e6 ; // capacity

        }

        void update_temperatures()
        {
                for(int i=0; i<GRID_SIZE; i++)
                        temperatures_old[i] = temperatures[i];
        }

        void print_temperatures()
        {
                std::cout << "                  ";
                for(int i=0; i<GRID_SIZE; i++)
                        std::cout << temperatures[i] << " ";
                std::cout << std::endl;
        }

        bool get_flag_iterate()
        {
                return flag_iterate;
        }

	void execute_timeStep(double Q_H, double value_target, double value_threshold)
        {
                wellDoubletControl->set_constraints(Q_H,
                                value_target, value_threshold);

                for(int i=0; i<NUMBER_OF_ITERATIONS; i++)
                {
                        std::cout << "  iteration " << i << std::endl;
                        wellDoubletControl->calculate_flowrate();

                        calculate_temperatures(wellDoubletControl->get_Q_H(),
                                                wellDoubletControl->get_Q_w());
                        wellDoubletControl->set_temperatures(temperatures[NODE_T1], TEMPERATURE_2);
                        wellDoubletControl->print_temperatures();

                        flag_iterate = wellDoubletControl->check_result();
                        if(!flag_iterate)
                        {
                                std::cout << "          	converged" << std::endl;
                                break;
                        }
                }


        }

	void simulate(double Q_H, double value_target, double value_threshold, bool flag_print=true)
	{
		initialize_temperatures();

		for(int i=0; i<NUMBER_OF_TIMESTEPS; i++)
		{       
			std::cout << "time step " << i << std::endl;
			execute_timeStep(Q_H, value_target, value_threshold);
			if(flag_print)
				print_temperatures();
			update_temperatures();
		}


	}

	WellDoubletCalculation& get_wellDoubletCalculationResult()
	{
                return wellDoubletControl->get_result();

	}
};


#endif
