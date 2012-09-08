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

#include <deque>
#include <algorithm>

#ifndef __QUEUE_H
#define __QUEUE_H

namespace BasicDataStructures
{

	const unsigned int initAllocationSize = 16; 


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

		const QueueType& operator[] (unsigned int position) const;
		QueueType& operator[] (unsigned int position);

		bool operator= ( const Queue& original_copy );

		/// @brief Count of elements in the queue
		/// @return Count of elements
		inline const unsigned int size() const;

		/// @brief Adds an element to the queue
		/// @param input Element to add
		void push(const QueueType& input);

		/// @brief Adds an element to the head of the queue
		/// @param input Element to add
		void pushAtHead(const QueueType& input);

		/// @brief Pops the first element of queue. Check if queue is not empty before.
		/// @return First element of queue
		inline const QueueType pop();

		/// @brief Returns the internal size of the allocated array
		/// @return Internal array size
		inline const unsigned int AllocationSize() const;

		/// @brief Deletes all elements of the array
		inline void clear();

		/// @brief Reorganizes the queue. Really dump implemented at the moment
		void compress();

		/// @brief Checks if element is in queue or not
		/// @param q Element
		bool find(QueueType q);

		/// @brief Returns first element of queue. Really dump implemented at the moment
		/// @return First element
		inline const QueueType peek() const;

		/// @brief Deletes an element. This is not very fast
		/// @param position Index of element
		void del(unsigned int position);

		/// @brief Reallocates the queue with no elements
		/// @param size Size of internal array size
		void clearAndForceAllocation(int size);

	private:
		unsigned int head; // First element with data
		unsigned int tail; // First element without data
		unsigned int allocation_size;

		typedef std::deque<QueueType> ContainerType;
		ContainerType array;

	};

	template <class QueueType> Queue<QueueType>::Queue()
	{
	}

	template <class QueueType> Queue<QueueType>::~Queue()
	{
	}

	template <class QueueType> Queue<QueueType>::Queue(const Queue& original_copy)
	{
		this->array = original_copy->array;
	}

	template <class QueueType> inline const QueueType& Queue<QueueType>::operator[] (unsigned int position) const
	{
		return this->array.at(position);
	}

	template <class QueueType> inline QueueType& Queue<QueueType>::operator[] (unsigned int position)
	{
		return this->array.at(position);
	}

	template <class QueueType> bool Queue<QueueType>::operator= (const Queue& original_copy)
	{
		return this->array = original_copy->array;
	}

	template <class QueueType> inline const unsigned int Queue<QueueType>::size() const
	{
		return this->array.size();
	}

	template <class QueueType> void Queue<QueueType>::push(const QueueType& input)
	{
		this->array.push_back(input);
	}

	template <class QueueType> void Queue<QueueType>::pushAtHead(const QueueType& input)
	{
		this->array.push_front(input);
	}

	template <class QueueType> inline const QueueType Queue<QueueType>::pop()
	{
		QueueType tmp = this->array.front();
		this->array.pop_front();
		
		return tmp;
	}

	template <class QueueType> inline const unsigned int Queue<QueueType>::AllocationSize() const
	{
		return this->array.max_size();
	}

	template <class QueueType> inline void Queue<QueueType>::clear()
	{
		this->array.clear();
	}

	template <class QueueType> void Queue<QueueType>::compress()
	{
	}

	template <class QueueType> bool Queue<QueueType>::find(QueueType q)
	{
		typename ContainerType::iterator it;
		it = std::find(this->array.begin(), this->array.end(), q);

		return it != this->array.end();
	}

	template <class QueueType> inline const QueueType Queue<QueueType>::peek( void ) const
	{
		return this->array.front();
	}

	template <class QueueType> void Queue<QueueType>::del(unsigned int position)
	{
		this->array.erase(this->array.begin() + position);
	}

	template <class QueueType> void Queue<QueueType>::clearAndForceAllocation(int size)
	{
		return this->array.clear();
	}
} // End namespace

#endif

