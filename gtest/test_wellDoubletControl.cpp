
#include "fakeSimulator.h"
#include "wellDoubletControl.h"

/*
TEST(WellDoublet, Test)
{

	FakeSimulator simulator;

	simulator.simulate('B', 1.e6, 0.01, 48., false);  // wellDoubletControl scheme, 
					// Q_H, value_target, value_threshold

	WellDoubletCalculation result = simulator.get_wellDoubletControl()->get_result();

	EXPECT_TRUE(true);
}
*/

class WellDoubletTest : public ::testing::TestWithParam<std::tuple<
	char, double, double, double, double, double, double, bool> > {};


TEST_P(WellDoubletTest, storage_simulation_with_ten_time_steps)
{

	FakeSimulator simulator;

	simulator.simulate(std::get<0>(GetParam()),  // wellDoubletControl scheme
				std::get<1>(GetParam()),  // Q_H
				std::get<2>(GetParam()),  // value_target
				std::get<3>(GetParam()));  // value_threshold

	WellDoubletControl::result_t result = simulator.get_wellDoubletControl()->get_result();

	EXPECT_NEAR(std::get<4>(GetParam()), result.Q_H, 1.e-3 * fabs(result.Q_H));
	EXPECT_NEAR(std::get<5>(GetParam()), result.Q_w, 1.e-3 * fabs(result.Q_w));
	EXPECT_NEAR(std::get<6>(GetParam()), result.T1, 1.e-3 * fabs(result.T1));
	EXPECT_EQ(std::get<7>(GetParam()), result.flag_powerrateAdapted);

}


INSTANTIATE_TEST_CASE_P(SCHEMES, WellDoubletTest, testing::Values(
	// input: scheme, Q_H, value_target, value_threshold
	// output: Q_H, Q_w, T1, flag_powerrateAdapted

	// Scheme A - storing
	std::make_tuple('A', 1.e5, 60., 0.01, // T1_target, Q_w_max
			1.e5, 0.0, 35., false),  // target T1 not reached although flow rate is zero
	std::make_tuple('A', 1.e6, 60., 0.01, // T1_target, Q_w_max
			1.e6, 0.00888, 60., false),  // target T1 reached by adapting flow rate
	std::make_tuple('A', 2.e6, 60., 0.01, // T1_target, Q_w_max
			1.17339e6, 0.01, 60., true),  // target T1 reached by adapting power rate
	// Scheme A - extracting
//	std::make_tuple('A', -1.e5, -10, -0.01, // T1_target, Q_w_min
//			-1.e5, -0.0, -5, false),  // target T1 not reached although flow rate is at threshold
	//std::make_tuple('A', -1.e5, 6., -0.01, // T1_target, Q_w_min
	//		-1.e5, -0.006531, 6., false),  // target T1 not reached by adapting flow rate
//	std::make_tuple('A', -1.e6, -0., -0.01, // T1_target, Q_w_min
//			-2.591e5, -0.01, 0., true),  // target T1 reached by adapting flow rate
	////// Scheme B - storing
	std::make_tuple('B', 1.e6, 0.01, 60., // Q_w_target, T1_max
			1.e6, 0.01, 53.11, false),  // has not reached threshold
	std::make_tuple('B', 1.e6, 0.01, 50., // Q_w_target, T1_max
			9.4565e5, 0.01, 50., true),  // has reached threshold
	//// Scheme B - extracting
//	std::make_tuple('B', -1.e5, -0.01, 5., // Q_w_target, T1_min
//			-1.e5, -0.01, 6.619, false),  // has not reached threshold
//	std::make_tuple('B', -1.e5, -0.01, 10., // Q_w_target, T1_min
//			-9130, -0.01, 10., true),  // has not reached threshold
	//// Scheme C - storing (same examples as for scheme A, and should give same results)
	std::make_tuple('C', 1.e5, 50., 0.01, // DT_target, Q_w_max
			1.e5, 0.0, 35., false),  // target T1 not reached although flow rate is zero
	std::make_tuple('C', 1.e6, 50., 0.01, // DT_target, Q_w_max
			1.e6, 0.00891, 60., false),  // target T1 reached by adapting flow rate
	std::make_tuple('C', 2.e6, 50., 0.01, // DT_target, Q_w_max
			1.173e6, 0.01, 60., true)// target T1 reached by adapting power rate
	// Scheme C - extracting
	//std::make_tuple('C', -1.e5, 0, -0.01, // T1_target, Q_w_min
	//		-1.e5, -0.0, -5, false)  // target T1 not reached although flow rate is at threshold
	//std::make_tuple('A', -1.e5, 6., -0.01, // T1_target, Q_w_min
	//		-1.e5, -0.006531, 6., false),  // target T1 not reached by adapting flow rate
	//std::make_tuple('C', -1.e6, -0., -0.01, // T1_target, Q_w_min
	//		-2.591e5, -0.01, 0., true),  // target T1 reached by adapting flow rate

));



/*
class WellDoubletTest : public ::testing::TestWithParam<std::tuple<
	char, double, double, double, double, double, double, bool, bool> > {};


TEST_P(WellDoubletTest, WellDoubletControl)
{

	Simulator simulator = Simulator(
		WellDoubletControl::createWellDoubletControl(
				std::get<0>(GetParam())));
	
	simulator.set_temperature(TEMPERATURE_1,  // T1_init
				TEMPERATURE_2);  // T2_init

	WellDoubletCalculation result = simulator.execute_timeStep(
		std::get<1>(GetParam()),  // Q_H
		std::get<2>(GetParam()),  // value_target
		std::get<3>(GetParam()),  // value_threshold 
		std::get<4>(GetParam()),  // T1_new
		TEMPERATURE_2);  // T2_new

	EXPECT_NEAR(std::get<5>(GetParam()), result.Q_H, 1.e-3 * fabs(result.Q_H));
	EXPECT_NEAR(std::get<6>(GetParam()), result.Q_w, 1.e-3 * fabs(result.Q_w));
	EXPECT_EQ(std::get<7>(GetParam()), result.flag_powerrateAdapted);
	EXPECT_EQ(std::get<8>(GetParam()), simulator.get_flag_iterate());

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



*/
