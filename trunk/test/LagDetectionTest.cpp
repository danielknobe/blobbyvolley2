//#define BOOST_TEST_ALTERNATIVE_INIT_API
#define BOOST_TEST_MODULE LagDetector
#include <boost/test/unit_test.hpp>


#include "InputSource.h"
#include "LagDetectionSystem.h"
#include <boost/circular_buffer.hpp>
#include "CrossCorrelation.h"
#include <fstream>
#include <cmath>
#include <iostream>

PlayerInput randomInputGen()
{
	return PlayerInput( rand() % 2, rand() % 2, rand() % 2);
}

PlayerInput randomInput(int p = 1)
{
	static PlayerInput last = randomInputGen();
	if(rand() % p == 0)
	{
		last = randomInputGen();
	}
	return last;
}
		
/// \todo add parameters: lag, packet loss, input randomness
struct LaggingNetworkInputSimulator
{
	LaggingNetworkInputSimulator(int p)
	{
		lag_simulator.resize(p + 1);
		srand(5011);
	}
	
	void sendInput( PlayerInput ip)
	{
		lag_simulator.push_back(ip);
	}
	
	void changePing(int p)
	{
		lag_simulator.set_capacity(p + 1);
		
		// we must ensure that this is full, otherwise 
		// back and front do not work as expected?
		while(!lag_simulator.full())
		{
			lag_simulator.push_front(getRemote());
		}
	}
	
	PlayerInput getLocal() const
	{
		return lag_simulator.back();
	}
	
	PlayerInput getRemote() const
	{
		return lag_simulator.front();
	}
	
	private:
		boost::circular_buffer<PlayerInput> lag_simulator;
};

/// Fixture for creating a LagDetector
template<int SetupLag>
struct LagDetectionSetup
{
	LagDetectionSetup() : D (LagDetector()), NetworkSimulator(LaggingNetworkInputSimulator( SetupLag ))
	{
	}
	
	LagDetectionSetup(int l) : D (LagDetector()), NetworkSimulator(LaggingNetworkInputSimulator( l ))
	{
	}
	
	
	~LagDetectionSetup() { };
	
	
	void reset_simulator(int l)
	{
		*this = LagDetectionSetup(l);
	}
	
	void simulate( int steps = 1, int p = 1)
	{
		for(int i = 0; i < steps; ++i)
		{
 			PlayerInput ip = randomInput(p);
			NetworkSimulator.sendInput(ip);
			D.insertData(NetworkSimulator.getLocal(), NetworkSimulator.getRemote());
		}
	}
	
	LagDetector D;
	LaggingNetworkInputSimulator NetworkSimulator;
};


// -------------------------------------------------------------------------------------------------
//
//							T E S T S
// 
// -------------------------------------------------------------------------------------------------

/// file which logs additional debug data
std::fstream file ("debug.txt", std::fstream::out);

/// file which logs quality data
std::fstream result ("results.txt", std::fstream::out);

/// how many steps do we simulate each test
const int SIMULATION_DURATION = 500;

/// \todo add a test which looks how good the algorithm performs on periodic data

BOOST_AUTO_TEST_SUITE( lag_detector )

// correlation
BOOST_AUTO_TEST_CASE( self_correlation )
{
	boost::circular_buffer<PlayerInput> buf;
	buf.resize(100);
	// insert zero lag data
	for(int i=0; i < 100; ++i)
	{
		PlayerInput ip = randomInput();
		buf.push_back(ip);
	}
	int t = crossCorrelation(buf, buf).offset;
	BOOST_REQUIRE(t == 0);
	
}


// test that checks wether LagDetector finds zero lag when no lag is present
BOOST_FIXTURE_TEST_CASE( zero_lag, LagDetectionSetup<0> )
{
	// insert zero lag data
	simulate(10);
	
	// now we do 50 steps and check for lag
	for(int i = 0; i < SIMULATION_DURATION; ++i)
	{
		simulate();
		int lag = D.getLag();
		if( lag != 0 )
		{
			char errormsg[1024];
			sprintf(errormsg, "lag of %d detected when simulating zero lag", lag);
			BOOST_FAIL("");
		};
	}
}


// test that checks wether LagDetector finds constant lags
/// \todo use http://www.boost.org/doc/libs/1_48_0/libs/test/doc/html/utf/user-guide/test-organization/unary-test-case.html
BOOST_FIXTURE_TEST_CASE( constant_lag, LagDetectionSetup<1> )
{
	file << "\nconstant lag" << "\n";
	// test for all small constant lags
	for(int clag = 1; clag < 10; clag++)
	{
		reset_simulator(clag);
		simulate(25);
		
		// now we do 50 steps and check for lag
		for(int i = 0; i < SIMULATION_DURATION; ++i)
		{
			simulate();
			int lag = D.getLag();
			
			if( lag != clag )
			{
				file << D.getDebugString() << "\n";
				char errormsg[1024];
				sprintf(errormsg, "detected lag of %d when constant lag of %d was simulated", lag, clag);
				BOOST_FAIL(errormsg);
			};
		}
	}
}

