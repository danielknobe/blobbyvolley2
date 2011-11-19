//#define BOOST_TEST_ALTERNATIVE_INIT_API
#define BOOST_TEST_MODULE LagDetector
#include <boost/test/unit_test.hpp>


#include "InputSource.h"
#include "LagDetectionSystem.h"
#include <boost/circular_buffer.hpp>
#include "CrossCorrelation.h"
#include <fstream>
#include <cmath>

PlayerInput randomInput()
{
	return PlayerInput( rand() % 2, rand() % 2, rand() % 2 );
}

PlayerInput randomInputR(int p)
{
	static PlayerInput last = randomInput();
	if(rand() % p == 0)
	{
		last = randomInput();
	}
	return last;
}


/// \todo make a LaggingNetworkInputSimulator, parameters: lag, packet loss, input randomness
/// \todo add a test which looks how good the algorithm performs on periodic data

#include <iostream>
BOOST_AUTO_TEST_SUITE( matrix_arithmetic )

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
BOOST_AUTO_TEST_CASE( zero_lag )
{
	LagDetector D;
	// insert zero lag data
	for(int i=0; i < 10; ++i)
	{
		PlayerInput ip = randomInput();
		D.insertData(ip, ip);
	}
	
	// now we do 50 steps and check for lag
	for(int i = 0; i < 50; ++i)
	{
		PlayerInput ip = randomInput();
		D.insertData(ip, ip);
		int lag = D.getLag();
		if( lag != 0 )
		{
			std::cout << lag << "\n";
			BOOST_FAIL("non zero lag detected with zero lag");
		};
	}
}


// test that checks wether LagDetector finds constant lags
/// \todo use http://www.boost.org/doc/libs/1_48_0/libs/test/doc/html/utf/user-guide/test-organization/unary-test-case.html
BOOST_AUTO_TEST_CASE( constant_lag )
{
	// test for all small constant lags
	for(int clag = 1; clag < 10; clag++)
	{
		LagDetector D;
		// insert zero lag data
		boost::circular_buffer<PlayerInput> oip;
		oip.resize(clag + 1);

		for(int i=0; i < 20; ++i)
		{
			PlayerInput ip = randomInput();
			oip.push_back(ip);
			D.insertData(ip, oip.front());
		}
		
		// now we do 50 steps and check for lag
		std::fstream file ("debug.txt", std::fstream::out);
		for(int i = 0; i < 50; ++i)
		{
			PlayerInput ip = randomInput();
			oip.push_back(ip);
			D.insertData(ip, oip.front());
			int lag = D.getLag();
			file << D.getDebugString() << "\n";
			if( lag != clag )
			{
				std::cout << lag<<" ! " <<clag << "\n";
				
				BOOST_FAIL("detected lag of %d when constant lag of %d was simulated");
			};
		}
	}
}

// CONFIGURE:
const int REAL_INPUT_PATTERN_LENGTH = 20;

// check constant lag with bad input data
BOOST_AUTO_TEST_CASE( constant_lag_real_input )
{
	// test for all small constant lags
	
	int clag = rand() % 4 + 4;
	
	LagDetector D;
	// insert zero lag data
	boost::circular_buffer<PlayerInput> oip;
	oip.resize(clag + 1);

	for(int i=0; i < 20; ++i)
	{
		PlayerInput ip = randomInputR(REAL_INPUT_PATTERN_LENGTH);
		oip.push_back(ip);
		D.insertData(ip, oip.front());
	}
	
	// now we do 500 steps and check for lag
	std::fstream file ("debug.txt", std::fstream::out);
	int errors = 0;
	int cum_err = 0;
	for(int i = 0; i < 500; ++i)
	{
		PlayerInput ip = randomInputR(REAL_INPUT_PATTERN_LENGTH);
		oip.push_back(ip);
		D.insertData(ip, oip.front());
		int lag = D.getLag();
		//
		if( lag != clag )
		{
			file << D.getDebugString() << "\n";
			errors++;
			cum_err += (lag - clag) * (lag - clag);
		};
	}
	
	std::cout << 100 * errors / 500.f << " v: " << std::sqrt(cum_err / 500.f) << std::endl;
	
	if(errors != 0)
		BOOST_ERROR("some problems with difficult constant lag");
}


const int LAG_CHANGE_RATE = 10;
// test with high quality data but changing lags
BOOST_AUTO_TEST_CASE( changing_lag )
{
	// test for all small constant lags
	int clag = rand() % 11;
	LagDetector D;
	// insert zero lag data
	boost::circular_buffer<PlayerInput> oip;
	oip.resize(clag + 1);

	for(int i=0; i < 20; ++i)
	{
		PlayerInput ip = randomInput();
		oip.push_back(ip);
		D.insertData(ip, oip.front());
	}
		
	// now we do 50 steps and check for lag
	std::fstream file ("debug.txt", std::fstream::out);
	for(int i = 0; i < 50; ++i)
	{
		PlayerInput ip = randomInput();
		oip.push_back(ip);
		
		if(rand() % LAG_CHANGE_RATE == 0)
		{
			int nclag = (rand() % 2) * 2 - 1 + clag;
			if(clag > 10)
				clag = 10;
			/*if(clag+1 < oip.size())
			{
				D.insertData(ip, oip.front());
				oip.pop_front();
			}*/	
			if(clag != nclag)
			{
				clag = nclag;
				oip.set_capacity(clag+1);
			}
		}
		
		
		D.insertData(ip, oip.front());
		int lag = D.getLag();
		if( lag != clag )
		{
			std::cout << lag<<" ! " <<clag << "\n";
			file << lag << " " << clag << "\n" << D.getDebugString() << "\n\n";
		
			BOOST_ERROR("detected lag of %d when constant lag of %d was simulated");
		};
	}
}

BOOST_AUTO_TEST_SUITE_END()
