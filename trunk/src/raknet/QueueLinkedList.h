/* -*- mode: c++; c-file-style: raknet; tab-always-indent: nil; -*- */
/*
  QueueLinkedList ADT - By Kevin Jenkins (http://www.rakkar.org)
  Initilize with the following structure
  QueueLinkedList<TYPE, OPTIONAL INITIAL ALLOCATION SIZE>
 
  Has the following member functions
  push - adds an element to the top of the queue
  pop - returns the bottom element from the queue and removes it.  Result undefined if queue empty
  peek - returns the bottom element from the queue.  Result undefined if queue empty
  end_peek - Lets you see the item you most recently added to the queue
  size - returns the size of the queue
  compress - reallocates memory to fit the number of elements. Best used when the number of elements decreases
  clear - empties the queue and returns storage
  The assignment and copy constructors are defined
 
  EXAMPLE
  QueueLinkedList<int, 20> A;
  A.push(5);
  A.push(10);
 
  A.peek(); // Returns 5
  A.pop(); // Returns 5
  A.peek(); // Returns 10
  A.pop();  // Returns 10
// At this point the queue is empty
 
NOTES
The default initial allocation size is 1
This queue uses a linked list to contain elements
 
*/

#ifndef __QUEUE_LINKED_LIST_H
#define __QUEUE_LINKED_LIST_H


#include "LinkedList.h" 
////#include "MemoryManager.h"

namespace BasicDataStructures
{
	template <class QueueType>
	
	class QueueLinkedList
	{
	
	public:
		QueueLinkedList();
		QueueLinkedList( const QueueLinkedList& original_copy );
		bool operator= ( const QueueLinkedList& original_copy );
		QueueType pop( void );
		QueueType& peek( void );
		QueueType& end_peek( void );
		void push( const QueueType& input );
		const unsigned int size( void );
		void clear( void );
		void compress( void );
		
	private:
		LinkedList<QueueType> data;
	};
	
	template <class QueueType>
	QueueLinkedList<QueueType>::QueueLinkedList()
	{
	}
	
	template <class QueueType>
	inline const unsigned int QueueLinkedList<QueueType>::size()
	{
		return data.size();
	}
	
	template <class QueueType>
	inline QueueType QueueLinkedList<QueueType>::pop( void )
	{
		data.beginning();
		return ( QueueType ) data.pop();
	}
	
	template <class QueueType>
	inline QueueType& QueueLinkedList<QueueType>::peek( void )
	{
		data.beginning();
		return ( QueueType ) data.peek();
	}
	
	template <class QueueType>
	inline QueueType& QueueLinkedList<QueueType>::end_peek( void )
	{
		data.end();
		return ( QueueType ) data.peek();
	}
	
	template <class QueueType>
	void QueueLinkedList<QueueType>::push( const QueueType& input )
	{
		data.end();
		data.add( input );
	}
	
	template <class QueueType>
	QueueLinkedList<QueueType>::QueueLinkedList( const QueueLinkedList& original_copy )
	{
		data = original_copy.data;
	}
	
	template <class QueueType>
	bool QueueLinkedList<QueueType>::operator= ( const QueueLinkedList& original_copy )
	{
		if ( ( &original_copy ) == this )
			return false;
			
		data = original_copy.data;
	}
	
	template <class QueueType>
	void QueueLinkedList<QueueType>::clear ( void )
	{
		data.clear();
	}
} // End namespace

#endif
