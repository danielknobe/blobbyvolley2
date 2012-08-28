/* -*- mode: c++; c-file-style: raknet; tab-always-indent: nil; -*- */
/**
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
 *
 * NEW AND IMPROVED EFFICIENT Queue ADT - By Kevin Jenkins
 * Initilize with the following structure
 * Queue<TYPE, OPTIONAL INITIAL ALLOCATION SIZE>
 *
 * Has the following member functions
 * push - adds an element to the top of the queue
 * pop - returns the bottom element from the queue and removes it.  Result undefined if queue empty
 * peek - returns the bottom element from the queue.  Result undefined if queue empty
 * size - returns the size of the queue
 * compress - reallocates memory to fit the number of elements. Best used when the number of elements decreases
 * clear - empties the queue and returns storage
 * find - returns bool if the specified element is in the queue
 * The assignment and copy constructors are defined
 * 
 * EXAMPLE
 * Queue<int, 20> A;
 * A.push(5);
 * A.push(10);
 * 
 * A.peek(); // Returns 5
 * A.pop(); // Returns 5
 * A.peek(); // Returns 10
 * A.pop();  // Returns 10
 * // At this point the queue is empty
 * 
 * NOTES
 * The default initial allocation size is 1
 * This function doubles the amount of memory allocated when the queue is filled
 * This is better than the linked list version for a queue that doesn't go from one extreme to the other with size changes.
 *
 */


#ifndef __QUEUE_H
#define __QUEUE_H

// Template classes have to have all the code in the header file
#include <assert.h>

namespace BasicDataStructures
{
	/*!	\class Queue
		\brief ADT for a Queue with some extra functionality
	*/
	template <class QueueType> class Queue
	{
	public:
		/// @brief constructor, creates an queue
		Queue();

		/// @brief deconstructor, destroys an queue
		~Queue();

		/// @brief constructor, creates a copy of an queue
		/// @param originalCopy The queue which will be copied
		Queue(const Queue& originalCopy);

		bool operator= ( const Queue& original_copy );
		void push( const QueueType& input );
		void pushAtHead( const QueueType& input );
		QueueType& operator[] ( unsigned int position ) const; // Not a normal thing you do with a queue but can be used for efficiency
		void del( unsigned int position ); // Not a normal thing you do with a queue but can be used for efficiency
		inline const QueueType peek( void ) const;
		inline const QueueType pop( void );
		inline const unsigned int size( void ) const;
		inline const unsigned int AllocationSize( void ) const;
		inline void clear( void );
		void compress( void );
		bool find ( QueueType q );
		void clearAndForceAllocation( int size ); // Force a memory allocation to a certain larger size

	private:
		QueueType* array;
		unsigned int head;  // Array index for the head of the queue
		unsigned int tail; // Array index for the tail of the queue
		unsigned int allocation_size;
	};

	template <class QueueType> Queue<QueueType>::Queue()
	{
		allocation_size = 16;
		array = new QueueType[allocation_size];
		head = 0;
		tail = 0;
	}

	template <class QueueType> Queue<QueueType>::~Queue()
	{
		delete[] array;
	}

	template <class QueueType> Queue<QueueType>::Queue(const Queue& original_copy )
	{
		if ( original_copy.size() == 0 )
		{
			allocation_size = 0;
		}
		else
		{
			array = new QueueType [ original_copy.size() + 1 ];

			for ( unsigned int counter = 0; counter < original_copy.size(); ++counter )
				array[ counter ] = original_copy.array[ ( original_copy.head + counter ) % ( original_copy.allocation_size ) ];

			head = 0;

			tail = original_copy.size();

			allocation_size = original_copy.size() + 1;
		}
	}



	template <class QueueType>
		inline const unsigned int Queue<QueueType>::size( void ) const
	{
		if ( head <= tail )
			return tail -head;
		else
			return allocation_size -head + tail;
	}

	template <class QueueType>
	inline const unsigned int Queue<QueueType>::AllocationSize( void ) const
	{
		return allocation_size;
	}

	template <class QueueType>
		inline const QueueType Queue<QueueType>::pop( void )
	{
#ifdef _DEBUG
		assert( allocation_size > 0 && size() >= 0 && head != tail);
#endif
		//head=(head+1) % allocation_size;

		if ( ++head == allocation_size )
			head = 0;

		if ( head == 0 )
			return ( QueueType ) array[ allocation_size -1 ];

		return ( QueueType ) array[ head -1 ];
	}

	template <class QueueType>
		void Queue<QueueType>::pushAtHead( const QueueType& input )
	{
		if ( allocation_size == 0 )
		{
			array = new QueueType[ 16 ];
			head = 0;
			tail = 1;
			array[ 0 ] = input;
			allocation_size = 16;
			return ;
		}

		if ( head == 0 )
			head = allocation_size - 1;
		else
			--head;

		array[ head ] = input;

		if ( tail == head )
		{
			//  unsigned int index=tail;

			// Need to allocate more memory.
			QueueType * new_array;
			new_array = new QueueType[ allocation_size * 2 ];
#ifdef _DEBUG

			assert( new_array );
#endif

			for ( unsigned int counter = 0; counter < allocation_size; ++counter )
				new_array[ counter ] = array[ ( head + counter ) % ( allocation_size ) ];

			head = 0;

			tail = allocation_size;

			allocation_size *= 2;

			// Delete the old array and move the pointer to the new array
			delete [] array;

			array = new_array;
		}
	}


