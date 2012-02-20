#define BOOST_TEST_MODULE GameLogic
#include <boost/test/unit_test.hpp>

#include <iostream>
#include "physfs.h"
#include "GameLogic.h"

BOOST_AUTO_TEST_SUITE( katjas_replay )

#define PASSING_TIME() for(int i=0; i < 50; ++i, GL->step());

BOOST_AUTO_TEST_CASE( random_input_test )
{
	BOOST_REQUIRE( PHYSFS_init("C:\\Dokumente und Einstellungen\\Erik\\Eigene Dateien\\Blobby Volley 2\\test\\bin\\debug\\") );
	PHYSFS_setWriteDir(".");
	PHYSFS_addToSearchPath(".", 1);
	
	GameLogic GL = createGameLogic("rules.lua");
	
	// thats the scenario from katja's replay
	PASSING_TIME();
	GL->onBallHitsPlayer(LEFT_PLAYER);
	GL->onBallHitsPlayer(RIGHT_PLAYER);
	PASSING_TIME();
	GL->onBallHitsPlayer(LEFT_PLAYER);
	PASSING_TIME();
	GL->onBallHitsPlayer(LEFT_PLAYER);
	PASSING_TIME();
	
	BOOST_CHECK_EQUAL(GL->getScore(LEFT_PLAYER), 0);
	BOOST_CHECK_EQUAL(GL->getScore(RIGHT_PLAYER), 0);


}


BOOST_AUTO_TEST_SUITE_END()
