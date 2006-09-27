/* -*- mode: c++; c-file-style: raknet; tab-always-indent: nil; -*- */
/*
NEW AND IMPROVED EFFICIENT Queue ADT - By Kevin Jenkins
Initilize with the following structure
Queue<TYPE, OPTIONAL INITIAL ALLOCATION SIZE>

Has the following member functions
push - adds an element to the top of the queue
pop - returns the bottom element from the queue and removes it.  Result undefined if queue empty
peek - returns the bottom element from the queue.  Result undefined if queue empty
size - returns the size of the queue
compress - reallocates memory to fit the number of elements. Best used when the number of elements decreases
clear - empties the queue and returns storage
find - returns bool if the specified element is in the queue
The assignment and copy constructors are defined

EXAMPLE
Queue<int, 20> A;
A.push(5);
A.push(10);

A.peek(); // Returns 5
A.pop(); // Returns 5
A.peek(); // Returns 10
A.pop();  // Returns 10
// At this point the queue is empty

NOTES
The default initial allocation size is 1
This function doubles the amount of memory allocated when the queue is filled
This is better than the linked list version for a queue that doesn't go from one extreme to the other with size changes.

*/


#ifndef __QUEUE_H
#define __QUEUE_H

// Template classes have to have all the code in the header file
#include <assert.h>

namespace BasicDataStructures
{
	template <class queue_type>
	class Queue
	{

	public:
		Queue();
		~Queue();
		Queue( Queue& original_copy );
		bool operator= ( const Queue& original_copy );
		void push( const queue_type& input );
		void pushAtHead( const queue_type& input );
		queue_type& operator[] ( unsigned int position ) const; // Not a normal thing you do with a queue but can be used for efficiency
		void del( unsigned int position ); // Not a normal thing you do with a queue but can be used for efficiency
		inline const queue_type peek( void ) const;
		inline const queue_type pop( void );
		inline const unsigned int size( void ) const;
		inline const unsigned int AllocationSize( void ) const;
		inline void clear( void );
		void compress( void );
		bool find ( queue_type q );
		void clearAndForceAllocation( int size ); // Force a memory allocation to a certain larger size

	private:
		queue_type* array;
		unsigned int head;  // Array index for the head of the queue
		unsigned int tail; // Array index for the tail of the queue
		unsigned int allocation_size;
	};


	template <class queue_type>
		inline const unsigned int Queue<queue_type>::size( void ) const
	{
		if ( head <= tail )
			return tail -head;
		else
			return allocation_size -head + tail;
	}

	template <class queue_type>
	inline const unsigned int Queue<queue_type>::AllocationSize( void ) const
	{
		return allocation_size;
	}

	template <class queue_type>
		Queue<queue_type>::Queue()
	{
		allocation_size = 16;
		array = new queue_type[ allocation_size ];
		head = 0L;
		tail = 0L;
	}

	template <class queue_type>
		Queue<queue_type>::~Queue()
	{
		clear();
	}

	template <class queue_type>
		inline const queue_type Queue<queue_type>::pop( void )
	{
#ifdef _DEBUG
		assert( allocation_size > 0 && size() >= 0L && head != tail);
#endif
		//head=(head+1) % allocation_size;

		if ( ++head == allocation_size )
			head = 0;

		if ( head == 0 )
			return ( queue_type ) array[ allocation_size -1 ];

		return ( queue_type ) array[ head -1 ];
	}

	template <class queue_type>
		void Queue<queue_type>::pushAtHead( const queue_type& input )
	{
		if ( allocation_size == 0L )
		{
			array = new queue_type[ 16 ];
			head = 0L;
			tail = 1L;
			array[ 0 ] = input;
			allocation_size = 16L;
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
			queue_type * new_array;
			new_array = new queue_type[ allocation_size * 2L ];
#ifdef _DEBUG

			assert( new_array );
#endif

			for ( unsigned int counter = 0L; counter < allocation_size; ++counter )
				new_array[ counter ] = array[ ( head + counter ) % ( allocation_size ) ];

			head = 0L;

			tail = allocation_size;

			allocation_size *= 2L;

			// Delete the old array and move the pointer to the new array
			delete [] array;

			array = new_array;
		}
	}


