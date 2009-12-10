/*=============================================================================
Blobby Volley 2
Copyright (C) 2008 Jonathan Sieber (jonathan_sieber@yahoo.de)

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

#include "Player.h"

#include "LocalInputSource.h"
#include "ScriptedInputSource.h"
#include "UserConfig.h"

Player::Player(PlayerSide side) :
	mInitialised(false),
	mPlayerSide(side)
{
}

Player::~Player()
{
	if (mInitialised) {
		delete mInputSource;
	}
}

void Player::loadFromConfig(const std::string& prefix)
{
	UserConfig gameConfig;
	gameConfig.loadFile("config.xml");

	if (gameConfig.getBool(prefix + "_player_human")) {
		mInputSource = new LocalInputSource(mPlayerSide);
	} else {
		mInputSource = new ScriptedInputSource("scripts/" +
			gameConfig.getString(prefix + "_script_name"), mPlayerSide);
	}

	mName = gameConfig.getBool(prefix + "_player_human") ?
		gameConfig.getString(prefix + "_player_name") :
		gameConfig.getString(prefix + "_script_name");

	mInitialised = true;
	
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

	if (!mOscillating) {
		return mStaticColor;
	} else {
		float time = float(SDL_GetTicks()) / 1000.0;
		return Color(
			int((sin(time*2) + 1.0) * 128),
			int((sin(time*4) + 1.0) * 128),
			int((sin(time*3) + 1.0) * 128));
	}
}

std::string Player::getName() const
{
	assert(mInitialised);

	return mName;
}

InputSource* Player::getInputSource() const
{
	assert(mInitialised);
	assert(mInputSource != 0);

	return mInputSource;
}

