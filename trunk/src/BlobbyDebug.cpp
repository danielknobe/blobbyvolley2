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

#include "BlobbyDebug.h"
#include <string>
#include <map>
#include <iostream>
#include <fstream>

std::map<std::string, CountingReport>& GetCounterMap()
{
	static std::map<std::string, CountingReport> CounterMap;
	return CounterMap;
}
int count(const std::type_info& type)
{
	std::string test = type.name();
	if(GetCounterMap().find(type.name()) == GetCounterMap().end() )
	{
		GetCounterMap()[type.name()] = CountingReport();
	}
	GetCounterMap()[type.name()].created++;
	GetCounterMap()[type.name()].alive++;
}

int uncount(const std::type_info& type)
{
	GetCounterMap()[type.name()].alive--;
}

std::fstream total_plot("logs/total.txt", std::fstream::out);

void report(std::ostream& stream)
{
	stream << "MEMORY REPORT\n";
	int sum = 0;
	for(std::map<std::string, CountingReport>::iterator i = GetCounterMap().begin(); i != GetCounterMap().end(); ++i)
	{
		stream << i->first << "\n- - - - - - - - - -\n";
		stream << " alive:   " << i->second.alive << "\n";
		stream<< " created: " << i->second.created << "\n\n";
		sum += i->second.alive;
	}
	
	total_plot << sum << std::endl;
}
