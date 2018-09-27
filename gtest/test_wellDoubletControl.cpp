extern const double c_accuracy_temperature;
extern const double c_accuracy_flowrate;

#include "fakeSimulator.h"
#include "wellDoubletControl.h"

#define RELATIVE_POWERRATE_ERROR 1.e-2

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

	EXPECT_NEAR(std::get<4>(GetParam()), result.Q_H,
				RELATIVE_POWERRATE_ERROR * fabs(result.Q_H));
	EXPECT_NEAR(std::get<5>(GetParam()), result.Q_w,
				c_accuracy_flowrate*10);
	EXPECT_NEAR(std::get<6>(GetParam()), result.T_HE,
				c_accuracy_temperature*10);
	EXPECT_EQ(std::get<7>(GetParam()), 
				result.flag_powerrateAdapted);
}


INSTANTIATE_TEST_CASE_P(SCHEMES, WellDoubletTest, testing::Values(
	// input: scheme, Q_H, value_target, value_threshold
	// output: Q_H, Q_w, T1, flag_powerrateAdapted

	// Scheme 1 - storing
	std::make_tuple(1, 1.e5, 100., 0.01, // T_HE_target, Q_w_max
			1.e5, 0.0, 70., false),  // target T1 not reached although flow rate is zero
	std::make_tuple(1, 1.e6, 100., 0.01, // T_HE_target, Q_w_max
			1.e6, 0.008, 100., false),  // target T1 reached by adapting flow rate
	std::make_tuple(1, 2.e6, 100., 0.01, // T_HE_target, Q_w_max
			1.25e6, 0.01, 100., true),  // target T1 reached by adapting power rate
	// Scheme 1 - extracting
	std::make_tuple(1, -1.e5, 25., -0.01, // T_HE_target, Q_w_min
			-1.e5, 0., 30.15, false),  // target T1 not reached although flow rate is at threshold
	std::make_tuple(1, -5.e5, 25., -0.01, // T_HE_target, Q_w_min
			-5.e5, -0.008, 25., false),  // target T1 reached by adapting flow rate
	std::make_tuple(1, -1.e6, 25., -0.01, // T_HE_target, Q_w_min
			-6.245e5, -0.01, 25., true),  // target T1 reached by adapting power rate
	////// Scheme 0 - storing
	std::make_tuple(0, 1.e6, 0.01, 100., // Q_w_target, T_HE_max
			1.e6, 0.01, 89.961, false),  // has not reached threshold
	std::make_tuple(0, 1.e6, 0.01, 80., // Q_w_target, T_HE_max
			7.5e5, 0.01, 80., true),  // has reached threshold
	//// Scheme 0 - extracting
	std::make_tuple(0, -1.e5, -0.01, 30., // Q_w_target, T_HE_min
			-1.e5, -0.01, 46., false),  // has not reached threshold
	std::make_tuple(0, -1.e6, -0.01, 30., // Q_w_target, T_HE_min
			-5.e5, -0.01, 30., true)  // has not reached threshold
/*	//// Scheme 2 - storing (same examples as for scheme A, and should give same results)
	std::make_tuple(2, 1.e5, 90., 0.01, // DT_target, Q_w_max
			1.e5, 0.0, 70., false),  // target DT not reached although flow rate is zero
	std::make_tuple(2, 1.e6, 90., 0.01, // DT_target, Q_w_max
			1.e6, 0.008, 100., false),  // target DT reached by adapting flow rate
	std::make_tuple(2, 2.e6, 90., 0.01,  // DT_target, Q_w_max
			1.25e6, 0.01, 100., true),  // target DT reached by adapting power rate
	// Scheme 2 - extracting
	std::make_tuple(2, -1.e5, 10., -0.01, // DT_target, Q_w_min
			-1.e5, 0., 30., false),  // target DT not reached although flow rate is at threshold
	std::make_tuple(2, -5.e5, 10., -0.01,  // DT_target, Q_w_min
			-5.e5, -0.00666, 20., false),  // target DT not reached by adapting flow rate
	std::make_tuple(2, -1.e6, 10., -0.01,  // DT_target, Q_w_min
			-7.5e5, -0.01, 20., true)  // target DT reached by adapting flow rate
*/
));