	template <class queue_type>
		inline const queue_type Queue<queue_type>::peek( void ) const
	{
#ifdef _DEBUG
		assert( head != tail );
		assert( allocation_size > 0 && size() >= 0L );
#endif

		return ( queue_type ) array[ head ];
	}

	template <class queue_type>
		void Queue<queue_type>::push( const queue_type& input )
	{
		if ( allocation_size == 0L )
		{
			array = new queue_type[ 16 ];
			head = 0L;
			tail = 1L;
			array[ 0 ] = input;
			allocation_size = 16L;
			return ;
		}

		array[ tail++ ] = input;

		if ( tail == allocation_size )
			tail = 0;

		if ( tail == head )
		{
			//  unsigned int index=tail;

			// Need to allocate more memory.
			queue_type * new_array;
			new_array = new queue_type[ allocation_size * 2L ];
#ifdef _DEBUG

			assert( new_array );
#endif

			for ( unsigned int counter = 0L; counter < allocation_size; ++counter )
				new_array[ counter ] = array[ ( head + counter ) % ( allocation_size ) ];

			head = 0L;

			tail = allocation_size;

			allocation_size *= 2L;

			// Delete the old array and move the pointer to the new array
			delete [] array;

			array = new_array;
		}

	}

	template <class queue_type>
		Queue<queue_type>::Queue( Queue& original_copy )
	{
		// Allocate memory for copy

		if ( original_copy.size() == 0L )
		{
			allocation_size = 0L;
		}

		else
		{
			array = new queue_type [ original_copy.size() + 1 ];

			for ( unsigned int counter = 0L; counter < original_copy.size(); ++counter )
				array[ counter ] = original_copy.array[ ( original_copy.head + counter ) % ( original_copy.allocation_size ) ];

			head = 0L;

			tail = original_copy.size();

			allocation_size = original_copy.size() + 1;
		}
	}

	template <class queue_type>
		bool Queue<queue_type>::operator= ( const Queue& original_copy )
	{
		if ( ( &original_copy ) == this )
			return false;

		clear();

		// Allocate memory for copy
		if ( original_copy.size() == 0L )
		{
			allocation_size = 0L;
		}

		else
		{
			array = new queue_type [ original_copy.size() + 1 ];

			for ( unsigned int counter = 0L; counter < original_copy.size(); ++counter )
				array[ counter ] = original_copy.array[ ( original_copy.head + counter ) % ( original_copy.allocation_size ) ];

			head = 0L;

			tail = original_copy.size();

			allocation_size = original_copy.size() + 1;
		}

		return true;
	}

	template <class queue_type>
		inline void Queue<queue_type>::clear ( void )
	{
		if ( allocation_size == 0L )
			return ;

		if (allocation_size > 32)
		{
			delete[] array;
			allocation_size = 0L;
		}

		head = 0L;
		tail = 0L;
	}

	template <class queue_type>
		void Queue<queue_type>::compress ( void )
	{
		queue_type* new_array;
		unsigned int newAllocationSize;
		if (allocation_size==0)
			return;

		newAllocationSize=1;
		while (newAllocationSize <= size())
			newAllocationSize<<=1; // Must be a better way to do this but I'm too dumb to figure it out quickly :)

		new_array = new queue_type [newAllocationSize];

		for (unsigned int counter=0L; counter < size(); ++counter)
			new_array[counter] = array[(head + counter)%(allocation_size)];

		tail=size();
		allocation_size=newAllocationSize;
		head=0L;

		// Delete the old array and move the pointer to the new array
		delete [] array;
		array=new_array;
	}

	template <class queue_type>
		bool Queue<queue_type>::find ( queue_type q )
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

	template <class queue_type>
		void Queue<queue_type>::clearAndForceAllocation( int size )
	{
		delete [] array;
		array = new queue_type[ size ];
		allocation_size = size;
		head = 0L;
		tail = 0L;
	}

	template <class queue_type>
		inline queue_type& Queue<queue_type>::operator[] ( unsigned int position ) const
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

	template <class queue_type>
		void Queue<queue_type>::del( unsigned int position )
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

