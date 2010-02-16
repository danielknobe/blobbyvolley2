/* -*- mode: c++; c-file-style: raknet; tab-always-indent: nil; -*- */
 /**
 * @file 
 * @brief RakPeer Implementation 
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

#ifndef __SINGLE_PRODUCER_CONSUMER_H
#define __SINGLE_PRODUCER_CONSUMER_H

#include <assert.h>

static const int MINIMUM_LIST_SIZE=8;

namespace BasicDataStructures
{
	template <class SingleProducerConsumerType>
	class SingleProducerConsumer
	{
	public:
		SingleProducerConsumer();
		~SingleProducerConsumer();

		// WriteLock must be immediately followed by WriteUnlock.  These two functions must be called in the same thread.
		SingleProducerConsumerType* WriteLock(void);
		// Cancelling locks cancels all locks back up to the data passed.  So if you lock twice and cancel using the first lock, the second lock is ignored
		void CancelWriteLock(SingleProducerConsumerType* cancelToLocation);
		void WriteUnlock(void);
		// ReadLock must be immediately followed by ReadUnlock. These two functions must be called in the same thread.
		SingleProducerConsumerType* ReadLock(void);
		// Cancelling locks cancels all locks back up to the data passed.  So if you lock twice and cancel using the first lock, the second lock is ignored
		void CancelReadLock(SingleProducerConsumerType* cancelToLocation);
		void ReadUnlock(void);
		// Clear is not thread-safe and none of the lock or unlock functions should be called while it is running.
		void Clear(void);
		int Size(void) const; // An ESTIMATE of how many data elements are waiting to be read
	private:
		struct DataPlusPtr
		{
			SingleProducerConsumerType object;
			DataPlusPtr *next;
		};
		DataPlusPtr *readPointer, *readAheadPointer, *writePointer, *writeAheadPointer;
		int listSize, dataSize;
	};

	template <class SingleProducerConsumerType>
	SingleProducerConsumer<SingleProducerConsumerType>::SingleProducerConsumer()
	{
		// Preallocate
		readPointer = new DataPlusPtr;
		writePointer=readPointer;
		readPointer->next = new DataPlusPtr;
#ifdef _DEBUG
		assert(MINIMUM_LIST_SIZE>=3);
#endif
		for (listSize=2; listSize < MINIMUM_LIST_SIZE; listSize++)
		{
			readPointer=readPointer->next;
			readPointer->next = new DataPlusPtr;
		}
		listSize=MINIMUM_LIST_SIZE;
		readPointer->next->next=writePointer; // last to next = start
		readPointer=writePointer;
		readAheadPointer=readPointer;
		writeAheadPointer=writePointer;
		dataSize=0;
	}

	template <class SingleProducerConsumerType>
	SingleProducerConsumer<SingleProducerConsumerType>::~SingleProducerConsumer()
	{
		DataPlusPtr *next;
		readPointer=writeAheadPointer->next;
		while (readPointer!=writeAheadPointer)
		{
			next=readPointer->next;
			delete readPointer;
			readPointer=next;
		}
		delete readPointer;
	}

	template <class SingleProducerConsumerType>
	SingleProducerConsumerType* SingleProducerConsumer<SingleProducerConsumerType>::WriteLock( void )
	{
		if (writeAheadPointer->next==readPointer)
		{
			DataPlusPtr *originalNext=writeAheadPointer->next;
			writeAheadPointer->next=new DataPlusPtr;
			writeAheadPointer->next->next=originalNext;
			++listSize;
		}

		DataPlusPtr *last;
		last=writeAheadPointer;
		writeAheadPointer=writeAheadPointer->next;
		return (SingleProducerConsumerType*) last;
	}

	template <class SingleProducerConsumerType>
	void SingleProducerConsumer<SingleProducerConsumerType>::CancelWriteLock( SingleProducerConsumerType* cancelToLocation )
	{
		writeAheadPointer=(DataPlusPtr *)cancelToLocation;
	}

	template <class SingleProducerConsumerType>
	void SingleProducerConsumer<SingleProducerConsumerType>::WriteUnlock( void )
	{
	//	DataPlusPtr *dataContainer = (DataPlusPtr *)structure;

#ifdef _DEBUG
		assert(writePointer->next!=readPointer);
		assert(writePointer!=writeAheadPointer);
#endif

		++dataSize;
		// User is done with the data, allow send by updating the write pointer
		writePointer=writePointer->next;
	}

	template <class SingleProducerConsumerType>
		SingleProducerConsumerType* SingleProducerConsumer<SingleProducerConsumerType>::ReadLock( void )
	{
		if (readAheadPointer==writePointer)
			return 0;

		DataPlusPtr *last;
		last=readAheadPointer;
		readAheadPointer=readAheadPointer->next;
		return (SingleProducerConsumerType*)last;
	}

	template <class SingleProducerConsumerType>
		void SingleProducerConsumer<SingleProducerConsumerType>::CancelReadLock( SingleProducerConsumerType* cancelToLocation )
	{
#ifdef _DEBUG
		assert(readPointer!=writePointer);
#endif
		readAheadPointer=(DataPlusPtr *)cancelToLocation;
	}

	template <class SingleProducerConsumerType>
	void SingleProducerConsumer<SingleProducerConsumerType>::ReadUnlock( void )
	{
#ifdef _DEBUG
		assert(readAheadPointer!=readPointer); // If hits, then called ReadUnlock before ReadLock
		assert(readPointer!=writePointer); // If hits, then called ReadUnlock when Read returns 0
#endif
		--dataSize;

		readPointer=readPointer->next;
	}
	
	template <class SingleProducerConsumerType>
	void SingleProducerConsumer<SingleProducerConsumerType>::Clear( void )
	{
		// Shrink the list down to MINIMUM_LIST_SIZE elements
		DataPlusPtr *next;
		writePointer=readPointer->next;

		while (listSize-- > MINIMUM_LIST_SIZE)
		{
			next=writePointer->next;
			delete writePointer;
			writePointer=next;
		}

		readPointer->next=writePointer;
		writePointer=readPointer;
		readAheadPointer=readPointer;
		writeAheadPointer=writePointer;
		dataSize=0;
	}

	template <class SingleProducerConsumerType>
	int SingleProducerConsumer<SingleProducerConsumerType>::Size( void ) const
	{
		return dataSize;
	}
}

#endif

