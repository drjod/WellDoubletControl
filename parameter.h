#ifndef PARAMETER_H
#define PARAMETER_H


//#define LOG(x)
#define LOG(x) std::cout << x << std::endl 

// Parameters which are constant throughout testing:
#define TEMPERATURE_2 10
#define HEATCAPACITY 5.e6
#define POROSITY 0.5

#define GRID_SIZE 10
// where warm well 1 is on the grid:
#define NODE_NUMBER_T1 5

// numerics
#define NUMBER_OF_ITERATIONS 15
#define NUMBER_OF_TIMESTEPS 5
#define TIMESTEPSIZE 1.e2


// accuracy in target calculations
#define EPSILON 1.e-2

#endif
