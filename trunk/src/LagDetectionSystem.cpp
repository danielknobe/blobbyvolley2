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

#include "LagDetectionSystem.h"
#include "CrossCorrelation.h"

LagDetector::LagDetector() : recalc(true), mLastLag(0)
{
	/// \todo document what this values do
	/// \todo adapt this values depending on gamespeed
	/// \todo add a gamespeed changed callback to speedmanager
	sended.resize(100);
	received.resize(80);
	
	// difference between sended.size() and received.size() is maximum detected lag
}

void LagDetector::insertData(PlayerInput send_value, PlayerInput received_value)
{
	// just insert the data into the two circular_buffers
	sended.push_front(send_value);
	received.push_front(received_value);
	
	recalc = true;
}

int LagDetector::getLag() const
{
	// only do CC if data has changed
	if( recalc ) 
	{
		CC_Result lagres = crossCorrelation(sended, received);
		recalc = false;
		mLastLag = lagres.offset;
	}
	return mLastLag;
}

std::string LagDetector::getDebugString() const
{
	// construct debug string
	std::string s;
	
	// just make a textual representation for every input
	for( boost::circular_buffer<PlayerInput>::const_iterator it = sended.begin(); it != sended.end(); ++it)
	{
		s += it->left ? (it->up ? (it->right ? '-' : 'L') : (it->right ? '_' : 'l')) : 
						(it->up ? (it->right ? 'R' : '^') : (it->right ? 'r' : 'o'));  
	}
	
	s += "\n";
	
	for( boost::circular_buffer<PlayerInput>::const_iterator it = received.begin(); it != received.end(); ++it)
	{
		s += it->left ? (it->up ? (it->right ? '-' : 'L') : (it->right ? '_' : 'l')) : (it->up ? (it->right ? 'R' : '^') : (it->right ? 'r' : 'o'));  
	}
	
	return s;
}

CC_Result LagDetector::getDebugData() const
{
	return crossCorrelation(sended, received);
}
