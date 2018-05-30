#include "wellDoubletControl.cpp"

// for T_1 it is initial temperature, T_2 is kept constant
#define TEMPERATURE_1 60
#define TEMPERATURE_2 10


class Simulator
{
	double T1, T2;
	WellDoubletControl* wellDoubletControl;
	bool flag_iterate;
public:
	Simulator() {}
	Simulator(WellDoubletControl* _wellDoubletControl) : wellDoubletControl(_wellDoubletControl)
	{ /* set accuracy (epsilon) and fluid thermal capacity */ }

	void set_temperature(const double& _T1, const double& _T2)
	{ T1 = _T1; T2 = _T2; }

	bool get_flag_iterate()
	{
		return flag_iterate;
	}

	WellDoubletCalculation execute_timeStep(double Q_H, double value_target, 
			double value_threshold, double T1_new, double T2_new)
	{
		wellDoubletControl->set_iterationValues(T1, T2);
		wellDoubletControl->set_timeStepValues(Q_H,
				value_target, value_threshold);

		//do  // iterate
		{
			wellDoubletControl->calculate_flowrate();
			set_temperature(T1_new, T2_new);			
			wellDoubletControl->set_iterationValues(T1, T2);
		}
		//while(
		flag_iterate = wellDoubletControl->check_result();
		//);

		return wellDoubletControl->get_result();
	}
};


class WellDoubletTest : public ::testing::TestWithParam<std::tr1::tuple<
	char, double, double, double, double, double, double, bool, bool> > {};


TEST_P(WellDoubletTest, WellDoubletControl)
{

	Simulator simulator = Simulator(
		WellDoubletControl::createWellDoubletControl(
				std::tr1::get<0>(GetParam())));
	
	simulator.set_temperature(TEMPERATURE_1,  // T1_init
				TEMPERATURE_2);  // T2_init

	WellDoubletCalculation result = simulator.execute_timeStep(
		std::tr1::get<1>(GetParam()),  // Q_H
		std::tr1::get<2>(GetParam()),  // value_target
		std::tr1::get<3>(GetParam()),  // value_threshold 
		std::tr1::get<4>(GetParam()),  // T1_new
		TEMPERATURE_2);  // T2_new

	EXPECT_NEAR(std::tr1::get<5>(GetParam()), result.Q_H, 1.e-3 * fabs(result.Q_H));
	EXPECT_NEAR(std::tr1::get<6>(GetParam()), result.Q_w, 1.e-3 * fabs(result.Q_w));
	EXPECT_EQ(std::tr1::get<7>(GetParam()), result.flag_powerrateAdapted);
	EXPECT_EQ(std::tr1::get<8>(GetParam()), simulator.get_flag_iterate());

//std::cout << result.Q_H << std::endl;
//EXPECT_TRUE(true);
}


INSTANTIATE_TEST_CASE_P(SCHEMES, WellDoubletTest, testing::Values(
	// scheme, Q_H,
	// value_target, value_threshold,
	// T1_new (given by Simulator, initially TEMPERATURE_1)
	// Q_H, Q_w, flag_powerrateAdapted (results), flag_iterate

//	std::make_tuple('A', 1e6)
	// Scheme A - storing
	std::make_tuple('A', 1.e6,
			70., 0.01, // T1_target, Q_w_max
			70., // at target
			1.e6, 0.00333333, false, false),  // does not reach threshold
	std::make_tuple('A', 1e6,
			70, 0.01, // T1_target, Q_w_max
			90,  // beyond target
			6.666667e5, 0.00333333, true, true),  // does not reach threshold
	std::make_tuple('A', 1e6,
			70, 0.01, // T1_target, Q_w_max
			60,  // does not reach target
			1.e6, 0.00333333, false, true),  // does not reach threshold
//	std::make_tuple('A', 1e6,
//			70, 0.01, // T1_target, Q_w_max
//			90,  // passed threshold
//			1333333.33333333, 0.00333333, true),
//	std::make_tuple('A', 1e6,
//			70, 0.01, // T1_target, Q_w_max
//			70,  // exactly as threshold
//			1e6, 0.00333333, false),
//	// Scheme A - extracting
//	std::make_tuple('A', -1e6,
//			30, -0.01, // T1_target, Q_w_min
//			40,  // does not reach threshold
//			-1e6, -0.01, false),
//	std::make_tuple('A', -1e6,
//			30, -0.01, // T1_target, Q_w_min
//			20,  // passed threshold
//			-5e5, -0.01, true),
//	std::make_tuple('A', -1e6,
//			30, -0.01, // T1_target, Q_w_min
//			30,  // exactly as threshold
//			-1e6, -0.01, false),
//
//	// Scheme B - storing
	std::make_tuple('B', 1e6,
			0.01, 70., // Q_w_target, T_1_max
			60.,  // does not reach threshold
			1e6, 0.01, false, false),
	std::make_tuple('B', 1e6,
			0.001, 70., // Q_w_target, T_1_max
			80.,  // passed threshold
			3e5, 0.001, true, true),
	std::make_tuple('B', 1e6,
			0.001, 70., // Q_w_target, T_1_max
			70.,  // exactly as threshold
			1e6, 0.001, false, false),
	// Scheme B - extracting
	std::make_tuple('B', -1e6,
			-0.01, 30., // Q_w_target, T_1_min
			60.,  // does not reach threshold
			-1e6, -0.01, false, false),
	std::make_tuple('B', -1e6,
			-0.001, 30., // Q_w_target, T_1_min
			20.,  // passed threshold
			-1e5, -0.001, true, true),
	std::make_tuple('B', -1e6,
			-0.001, 30., // Q_w_target, T_1_min
			30.,  // exactly as threshold
			-1e6, -0.001, false, false),
	std::make_tuple('B', 1e6,
			0.01, 70., // Q_w_target, T_1_max
			60.,  // does not reach threshold
			1e6, 0.01, false, false),

//	// Scheme C - storing
	std::make_tuple('C', 1e6,
			50., 70., // DT_target, T_1_max
			60.,  // does not reach threshold
			1e6, 0.004, false, false),
	std::make_tuple('C', 1e6,
			50, 70, // DT_target, T_1_max
			80,  // passed threshold
			1.2e6, 0.004, true, true), 

	std::make_tuple('C', 1e6,
			50, 70, // DT_target, T_1_max
			70,  // exactly as threshold
			1e6, 0.004, false, false)
//	//// Scheme C - extracting				value of T_2 ?????
//	//std::make_tuple('C', -1e6,
//	//		50, 30, // DT_target, T_1_min
//	//		60,  // does not reach threshold
//	//		-1e6, -0.01, false),
//	//std::make_tuple('C', -1e6,
//	//		50, 30, // DT_target, T_1_min
//	//		20,  // passed threshold
//	//		-1e5, -0.001, true),
//	//std::make_tuple('C', -1e6,
//	//		50, 30, // DT_target, T_1_min
//	//		30,  // exactly as threshold
//	//		-1e6, -0.001, false)
));

