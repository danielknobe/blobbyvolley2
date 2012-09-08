/*=============================================================================
blobNet
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

#ifndef _QUEUE_HPP_
#define _QUEUE_HPP_

/* Includes */
#include <deque>
#include <algorithm>

namespace BlobNet {
namespace ADT {
	/*!	\class Queue
		\brief ADT for a Queue with some extra functionality
		\note This class needs a cleanup. There are some dummy methods
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
}
}
#endif

