/*=============================================================================
Blobby Volley 2
Copyright (C) 2006 Jonathan Sieber (jonathan_sieber@yahoo.de)
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

/* header include */
#include "InputSourceFactory.h"

/* includes */
#include <iostream>

#include <boost/make_shared.hpp>

#include "IUserConfigReader.h"
#include "LocalInputSource.h"
#include "ScriptedInputSource.h"

boost::shared_ptr<InputSource> InputSourceFactory::createInputSource( boost::shared_ptr<IUserConfigReader> config, PlayerSide side )
{
	std::string prefix = side == LEFT_PLAYER ? "left" : "right";
	try
	{
		// these operations may throw, i.e., when the script is not found (should not happen)
		//  or has errors
		if (config->getBool(prefix + "_player_human")) 
		{
			return boost::make_shared<LocalInputSource>(side);
		} 
		else 
		{
			return boost::make_shared<ScriptedInputSource>("scripts/" + config->getString(prefix + "_script_name"), 
																side, config->getInteger(prefix + "_script_strength"));
		}
	} catch (std::exception& e)
	{
		/// \todo REWORK ERROR REPORTING
		std::cerr << e.what() << std::endl;
		return boost::make_shared<InputSource>();
	}
}
