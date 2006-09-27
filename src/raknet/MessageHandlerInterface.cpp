#include "MessageHandlerInterface.h"

#pragma warning( disable : 4100 ) // warning C4100: <variable name> : unreferenced formal parameter
bool MessageHandlerInterface::PropagateToGame(Packet *packet) const
{
	return false;
}

#pragma warning( disable : 4100 ) // warning C4100: <variable name> : unreferenced formal parameter
void MessageHandlerInterface::OnAttach(RakPeerInterface *peer)
{
}

