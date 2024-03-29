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

#ifndef BLOBNET_EXCEPTION_HTTPEXCEPTION_HPP_
#define BLOBNET_EXCEPTION_HTTPEXCEPTION_HPP_

/* Includes */
#include <stdexcept>

namespace BlobNet {
namespace Exception {
	/*!	\class HttpException
		\brief Runtimeexception for HttpErrors
	*/
	class HttpException : public std::runtime_error
	{
	public:
		/// @brief constructor
		/// @param message Message to describe the error
		explicit HttpException(const std::string& message) : std::runtime_error(message)
		{	
		};
	};
}
}

#endif


