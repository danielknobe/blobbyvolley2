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

#pragma once

#include <typeinfo>
#include <iosfwd>
#include <utility>
#include <cstddef>
#include <cstdlib>
#include <memory>
#include <string>

int count(const std::type_info& type);
int uncount(const std::type_info& type);
int count(const std::type_info& type, std::string tag, int num);
int uncount(const std::type_info& type, std::string tag, int num);

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
int getObjectCount(const std::type_info& type);

struct CountingReport
{
	CountingReport() : alive(0), created(0)
	{
		
	}
	
	int alive;
	int created;
};

// counting allocator
template<class T, typename tag_type>
struct CountingAllocator : private std::allocator<T>
{
	typedef std::allocator<T> Base;
	typedef T value_type;
	typedef T* pointer;
	typedef T& reference;
	typedef const T* const_pointer;
	typedef const T& const_reference;
	typedef size_t size_type;
	typedef ptrdiff_t difference_type ;
	
	CountingAllocator()
	{
		
	}
	
	template<class V, typename tag2>
	CountingAllocator(const CountingAllocator<V, tag2>& other)
	{
	}
	
	template<typename _Tp1>
	struct rebind
	{ 
		typedef CountingAllocator<_Tp1, tag_type> other; 
	};
	
	
	
	pointer allocate (size_type n, std::allocator<void>::const_pointer hint = 0)
	{
		count(typeid(T), tag_type::tag(), n);
		return Base::allocate(n, hint);
	}
	
	void deallocate (pointer p, size_type n)
	{
		uncount(typeid(T), tag_type::tag(), n);
		Base::deallocate(p, n);
	}
	
	using Base::address;
	using Base::max_size;
	using Base::construct;
	using Base::destroy;
};



int count(const std::type_info& type, std::string tag, void* address, int num);
int uncount(const std::type_info& type, std::string tag, void* address);

template<class T, typename tag_type>
struct CountingMalloc
{	
	typedef T* pointer;
	typedef T& reference;
	typedef size_t size_type;
	
	static pointer malloc (size_type n)
	{
		pointer nm = static_cast<pointer> ( ::malloc(n) );
		count(typeid(T), tag_type::tag(), nm, n);
		return nm;
	}
	
	static void free (pointer& p)
	{
		uncount(typeid(T), tag_type::tag(), p);
		::free(p);
		p = 0;
	}
	
	static pointer realloc ( pointer ptr, size_t size )
	{
		uncount(typeid(T), tag_type::tag(), ptr);
		pointer nm = static_cast<pointer>(::realloc(ptr, size));
		count(typeid(T), tag_type::tag(), nm, size);
	}
};

struct string_tag
{
	static std::string tag()
	{
		return "basic_string<char>";
	}
};

typedef std::basic_string< char, std::char_traits<char>, CountingAllocator<char, string_tag> > TrackedString;

void debug_count_execution_fkt(std::string file, int line);

#define DEBUG_COUNT_EXECUTION debug_count_execution_fkt(__FILE__, __LINE__);
