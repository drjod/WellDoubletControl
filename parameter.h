#ifndef PARAMETER_H
#define PARAMETER_H


//#define LOG(x)
#define LOG(x) std::cout << x << std::endl 

// Parameters which are constant throughout testing:
#define WELL1_TEMPERATURE_INITIAL 50
#define WELL2_TEMPERATURE 10

#define HEATCAPACITY 5.e6
#define POROSITY 0.5

#define GRID_SIZE 11
#define WELL1_NODE_NUMBER 5

// numerics
#define NUMBER_OF_ITERATIONS 10
#define NUMBER_OF_TIMESTEPS 10
#define TIMESTEPSIZE 1.e2

#endif
