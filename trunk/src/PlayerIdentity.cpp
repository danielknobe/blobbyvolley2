#include "PlayerIdentity.h"

PlayerIdentity::PlayerIdentity(std::string name) : mName(name),
																	mOscillating(false)
{

}

PlayerIdentity::PlayerIdentity(const std::string& name, Color color, bool osci, PlayerSide side) : mName(name),
																	mStaticColor(color),
																	mOscillating(osci),
																	mPreferredSide(side)
{

}

PlayerIdentity::~PlayerIdentity()
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
