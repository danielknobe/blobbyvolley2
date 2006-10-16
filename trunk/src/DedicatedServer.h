#pragma once

#include <map>
#include <list>

#include "raknet/NetworkTypes.h"

class NetworkGame;

typedef std::map<PlayerID, NetworkGame*> PlayerMap;
typedef std::list<NetworkGame*> GameList;
