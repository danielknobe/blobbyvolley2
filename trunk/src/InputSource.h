#pragma once

struct PlayerInput
{
	PlayerInput()
	{
		left = false;
		right = false;
		up = false;
	}
	
	PlayerInput(bool l, bool r, bool u)
	{
		left = l;
		right = r;
		up = u;
	}

	bool left : 1;
	bool right : 1;
	bool up : 1;
};

class InputSource
{
public:
	virtual PlayerInput getInput() = 0;
};

/*
class RemoteInputSource : public InputSource
{
private:
	NetworkManager* mNetworkManager;

public:
	RemoteInputSource(NetworkManager* netmgr)
		: mNetworkManager(netmgr);
	virtual PlayerInput getInput()
	{
		return mNetworkManager->getLatestInput();
	}
};
*/

