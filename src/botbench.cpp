/*=============================================================================
Blobby Volley 2
Copyright (C) 2023 Daniel Knobe (daniel-knobe@web.de)
Copyright (C) 2023 Erik Schultheis (erik-schultheis@freenet.de)

This program is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
=============================================================================*/

/* includes */
#include <ctime>
#include <cstring>
#include <sstream>
#include <atomic>
#include <thread>
#include <iostream>

#include <SDL.h>

#include "Global.h"
#include "UserConfig.h"
#include "FileSystem.h"
#include "ScriptedInputSource.h"
#include "DuelMatch.h"
#include "replays/ReplayRecorder.h"
#include "FileWrite.h"

/* implementation */

struct DuelResult {
	std::string LeftPlayer;
	std::string RightPlayer;
	int LeftScore;
	int RightScore;
	int Duration;
};

DuelResult duel(std::string left, std::string right, bool verbose=false);
void present(const DuelResult& result);

int main(int argc, char* argv[])
{
	if(argc < 3) {
		std::cerr << "Usage: " << argv[0] << " [LEFT] [RIGHT]\n";
		return EXIT_FAILURE;
	}

	std::string left_bot = argv[1];
	std::string right_bot = argv[2];

	FileSystem filesys(argv[0]);
	filesys.setWriteDir("/tmp");
	filesys.addToSearchPath("data");

	SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO | SDL_INIT_JOYSTICK);
	srand(SDL_GetTicks());

	try
	{
		auto result = duel( left_bot, right_bot, true );
		present( result );
	} catch (const boost::exception& ex) {
		// error handling
		std::cerr <<  boost::diagnostic_information(ex);
		exit(EXIT_FAILURE);
	} catch (std::exception& ex) {
		std::cerr << ex.what();
		exit(EXIT_FAILURE);
	}

	return EXIT_SUCCESS;
}


DuelResult duel(std::string left, std::string right, bool verbose) {
	DuelMatch match{false, "default.lua"};
	auto leftInput = std::make_shared<ScriptedInputSource>("scripts/" + left, LEFT_PLAYER, 0, &match);
	auto rightInput = std::make_shared<ScriptedInputSource>("scripts/" + right, RIGHT_PLAYER, 0, &match);
	leftInput->setWaitTime(5);
	rightInput->setWaitTime(5);

	match.setPlayers(PlayerIdentity{}, PlayerIdentity{});
	match.setInputSources(leftInput, rightInput);

	ReplayRecorder recorder;
	recorder.setGameSpeed( 75 );
	recorder.setGameRules( "default.lua" );
	recorder.setPlayerNames(left, right);

	int timer = 0;

	while (match.winningPlayer() == NO_PLAYER)
	{
		++timer;
		recorder.record(match.getState());
		match.step();
		if(verbose && timer % 10000 == 0) {
			std::cout << timer / 1000 << "k steps: ";
			std::cout << match.getScore(LEFT_PLAYER) << " - " << match.getScore(RIGHT_PLAYER) << "\n";
		}

		// stop at 12 hours
		if(timer > 75 * 60 * 60 * 12) {
			break;
		}
	}

	recorder.record( match.getState() );
	recorder.finalize( match.getScore(LEFT_PLAYER), match.getScore(RIGHT_PLAYER) );

	FileWrite save_target{"bot-fight.bvr"};
	recorder.save(save_target);

	return {std::move(left), std::move(right), match.getScore(LEFT_PLAYER), match.getScore(RIGHT_PLAYER), timer / 75};
}

void present(const DuelResult& result) {
	std::cout << result.LeftPlayer << " vs " << result.RightPlayer << ": "
	          << result.LeftScore << " - " << result.RightScore << " in "
			  << result.Duration << " seconds of game time\n";
}