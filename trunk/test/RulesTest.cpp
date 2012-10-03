#define BOOST_TEST_MODULE GameLogic
#include <boost/test/unit_test.hpp>

#include <iostream>
#include "physfs.h"
#include "GameLogic.h"
#include "DuelMatch.h"
#include "DuelMatchState.h"
#include "IGenericIO.h"
#include "FileRead.h"
#include <fstream>

BOOST_AUTO_TEST_SUITE( rules_test )

#define PASSING_TIME() for(int i=0; i < 50; ++i, GL->step());

BOOST_AUTO_TEST_CASE( katja_replay )
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

BOOST_AUTO_TEST_CASE( net_squish )
{
	//BOOST_REQUIRE( PHYSFS_init("C:\\Dokumente und Einstellungen\\Erik\\Eigene Dateien\\Blobby Volley 2\\test\\bin\\debug\\") );
	//PHYSFS_setWriteDir(".");
	//PHYSFS_addToSearchPath(".", 1);
	
	std::fstream deb_out ("debug.txt", std::fstream::out);
	
	DuelMatch match(0, 0, false, false);
	
	DuelMatchState mstate;
	FileRead reader ("katjareplay_crash.state");
	boost::shared_ptr<GenericIn> gin = GenericIO::createReader(reader);
	
	mstate.deserialize(gin.get());
	
	match.setState(mstate);
	
	for(int i=0; i < 75; ++i)
	{
		match.setPlayersInput( PlayerInput(false, true, true), PlayerInput(true, false, true) );
		match.step();
		deb_out << match.getState().worldState.ballPosition.x << "\t" << 600 - match.getState().worldState.ballPosition.y << "\n";;
	}
}

BOOST_AUTO_TEST_SUITE_END()
