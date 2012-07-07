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

#include <typeinfo>
#include <iosfwd>

int count(const std::type_info& type);
int uncount(const std::type_info& type);

/*! \class ObjectCounter
	\brief Logging number of creations and living objects
	\details To use this class for logging creations of a class TYPE, just derive it
			from ObjectCounter<TYPE>. A full memory report can be written to a stream 
			by the record function.
	\todo more specific reporting, watches, etc.
*/
template<class Base>
class ObjectCounter
{
	public:
		ObjectCounter() 
		{
			count(typeid(Base));
		};
		~ObjectCounter() 
		{
			uncount(typeid(Base));
		};
		
		ObjectCounter(const ObjectCounter& other)
		{
			count(typeid(Base));
		}
		
		ObjectCounter& operator=(const ObjectCounter& other)
		{
			return *this;
		}
};


void report(std::ostream& stream);

struct CountingReport
{
	CountingReport() : alive(0), created(0)
	{
		
	}
	int alive;
	int created;
};