	template <class QueueType>
		inline const QueueType Queue<QueueType>::peek( void ) const
	{
#ifdef _DEBUG
		assert( head != tail );
		assert( allocation_size > 0 && size() >= 0 );
#endif

		return ( QueueType ) array[ head ];
	}

	template <class QueueType>
		void Queue<QueueType>::push( const QueueType& input )
	{
		if ( allocation_size == 0 )
		{
			array = new QueueType[ 16 ];
			head = 0;
			tail = 1;
			array[ 0 ] = input;
			allocation_size = 16;
			return ;
		}

		array[ tail++ ] = input;

		if ( tail == allocation_size )
			tail = 0;

		if ( tail == head )
		{
			//  unsigned int index=tail;

			// Need to allocate more memory.
			QueueType * new_array;
			new_array = new QueueType[ allocation_size * 2 ];
#ifdef _DEBUG

			assert( new_array );
#endif

			for ( unsigned int counter = 0; counter < allocation_size; ++counter )
				new_array[ counter ] = array[ ( head + counter ) % ( allocation_size ) ];

			head = 0;

			tail = allocation_size;

			allocation_size *= 2;

			// Delete the old array and move the pointer to the new array
			delete [] array;

			array = new_array;
		}

	}



	template <class QueueType>
		bool Queue<QueueType>::operator= ( const Queue& original_copy )
	{
		if ( ( &original_copy ) == this )
			return false;

		clear();

		// Allocate memory for copy
		if ( original_copy.size() == 0 )
		{
			allocation_size = 0;
		}

		else
		{
			array = new QueueType [ original_copy.size() + 1 ];

			for ( unsigned int counter = 0; counter < original_copy.size(); ++counter )
				array[ counter ] = original_copy.array[ ( original_copy.head + counter ) % ( original_copy.allocation_size ) ];

			head = 0;

			tail = original_copy.size();

			allocation_size = original_copy.size() + 1;
		}

		return true;
	}

	template <class QueueType>
		inline void Queue<QueueType>::clear ( void )
	{
		if ( allocation_size == 0 )
			return ;

		if (allocation_size > 32)
		{
			delete[] array;
			allocation_size = 0;
		}

		head = 0;
		tail = 0;
	}

	template <class QueueType>
		void Queue<QueueType>::compress ( void )
	{
		QueueType* new_array;
		unsigned int newAllocationSize;
		if (allocation_size==0)
			return;

		newAllocationSize=1;
		while (newAllocationSize <= size())
			newAllocationSize<<=1; // Must be a better way to do this but I'm too dumb to figure it out quickly :)

		new_array = new QueueType [newAllocationSize];

		for (unsigned int counter=0; counter < size(); ++counter)
			new_array[counter] = array[(head + counter)%(allocation_size)];

		tail=size();
		allocation_size=newAllocationSize;
		head=0;

		// Delete the old array and move the pointer to the new array
		delete [] array;
		array=new_array;
	}

	template <class QueueType>
		bool Queue<QueueType>::find ( QueueType q )
	{
		if ( allocation_size == 0 )
			return false;

		unsigned int counter = head;

		while ( counter != tail )
		{
			if ( array[ counter ] == q )
				return true;

			counter = ( counter + 1 ) % allocation_size;
		}

		return false;
	}

	template <class QueueType>
		void Queue<QueueType>::clearAndForceAllocation( int size )
	{
		delete [] array;
		array = new QueueType[ size ];
		allocation_size = size;
		head = 0;
		tail = 0;
	}

	template <class QueueType>
		inline QueueType& Queue<QueueType>::operator[] ( unsigned int position ) const
	{
#ifdef _DEBUG
		assert( position < size() );
#endif
		//return array[(head + position) % allocation_size];

		if ( head + position >= allocation_size )
			return array[ head + position - allocation_size ];
		else
			return array[ head + position ];
	}

	template <class QueueType>
		void Queue<QueueType>::del( unsigned int position )
	{
#ifdef _DEBUG
		assert( position < size() );
		assert( head != tail );
#endif

		if ( head == tail || position >= size() )
			return ;

		unsigned int index;

		unsigned int next;

		//index  = (head + position) % allocation_size;
		if ( head + position >= allocation_size )
			index = head + position - allocation_size;
		else
			index = head + position;

		//next = (index + 1) % allocation_size;
		next = index + 1;

		if ( next == allocation_size )
			next = 0;

		while ( next != tail )
		{
			// Overwrite the previous element
			array[ index ] = array[ next ];
			index = next;
			//next = (next + 1) % allocation_size;

			if ( ++next == allocation_size )
				next = 0;
		}

		// Move the tail back
		if ( tail == 0 )
			tail = allocation_size - 1;
		else
			--tail;
	}
} // End namespace

#endif

