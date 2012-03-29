/*=============================================================================
Blobby Volley 2
Copyright (C) 2006 Jonathan Sieber (jonathan_sieber@yahoo.de)
Copyright (C) 2006 Daniel Knobe (daniel-knobe@web.de)

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

/* header include */
#include "Player.h"

/* includes */
#include <iostream> // for cerr

#include "LocalInputSource.h"
#include "ScriptedInputSource.h"
#include "UserConfig.h"

/* implementation */
Player::Player(PlayerSide side) :
	mInitialised(false),
	mPlayerSide(side)
{
}

Player::~Player()
{
}

void Player::loadFromConfig(const std::string& prefix, bool initInput)
{
	UserConfig gameConfig;
	gameConfig.loadFile("config.xml");

	// init local input
	if(initInput)
	{
		try
		{
			// these operations may throw, i.e., when the script is not found (should not happen)
			//  or has errors
			if (gameConfig.getBool(prefix + "_player_human")) 
			{
				mInputSource.reset(new LocalInputSource(mPlayerSide));
			} 
			else 
			{
				mInputSource.reset(new ScriptedInputSource("scripts/" +
					gameConfig.getString(prefix + "_script_name"), mPlayerSide, gameConfig.getInteger(prefix + "_script_strength")));
			}
		} catch (std::exception& e)
		{
			/// \todo REWORK ERROR REPORTING
			std::cerr << e.what() << std::endl;
			mInputSource.reset(new DummyInputSource());
		}
	
		mName = gameConfig.getBool(prefix + "_player_human") ?
			gameConfig.getString(prefix + "_player_name") :
			gameConfig.getString(prefix + "_script_name");
	}
	else
	{
		// input is set externally (network)
		// so we need not to create any input source
		mInputSource.reset();
		// don't use bot name if extern input is used
		mName = gameConfig.getString(prefix + "_player_name");
	}
	
	mStaticColor = Color(
		gameConfig.getInteger(prefix + "_blobby_color_r"),
		gameConfig.getInteger(prefix + "_blobby_color_g"),
		gameConfig.getInteger(prefix + "_blobby_color_b"));


	mOscillating = gameConfig.getBool(prefix + "_blobby_oscillate");

	mInitialised = true;
}

Color Player::getColor() const
{
	assert(mInitialised);

	if (!mOscillating) 
	{
		return mStaticColor;
	}
	 else 
	{
		float time = float(SDL_GetTicks()) / 1000.0;
		return Color(
			int((sin(time*2) + 1.0) * 128),
			int((sin(time*4) + 1.0) * 128),
			int((sin(time*3) + 1.0) * 128));
	}
}

void Player::setColor(Color ncol)
{
	mStaticColor = ncol;
}

std::string Player::getName() const
{
	assert(mInitialised);

	return mName;
}

void Player::setName(const std::string& name)
{
	mName = name;
}

InputSource* Player::getInputSource() const
{
	assert(mInitialised);
	assert(mInputSource != 0);

	return mInputSource.get();
}

