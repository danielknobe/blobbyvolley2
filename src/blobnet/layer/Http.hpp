/*=============================================================================
blobNet
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

#ifndef _BLOBNET_LAYER_HTTP_HPP_
#define _BLOBNET_LAYER_HTTP_HPP_

/* Includes */
#include "../../raknet/SocketLayer.h"

#include <string>

namespace BlobNet {
namespace Layer {
	/*!	\class Http
		\brief Simple Layer for HTTP communication over TCP/IP
	*/
	class Http
	{
	public:
		/// @brief constructor, connects to an host
		Http(const std::string& host, const int& port);

		/// @brief deconstructor, closes connection to host
		~Http();

		/// @brief sends a request
		void request(const std::string& path);
	private:
		SOCKET mSocket;
		SocketLayer mSocketLayer;
	};
}
}

#endif


