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
#include <boost/lexical_cast.hpp>

std::map<std::string, CountingReport>& GetCounterMap()
{
	static std::map<std::string, CountingReport> CounterMap;
	return CounterMap;
}

std::map<void*, int>& GetAddressMap()
{
	static std::map<void*, int> AddressMap;
	return AddressMap;
}

std::map<std::string, int>& GetProfMap()
{
	static std::map<std::string, int> ProfMap;
	return ProfMap;
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

int getObjectCount(const std::type_info& type)
{
	return 	GetCounterMap()[type.name()].alive;
}

int count(const std::type_info& type, std::string tag, int n)
{
	std::string name = std::string(type.name()) + " - " + tag;
	if(GetCounterMap().find(name) == GetCounterMap().end() )
	{
		GetCounterMap()[name] = CountingReport();
	}
	GetCounterMap()[name].created += n;
	GetCounterMap()[name].alive += n;
}

int uncount(const std::type_info& type, std::string tag, int n)
{
	GetCounterMap()[std::string(type.name()) + " - " + tag].alive -= n;
}

int count(const std::type_info& type, std::string tag, void* address, int num)
{
	std::cout << "MALLOC " << num << "\n";
	count(type, tag, num);
	GetAddressMap()[address] = num;
}

int uncount(const std::type_info& type, std::string tag, void* address)
{
	int num = GetAddressMap()[address];
	std::cout << "FREE " << num << "\n";
	uncount(type, tag, num);
}

void debug_count_execution_fkt(std::string file, int line)
{
	std::string rec = file + ":" + boost::lexical_cast<std::string>(line);
	if(GetProfMap().find(rec) == GetProfMap().end() )
	{
		GetProfMap()[rec] = 0;
	}
	GetProfMap()[rec]++;
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
		stream << " created: " << i->second.created << "\n\n";
		sum += i->second.alive;
	}
	
	stream << "\n\nPROFILE REPORT\n";
	for(std::map<std::string, int>::iterator i = GetProfMap().begin(); i != GetProfMap().end(); ++i)
	{
		stream << i->first << ": ";
		stream << i->second << "\n";
	}
	
	
	total_plot << sum << std::endl;
}