// CONFIGURE:
const int REAL_INPUT_PATTERN_LENGTH = 40;
const int INPUT_CHANGE_STEP_WIDTH = 5;

// check constant lag with bad input data
BOOST_FIXTURE_TEST_CASE( constant_lag_real_input, LagDetectionSetup<0> )
{
	file << "\nconstant lag - real input" << "\n";
	result << "constant lag - real input\n";
	result << "In\tF%\tsig\n";
	
	for(int input_quality = 10; input_quality <= REAL_INPUT_PATTERN_LENGTH; input_quality += INPUT_CHANGE_STEP_WIDTH )
	{
		// determine a random lag and set up the simulation
		int clag = rand() % 4 + 4;
		reset_simulator(clag);
		
		// start
		simulate(25, input_quality);

		// now we do 500 steps and check for lag
		int errors = 0;
		int cum_err = 0;
		for(int i = 0; i < SIMULATION_DURATION; ++i)
		{
			simulate(1, input_quality);
			
			int lag = D.getLag();
			//
			if( lag != clag )
			{
				file << D.getDebugString() << "\n";
				errors++;
				cum_err += (lag - clag) * (lag - clag);
			};
		}
		
		result << input_quality << ",\t" << (int)((100.f * errors / SIMULATION_DURATION)) << ",\t" << std::sqrt(cum_err / (float)SIMULATION_DURATION) << "\n";
		
		// we consider this test failed if more than 20% of our results are incorrect
		if(errors / (float)SIMULATION_DURATION > 0.2)
		{
			char errormsg[1024];
			sprintf(errormsg, "realisitc input and constant lag: %d %% not correct",  (int)(100.f * errors / SIMULATION_DURATION) );
			BOOST_ERROR(errormsg);
		}
	}
}


const int LAG_CHANGE_RATE = 10;
// test with high quality data but changing lags
// in this test, we can see how fast die algorithm can detect changing lag
// in this test, the lag changes are only slight, so more than 1frame per change
// does not happen
BOOST_FIXTURE_TEST_CASE( changing_lag, LagDetectionSetup<0> )
{
	file << "\nchanging lag - real input" << "\n";
	result << "changing lag - good input - small change\n";
	
	// test for all small constant lags
	int clag = rand() % 8 + 4;
	NetworkSimulator.changePing(clag);
	
	simulate(100);
	
	// now we do 500 steps and check for lag
	/// \todo maybe we should use boost.accumulator
	int lag = 0;
	int errors = 0;
	int timer = 0;
	int changes = 0;
	int cum_timer = 0;
	int cum_diff_timer = 0;
	int max_timer = 0;
	for(int i = 0; i < 500; ++i)
	{		
		// randomly change lags. we are friendly for now.
		// only change lag after system has adapted to new lag
		if(rand() % LAG_CHANGE_RATE == 0 && clag == lag)
		{
			int nclag = (rand() % 2) * 2 - 1 + clag;
			nclag = std::max(5, std::min(15, nclag));
			
			clag = nclag;
			NetworkSimulator.changePing(clag);
			timer = 0;
			changes++;
		}
		simulate();
		
		lag = D.getLag();
		
		if( lag != clag )
		{
			errors++;
			timer++;
		} else if(timer != 0) {
			result << "took " << timer << "steps to recognice lag of "<< lag << std::endl;
			file << lag << " " << timer << "\n" << D.getDebugString() << "\n";
			
			// calculate cum timer
			cum_timer += timer;
			max_timer = std::max(max_timer, std::max(0, timer - lag));
			cum_diff_timer += std::max(0, timer - lag);
			timer = 0;
		}
	}
	
	// when we take longer than 10ms to detect the lag change after the first packet with new lag arrived
	// we are too slow. 
	// when we take longer than 15ms once, it is bad, too
	if( cum_diff_timer / changes > 5  || max_timer > 15)
	{
		char errormsg[1024];
		sprintf(errormsg, "LagDetector takes too long to detect lag change of 1ms. Add: %d, Avg: %d, Max: %d",  cum_diff_timer / changes, cum_timer/changes, max_timer);
		BOOST_ERROR(errormsg);
	}
	
	result << "maximum reaction time: " << max_timer << std::endl;
	result << "average reaction time: " << cum_timer / changes << std::endl;
	result << "average additional reaction time: " << cum_diff_timer / changes << std::endl;
}

BOOST_AUTO_TEST_SUITE_END()
