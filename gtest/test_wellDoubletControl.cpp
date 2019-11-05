#include "fakeSimulator.h"
#include "wellDoubletControl.h"

const double relative_powerrate_error = 1.e-2;

class WellDoubletTest : public ::testing::TestWithParam<std::tuple<
	char, double, double, double, double, double, double, wdc::WellDoubletControl::storage_state_t> > {};


TEST_P(WellDoubletTest, storage_simulation_with_ten_time_steps)
{
	FakeSimulator simulator;

	simulator.simulate(std::get<0>(GetParam()),  // wellDoubletControl scheme
				std::get<1>(GetParam()),  // Q_H
				std::get<2>(GetParam()),  // value_target
				std::get<3>(GetParam()));  // value_threshold

	wdc::WellDoubletControl::result_t result = simulator.get_wellDoubletControl()->get_result();

	EXPECT_NEAR(std::get<4>(GetParam()), result.Q_H,
				relative_powerrate_error * fabs(result.Q_H));
	EXPECT_NEAR(std::get<5>(GetParam()), result.Q_W,
				simulator.get_wellDoubletControl()->get_accuracies().flowrate*10);
	EXPECT_NEAR(std::get<6>(GetParam()), result.T_HE,
				simulator.get_wellDoubletControl()->get_accuracies().temperature*10);
	EXPECT_EQ(std::get<7>(GetParam()), 
				result.storage_state);

}


INSTANTIATE_TEST_CASE_P(SCHEMES, WellDoubletTest, testing::Values(
	// input: scheme, Q_H, value_target, value_threshold
	// output: Q_H, Q_w, T1, flag_powerrateAdapted

	////// Scheme 0 - storing
	std::make_tuple(0, 1.e6, 0.01, 100., // Q_w_target, T_HE_max
			1.e6, 0.01, 89.961, wdc::WellDoubletControl::on_demand),  // has not reached threshold
	std::make_tuple(0, 1.e6, 0.01, 80., // Q_w_target, T_HE_max
			7.5e5, 0.01, 80., wdc::WellDoubletControl::powerrate_to_adapt),  // has reached threshold
	//// Scheme 0 - extracting
	std::make_tuple(0, -1.e5, -0.01, 30., // Q_w_target, T_HE_min
			-1.e5, -0.01, 46., wdc::WellDoubletControl::on_demand),  // has not reached threshold
	std::make_tuple(0, -1.e6, -0.01, 30., // Q_w_target, T_HE_min
			-5.e5, -0.01, 30., wdc::WellDoubletControl::powerrate_to_adapt),  // has not reached threshold
	// Scheme 1 - storing
	std::make_tuple(1, 1.e5, 100., 0.01, // T_HE_target, Q_w_max
			1.e5, 0.0, 70., wdc::WellDoubletControl::target_not_achievable),  // target T1 not reached although flow rate is zero
	std::make_tuple(1, 1.e6, 100., 0.01, // T_HE_target, Q_w_max
			1.e6, 0.008, 100., wdc::WellDoubletControl::on_demand),  // target T1 reached by adapting flow rate
	std::make_tuple(1, 2.e6, 100., 0.01, // T_HE_target, Q_w_max
			1.25e6, 0.01, 100., wdc::WellDoubletControl::powerrate_to_adapt),  // target T1 reached by adapting power rate

	// Scheme 1 - extracting
	std::make_tuple(1, -1.e5, 25., -0.01, // T_HE_target, Q_w_min
			-1.e5, 0., 30., wdc::WellDoubletControl::target_not_achievable),  // target T1 not reached although flow rate is at threshold
	std::make_tuple(1, -5.e5, 25., -0.01, // T_HE_target, Q_w_min
			-5.e5, -0.008, 25., wdc::WellDoubletControl::on_demand),  // target T1 reached by adapting flow rate
	std::make_tuple(1, -1.e6, 25., -0.01, // T_HE_target, Q_w_min
			-6.245e5, -0.01, 25., wdc::WellDoubletControl::powerrate_to_adapt),  // target T1 reached by adapting power rate


	//// Scheme 2 - storing (same examples as for scheme A, and should give same results)
	std::make_tuple(2, 1.e5, 450.e6, 0.01, // DT_target, Q_w_max
			1.e5, 0.0, 70., wdc::WellDoubletControl::target_not_achievable),  // target DT not reached although flow rate is zero
	std::make_tuple(2, 1.e6, 450.e6, 0.01, // DT_target, Q_w_max
			1.e6, 0.008, 100., wdc::WellDoubletControl::on_demand),  // target DT reached by adapting flow rate
	std::make_tuple(2, 2.e6, 450.e6, 0.01,  // DT_target, Q_w_max
			1.25e6, 0.01, 100., wdc::WellDoubletControl::powerrate_to_adapt),  // target DT reached by adapting power rate
	// Scheme 2 - extracting
	std::make_tuple(2, -1.e5, -125.e6, -0.01, // DT_target, Q_w_min
			-1.e5, 0., 30., wdc::WellDoubletControl::target_not_achievable),  // target DT not reached although flow rate is at threshold

	std::make_tuple(2, -5.e5, -125.e6, -0.01,  // DT_target, Q_w_min
			-5.e5, -0.008, 25., wdc::WellDoubletControl::on_demand),  // target DT not reached by adapting flow rate
	std::make_tuple(2, -1.e6, -125.e6, -0.01,  // DT_target, Q_w_min
			-6.245e5, -0.01, 25., wdc::WellDoubletControl::powerrate_to_adapt)  // target DT reached by adapting flow rate

));

