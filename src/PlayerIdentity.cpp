#include "PlayerIdentity.h"

#include <utility>

PlayerIdentity::PlayerIdentity(std::string name) : mName(std::move(name)), mOscillating(false)
{

}

PlayerIdentity::PlayerIdentity(std::string name, Color color, bool osci, PlayerSide side) : mName(std::move(name)),
																	mStaticColor(color),
																	mOscillating(osci),
																	mPreferredSide(side)
{

}

const std::string& PlayerIdentity::getName() const
{
	return mName;
}

Color PlayerIdentity::getStaticColor() const
{
	return mStaticColor;
}

bool PlayerIdentity::getOscillating() const
{
	return mOscillating;
}

PlayerSide PlayerIdentity::getPreferredSide() const
{
	return mPreferredSide;
}

void PlayerIdentity::setName(const std::string& nname)
{
	mName = nname;
}

void PlayerIdentity::setStaticColor(Color c)
{
	mStaticColor = c;
}

void PlayerIdentity::setOscillating(bool oc)
{
	mOscillating = oc;
}

void PlayerIdentity::setPreferredSide(PlayerSide side)
{
	mPreferredSide = side;
}
