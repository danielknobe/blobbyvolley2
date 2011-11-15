/*=============================================================================
Blobby Volley 2
Copyright (C) 2006 Jonathan Sieber (jonathan_sieber@yahoo.de)

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

#pragma once

#include <boost/circular_buffer.hpp>
#include <string>

#include "InputSource.h"
/*!	\class LagDetector
	\brief Class for detecting network lag
	\details This class is used to determine how much lag we have in network game. 
			It works by comparing the input generated locally with the input data
			the server sends together with the world information, and determines 
			the time difference by using cross-correlation.
	\attention There are situations when it is impossible to determine the lag
				exaclty, e.g. ehen ping is 	changing rapidly or the imformation 
				we get from the network	is not identifiable as the same sequence
				 (i.e. half the packets got lost etc). In either of these cases
				 incorrect lag values are our smalles problem ;)
				 Still, we should be able to detect such situations.
				 Also, when player input is not changing, we get no inforamtion
				 and cannot calculate the ping correctly.
	\todo	Auto-Detect unusable data (due to packet loss, fast changing ping 
										or no user input)
	\todo	Fix false detection for periodical structures.
			When ping is generally about 8, and the same input data is sent twice
			and due to some noise the older one fits better, the lag is assumed to
			be much bigger than it actually is.
	\todo	should this lag detector get a understanding of real time? so if steps 
			are not equi-timed, it still would work.
	\todo	I would like to implement this in a more generic manner, so we are no
			limited to using PlayerInput values for determining ping but could use 
			data send especially for this purpose. (can't happen before 1.0 though, 
			as we agreed not to change any network packets till this release).
				
*/
class LagDetector
{
	public:
		/// \brief constructor
		/// \todo add parameters, as we have internals which affec how good the detection works
		///		(and how much CPU power is consumed ;)
		LagDetector();
		
		/// \brief submit data for evaluation
		/// \details This function has to be called every frame and gets 
		///			 the current input values.
		/// \attention Don't!! swap the ordering of these parameters. Lag detection 
		///				produces weird results!
		/// \param send_value	Current PlayerInput, which was sent/will be send within this
		///						time step to the server
		///	\param received_value	Last Input data we received from server.
		void insertData(PlayerInput send_value, PlayerInput received_value);
		
		/// \brief returns current lag
		///	\todo cache the lag value so to successive calls without any call to insertData inbetween
		///			don't result in doing the cross-correlation again!
		int getLag() const;
		
		/// \brief Debug string
		/// \details returns the current internal data in human readable format for checking
		///			if cross-correlation found the correct ping.
		/// \todo would be good if we could get rid of the <string> include, though
		std::string getDebugString() const;
		
		
	private:
		
		/// \todo it would be cool if we could hide this details in a pimpl pointer
		boost::circular_buffer<PlayerInput> sended;
		boost::circular_buffer<PlayerInput> received;
};
