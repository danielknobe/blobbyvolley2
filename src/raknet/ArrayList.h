/* -*- mode: c++; c-file-style: raknet; tab-always-indent: nil; -*- */
/**
 * @file 
 * @brief Provide an array based list container. 
 *
 * An Array list container provide an o(1) access to its 
 * elements. It's quite similar to the std::vector features. 
 *
 * Copyright (c) 2003, Rakkarsoft LLC and Kevin Jenkins
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *     * Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above copyright
 *       notice, this list of conditions and the following disclaimer in the
 *       documentation and/or other materials provided with the distribution.
 *     * Neither the name of the <organization> nor the
 *       names of its contributors may be used to endorse or promote products
 *       derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL <COPYRIGHT HOLDER> BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */


#ifndef __LIST_H
#define __LIST_H 

#include <cassert>
#include <vector>

/**
 * Define the max unsigned int value available 
 * @todo Take this value from @em limits.h. 
 * 
 */
static const unsigned int MAX_UNSIGNED_LONG = 4294967295U;

namespace BasicDataStructures
{
	/**
	 * 
	 * List ADT (Array Style) - By Kevin Jenkins
	 * Initialize with the following structure
	 * List<TYPE, OPTIONAL INITIAL ALLOCATION SIZE>
	 *
	 * Has the following member functions
	 * [x] - Overloaded: Returns element x (starting at 0) in the list between the brackets.  If no x argument is specified it returns the last element on the list. If x is invalid it returns 0
	 * size - returns number of elements in the list
	 * insert(item, x) - inserts <item> at position x in the list.  If no x argument is specified it adds item to the end of the list
	 * replace(item, filler, x) - replaces the element at position x with <item>.  If x is greater than the current list size, the list is increased and the other values are assigned filler
	 * del(OPTIONAL x) - deletes the element at position x.  If no x argument is specified it deletes the last element on the list
	 * compress - reallocates memory to fit the number of elements.  Best used when the number of elements decreases
	 * clear - empties the list and returns storage
	 * The assignment and copy constructor operators are defined
	 *
	 * EXAMPLE
	 * @code
	 * List<int, 20> A;
	 * A.size; // Returns 0
	 * A.insert(10);  // Adds 10 to the end of the list
	 * A[0];  // Returns 10
	 * A.insert(1,0);  // Adds 1 to front of list so list reads 1,10
	 * A.replace(5,0, 1); // Replaces element 1 with a 5.  List reads 1,5
	 * A.del();  // Deletes the last item on the list. List reads 1
	 * A.size; // Returns 1
	 * @endcode
	 * 
	 * @note
	 * The default initial allocation size is 1.
	 * This function doubles the amount of memory allocated when the list is filled
	 * This list is held in an array and is best used for random element selection
	 *
	 */
	
	template <class list_type>
	class List
	{	
	public:
		/**
		 * Default constructor
		 */
		List();
		/**
		 * Destructor
		 */
		~List();
		/**
		 * Copy constructor
		 * @param original_copy The list to duplicate 
		 */
		List( const List& original_copy );
		/**
		 * 
		 */
		List& operator= ( const List& original_copy );
		/**
		 * Access an element by its index in the array 
		 * @param position The index in the array. 
		 * @return the element at position @em position. 
		 */
		list_type& operator[] ( unsigned int position );
		/**
		 * Insert an element at position @em position in the list 
		 * @param input The new element. 
		 * @param position The position of the new element. 
		 */
		void insert( list_type input, unsigned int position );
		/**
		 * Insert at the end of the list.
		 * @param input The new element. 
		 */
		void insert( list_type input );
		/**
		 * replace the value at @em position by @em input.  If the size of
		 * the list is less than @em position, it increase the capacity of
		 * the list and fill slot with @em filler.
		 * @param input The element to replace at position @em position. 
		 * @param filler The element use to fill new allocated capacity. 
		 * @param position The position of input in the list. 
		 */
		void replace( list_type input, list_type filler, unsigned int position );
		
		/**
		 * replace the last element of the list by @em input 
		 * @param input the element used to replace the last element. 
		 */
		void replace( list_type input );
		
		/**
		 * Delete the element at position @em position. 
		 * @param position the index of the element to delete 
		 */
		void del( unsigned int position );
		
		/**
		 * Delete the element at the end of the list 
		 */
		void del();
		
		/**
		 * Returns the index of the specified item or MAX_UNSIGNED_LONG if not found
		 * @param input The element to check for 
		 * @return The index or position of @em input in the list. 
		 * If input is not in the list MAX_UNSIGNED_LONG is returned. 
		 */
		unsigned int getIndexOf( list_type input );
		
		/**
		 * Get the size of the list 
		 */
		const unsigned int size( void ) const;
		
		/**
		 * Clear the list 
		 */
		void clear( void );
		
	private:
		/**
		 * Store all values 
		 */
		 typedef std::vector<list_type> container_type;
		 container_type array;
		
	};
	
	template <class list_type>
	List<list_type>::List()
	{
		array.reserve(16);
	}
	
	template <class list_type>
	List<list_type>::~List()
	{
	}
	
	
	template <class list_type>
	List<list_type>::List( const List& original_copy )
	{
		array = original_copy.array;
	}
	
	template <class list_type>
	List<list_type>& List<list_type>::operator= ( const List& original_copy )
	{
		array = original_copy.array;
		
		return *this;
	}
	
	
	template <class list_type>
	inline list_type& List<list_type>::operator[] ( unsigned int position )
	{
		return array.at(position);
	}
	
	template <class list_type>
	void List<list_type>::insert( list_type input, unsigned int position )
	{
		array.insert(array.begin() + position, input);
		
		// Insert the new item at the correct spot
		assert( array[ position ] == input );
	}
	
	
	template <class list_type>
	void List<list_type>::insert( list_type input )
	{
		array.push_back(input);
	}
	
	template <class list_type>
	inline void List<list_type>::replace( list_type input, list_type filler, unsigned int position )
	{
		if( position >= array.size() )
		{
			// reserve new memory
			array.resize( position + 1, filler );
		}
		
		array[ position ] = input;
	}
	
	template <class list_type>
	inline void List<list_type>::replace( list_type input )
	{
		if ( !array.empty() )
			array.back() = input;
	}
	
	template <class list_type>
	void List<list_type>::del( unsigned int position )
	{

		array.erase( array.begin() + position );
	}
	
	template <class list_type>
	inline void List<list_type>::del()
	{
		// Delete the last element on the list.  No compression needed
		array.pop_back();
	}
	
	template <class list_type>
	unsigned int List<list_type>::getIndexOf( list_type input )
	{
		for ( unsigned int i = 0; i < array.size(); ++i )
			if ( array[ i ] == input )
				return i;
				
		return MAX_UNSIGNED_LONG;
	}
	
	template <class list_type>
	inline const unsigned int List<list_type>::size( void ) const
	{
		return array.size();
	}
	
	template <class list_type>
	void List<list_type>::clear( void )
	{
		array.clear();
	}
	
} // End namespace

#endif
