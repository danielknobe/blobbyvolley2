#pragma once

class NetworkServer;
class NetworkClient;

class NetworkManager
{
private:
	NetworkManager();

	static NetworkManager* mSingleton;

public:
	~NetworkManager();
	
	static NetworkManager* getSingleton();
	static NetworkManager* createNetworkManager();

	NetworkServer* createServer(int port);
	NetworkClient* createClient(const std::string& hostname, int port);

};

