
#include "fakeSimulator.h"
#include "wellDoubletControl.h"


int main(int argc, char* argv[])
{

	if(argc != 5)
	{
		std::cout << "Wrong number of arguments - gave " <<
		argc << ", but 4 are required: ";  
		std::cout << "Identifier [A, B, C], power rate, ";
		std::cout << "target value, and treshold value" << std::endl;

		return 1;
	} 
	else
	{
		FakeSimulator simulator;

		simulator.simulate(
			argv[1][0],  // wellDoubletControl scheme
			atof(argv[2]),  // Q_H
			atof(argv[3]),  // value_target
			atof(argv[4])  // value_threshold
		);
		// example: A 1e5 100 0.1
	}
	return 0;
}
