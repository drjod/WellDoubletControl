#include "wellDoubletControl.cpp"
#define TEMPERATURE_2 30


class Simulator
{
	double T1, T2;
	WellDoubletControl* wellDoublet;
public:
	Simulator() {}
	Simulator(WellDoubletControl* _wellDoublet) : wellDoublet(_wellDoublet)
	{ /* set accuracy (epsilon) and fluid thermal capacity */ }

	void set_temperature(const double& _T1, const double& _T2)
	{ T1 = _T1; T2 = _T2; }

	WellDoubletResult execute_timeStep(double Q_H, double value_target, 
			double value_threshold, double T1_new, double T2_new)
	{
		wellDoublet->set_iterationValues(T1, T2);
		wellDoublet->set_timeStepValues(Q_H,
				value_target, value_threshold);

		do  // iterate
		{
			wellDoublet->calculate_flowrate();
			set_temperature(T1_new, T2_new);			
			wellDoublet->set_iterationValues(T1, T2);
		}
		while(wellDoublet->check_result());

		return wellDoublet->get_result();
	}
};


class WellDoubletTest : public ::testing::TestWithParam<std::tr1::tuple<
	char, double, double, double, double, double, 
	double, double, bool> > {};


TEST_P(WellDoubletTest, WellDoubletControl)
{
	Simulator simulator = Simulator(
		WellDoubletControl::createWellDoubletControl(
				std::tr1::get<0>(GetParam())));
	
	simulator.set_temperature(std::tr1::get<4>(GetParam()),  // T1_init
				TEMPERATURE_2);  // T2_init

	WellDoubletResult result = simulator.execute_timeStep(
		std::tr1::get<1>(GetParam()),  // Q_H
		std::tr1::get<2>(GetParam()),  // value_target
		std::tr1::get<3>(GetParam()),  // value_threshold 
		std::tr1::get<5>(GetParam()),  // T1_new
		TEMPERATURE_2);  // T2_new

	EXPECT_EQ(std::tr1::get<6>(GetParam()), result.Q_H);
	EXPECT_EQ(std::tr1::get<7>(GetParam()), result.Q_w);
	EXPECT_EQ(std::tr1::get<8>(GetParam()), result.flag_powerrateAdapted);
}


INSTANTIATE_TEST_CASE_P(TestWithParamsC1, WellDoubletTest, testing::Values(
	// scheme, Q_H,
	// value_target, value_threshold,
	// T1_init, T1_new
	// result: Q_H, Q_w, flag_powerrateAdapted

	// scheme B - storing
	std::make_tuple('B', 1e6,
			0.01, 70, // Q_w_target, T_1_max
			60, 60,  // does not reach threshold
			1e6, 0.01, false),
	std::make_tuple('B', 1e6,
			0.001, 70, // Q_w_target, T_1_max
			60, 80,  // passed threshold
			2e5, 0.001, true),
	std::make_tuple('B', 1e6,
			0.001, 70, // Q_w_target, T_1_max
			60, 70,  // exactly as threshold
			1e6, 0.001, false)
));


