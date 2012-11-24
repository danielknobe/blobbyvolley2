#include "PlayerIdentity.h"

PlayerIdentity::PlayerIdentity(std::string name) : mName(name)
{
	
}

PlayerIdentity::PlayerIdentity(const std::string& name, Color color, bool osci) : mName(name),
																	mStaticColor(color),
																	mOscillating(osci)
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
