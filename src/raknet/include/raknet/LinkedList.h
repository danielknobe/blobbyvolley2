/* -*- mode: c++; c-file-style: raknet; tab-always-indent: nil; -*- */
/**
 * @file 
 * 
 * @brief LinkedList and Circular Linked List 
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
 *  
 * 9/04 Giblet - updated code to work with new compilers adhering more 
 *   closely to ISO standard
 *
 */

#ifndef __LINKED_LIST_H
#define __LINKED_LIST_H 
/**
* @brief Basic Data Structures (containers) 
* 
* This namespace contains containers used in RakNet.
*/

namespace BasicDataStructures
{

	// Prototype to prevent error in CircularLinkedList class when a reference is made to a LinkedList class

	template <class LinkedListType>

	class LinkedList;

	/**
	* (Circular) Linked List ADT (Doubly Linked Pointer to Node Style) - 
	* By Kevin Jenkins (http://www.rakkar.org)
	* Initilize with the following command
	* LinkedList<TYPE>
	* OR
	* CircularLinkedList<Type>
	*
	* Has the following member functions
	* - size: returns number of elements in the linked list
	* - insert(item):  inserts @em item at the current position in 
	*   the LinkedList.
	* - add(item): inserts @em item after the current position in 
	*   the LinkedList.  Does not increment the position
	* - replace(item): replaces the element at the current position @em item.
	* - peek:  returns the element at the current position
	* - pop:  returns the element at the current position and deletes it
	* - del: deletes the current element. Does nothing for an empty list.
	* - clear:  empties the LinkedList and returns storage
	* - bool is_in(item): Does a linear search for @em item.  Does not set 
	*   the position to it, only returns true on item found, false otherwise
	* - bool find(item): Does a linear search for @em item and sets the current 
	*   position to point to it if and only if the item is found. Returns true 
	*   on item found, false otherwise
	* - sort: Sorts the elements of the list with a mergesort and sets the 
	*   current pointer to the first element
	* - concatenate(list L): This appends L to the current list
	* - ++(prefix): moves the pointer one element up in the list and returns the 
	*   appropriate copy of the element in the list
	* - --(prefix): moves the pointer one element back in the list and returns 
	*   the appropriate copy of the element in the list
	* - beginning - moves the pointer to the start of the list.  For circular 
	*   linked lists this is first 'position' created.  You should call this 
	*   after the sort function to read the first value.
	* - end - moves the pointer to the end of the list.  For circular linked 
	*   lists this is one less than the first 'position' created
	* The assignment and copy constructor operators are defined
	*
	* @note 
	* 1. LinkedList and CircularLinkedList are exactly the same except LinkedList 
	*    won't let you wrap around the root and lets you jump to two positions 
	*    relative to the root/
	* 2. Postfix ++ and -- can be used but simply call the prefix versions.
	*
	*
	* EXAMPLE:
	* @code
	* LinkedList<int> A;  // Creates a Linked List of integers called A
	* CircularLinkedList<int> B;  // Creates a Circular Linked List of 
	*          // integers called B
	*
	* A.insert(20);  // Adds 20 to A.  A: 20 - current is 20
	* A.insert(5);  // Adds 5 to A.  A: 5 20 - current is 5
	* A.insert(1);  // Adds 1 to A.  A: 1 5 20 - current is 1
	*
	* A.is_in(1); // returns true
	* A.is_in(200); // returns false
	* A.find(5);  // returns true and sets current to 5
	* A.peek();  // returns 5
	* A.find(1);  // returns true and sets current to 1
	*
	* (++A).peek();  // Returns 5
	* A.peek(); // Returns 5
	*
	* A.replace(10);  // Replaces 5 with 10.
	* A.peek();  // Returns 10
	*
	* A.beginning();  // Current points to the beginning of the list at 1
	*
	* (++A).peek();  // Returns 5
	* A.peek();  // Returns 10
	*
	* A.del();  // Deletes 10.  Current points to the next element, which is 20
	* A.peek();  // Returns 20
	* 
	* A.beginning();  // Current points to the beginning of the list at 1
	*
	* (++A).peek();  // Returns 5
	* A.peek();  // Returns 20
	*
	* A.clear();  // Deletes all nodes in A
	*
	* A.insert(5);  // A: 5 - current is 5
	* A.insert(6); // A: 6 5 - current is 6
	* A.insert(7); // A: 7 6 5 - current is 7
	*
	* A.clear();
	* B.clear();
	*
	* B.add(10);
	* B.add(20);
	* B.add(30);
	* B.add(5);
	* B.add(2);
	* B.add(25);
	* // Sorts the numbers in the list and sets the current pointer to the 
	* // first element
	* B.sort();  
	*
	* // Postfix ++ just calls the prefix version and has no functional 
	* // difference.
	* B.peek();  // Returns 2
	* B++;
	* B.peek();  // Returns 5
	* B++;
	* B.peek();  // Returns 10
	* B++;
	* B.peek();  // Returns 20
	* B++;
	* B.peek();  // Returns 25
	* B++;
	* B.peek();  // Returns 30
	* @endcode
	*/
	template <class CircularLinkedListType>

	class CircularLinkedList
	{

	public:

		struct node
		{
			CircularLinkedListType item;

			node* previous;
			node* next;
		};

		CircularLinkedList();
		~CircularLinkedList();
		CircularLinkedList( const CircularLinkedList& original_copy );
		// CircularLinkedList(LinkedList<CircularLinkedListType> original_copy) {CircularLinkedList(original_copy);}  // Converts linked list to circular type
		bool operator= ( const CircularLinkedList& original_copy );
		CircularLinkedList& operator++();  // CircularLinkedList A; ++A;
		CircularLinkedList& operator++( int );  // Circular_Linked List A; A++;
		CircularLinkedList& operator--();  // CircularLinkedList A; --A;
		CircularLinkedList& operator--( int );  // Circular_Linked List A; A--;
		bool is_in( const CircularLinkedListType& input );
		bool find( const CircularLinkedListType& input );
		void insert( const CircularLinkedListType& input );

		CircularLinkedListType& add ( const CircularLinkedListType& input )

			; // Adds after the current position
		void replace( const CircularLinkedListType& input );

		void del( void );

		unsigned int size( void );

		CircularLinkedListType& peek( void );

		const CircularLinkedListType pop( void );

		void clear( void );

		void sort( void );

		void beginning( void );

		void end( void );

		void concatenate( const CircularLinkedList& L );

	protected:
		unsigned int list_size;

		node *root;

		node *position;

		node* find_pointer( const CircularLinkedListType& input );

	private:
		CircularLinkedList merge( CircularLinkedList L1, CircularLinkedList L2 );

		CircularLinkedList mergesort( const CircularLinkedList& L );
	};

	template <class LinkedListType>

	class LinkedList : public CircularLinkedList<LinkedListType>
	{

	public:
		LinkedList()
		{}

		LinkedList( const LinkedList& original_copy );
		~LinkedList();
		bool operator= ( const LinkedList<LinkedListType>& original_copy );
		LinkedList& operator++();  // LinkedList A; ++A;
		LinkedList& operator++( int );  // Linked List A; A++;
		LinkedList& operator--();  // LinkedList A; --A;
		LinkedList& operator--( int );  // Linked List A; A--;

	private:
		LinkedList merge( LinkedList L1, LinkedList L2 );
		LinkedList mergesort( const LinkedList& L );

	};


	template <class CircularLinkedListType>
		inline void CircularLinkedList<CircularLinkedListType>::beginning( void )
	{
		if ( this->root )
			this->position = this->root;
	}

	template <class CircularLinkedListType>
		inline void CircularLinkedList<CircularLinkedListType>::end( void )
	{
		if ( this->root )
			this->position = this->root->previous;
	}

	template <class LinkedListType>
		bool LinkedList<LinkedListType>::operator= ( const LinkedList<LinkedListType>& original_copy )
	{
		typename LinkedList::node * original_copy_pointer, *save_position;

		if ( ( &original_copy ) != this )
		{

			this->clear();


			if ( original_copy.list_size == 0 )
			{
				this->root = 0;
				this->position = 0;
				this->list_size = 0;
			}

			else
				if ( original_copy.list_size == 1 )
				{
					this->root = new typename LinkedList::node;
					// root->item = new LinkedListType;
					this->root->next = this->root;
					this->root->previous = this->root;
					this->list_size = 1;
					this->position = this->root;
					// *(root->item)=*((original_copy.root)->item);
					this->root->item = original_copy.root->item;
				}

				else
				{
					// Setup the first part of the root node
					original_copy_pointer = original_copy.root;
					this->root = new typename LinkedList::node;
					// root->item = new LinkedListType;
					this->position = this->root;
					// *(root->item)=*((original_copy.root)->item);
					this->root->item = original_copy.root->item;

					if ( original_copy_pointer == original_copy.position )
						save_position = this->position;

					do
					{


						// Save the current element
						this->last = this->position;

						// Point to the next node in the source list
						original_copy_pointer = original_copy_pointer->next;

						// Create a new node and point position to it
						this->position = new typename LinkedList::node;
						// position->item = new LinkedListType;

						// Copy the item to the new node
						// *(position->item)=*(original_copy_pointer->item);
						this->position->item = original_copy_pointer->item;

						if ( original_copy_pointer == original_copy.position )
							save_position = this->position;


						// Set the previous pointer for the new node
						( this->position->previous ) = this->last;

						// Set the next pointer for the old node to the new node
						( this->last->next ) = this->position;

					}

					while ( ( original_copy_pointer->next ) != ( original_copy.root ) );

					// Complete the circle.  Set the next pointer of the newest node to the root and the previous pointer of the root to the newest node
					this->position->next = this->root;

					this->root->previous = this->position;

					this->list_size = original_copy.list_size;

					this->position = save_position;
				}
		}

		return true;
	}


	template <class CircularLinkedListType>
		CircularLinkedList<CircularLinkedListType>::CircularLinkedList()
	{
		this->root = 0;
		this->position = 0;
		this->list_size = 0;
	}

	template <class CircularLinkedListType>
		CircularLinkedList<CircularLinkedListType>::~CircularLinkedList()
	{
		this->clear();
	}

	template <class LinkedListType>
		LinkedList<LinkedListType>::~LinkedList()
	{
		this->clear();
	}

	template <class LinkedListType>
		LinkedList<LinkedListType>::LinkedList( const LinkedList& original_copy )
	{
		typename LinkedList::node * original_copy_pointer, *last, *save_position;

		if ( original_copy.list_size == 0 )
		{
			this->root = 0;
			this->position = 0;
			this->list_size = 0;
			return ;
		}

		else
			if ( original_copy.list_size == 1 )
			{
				this->root = new typename LinkedList::node;
				// root->item = new CircularLinkedListType;
				this->root->next = this->root;
				this->root->previous = this->root;
				this->list_size = 1;
				this->position = this->root;
				// *(root->item) = *((original_copy.root)->item);
				this->root->item = original_copy.root->item;
			}

			else
			{
				// Setup the first part of the root node
				original_copy_pointer = original_copy.root;
				this->root = new typename LinkedList::node;
				// root->item = new CircularLinkedListType;
				this->position = this->root;
				// *(root->item)=*((original_copy.root)->item);
				this->root->item = original_copy.root->item;

				if ( original_copy_pointer == original_copy.position )
					save_position = this->position;

				do
				{
					// Save the current element
					this->last = this->position;

					// Point to the next node in the source list
					original_copy_pointer = original_copy_pointer->next;

					// Create a new node and point position to it
					this->position = new typename LinkedList::node;
					// position->item = new CircularLinkedListType;

					// Copy the item to the new node
					// *(position->item)=*(original_copy_pointer->item);
					this->position->item = original_copy_pointer->item;

					if ( original_copy_pointer == original_copy.position )
						save_position = this->position;

					// Set the previous pointer for the new node
					( this->position->previous ) = last;

					// Set the next pointer for the old node to the new node
					( this->last->next ) = this->position;

				}

				while ( ( original_copy_pointer->next ) != ( original_copy.root ) );

				// Complete the circle.  Set the next pointer of the newest node to the root and the previous pointer of the root to the newest node
				this->position->next = this->root;

				this->root->previous = this->position;

				this->list_size = original_copy.list_size;

				this->position = save_position;
			}
	}

	template <class CircularLinkedListType>
		CircularLinkedList<CircularLinkedListType>::CircularLinkedList( const CircularLinkedList& original_copy )
	{
		node * original_copy_pointer;
		node *save_position;

		if ( original_copy.list_size == 0 )
		{
			this->root = 0;
			this->position = 0;
			this->list_size = 0;
			return ;
		}

		else
			if ( original_copy.list_size == 1 )
			{
				this->root = new typename CircularLinkedList::node;
				// root->item = new CircularLinkedListType;
				this->root->next = this->root;
				this->root->previous = this->root;
				this->list_size = 1;
				this->position = this->root;
				// *(root->item) = *((original_copy.root)->item);
				this->root->item = original_copy.root->item;
			}

			else
			{
				// Setup the first part of the root node
				original_copy_pointer = original_copy.root;
				this->root = new typename CircularLinkedList::node;
				// root->item = new CircularLinkedListType;
				this->position = this->root;
				// *(root->item)=*((original_copy.root)->item);
				this->root->item = original_copy.root->item;

				if ( original_copy_pointer == original_copy.position )
					save_position = this->position;

				do
				{


					// Save the current element
					this->last = this->position;

					// Point to the next node in the source list
					original_copy_pointer = original_copy_pointer->next;

					// Create a new node and point position to it
					this->position = new typename CircularLinkedList::node;
					// position->item = new CircularLinkedListType;

					// Copy the item to the new node
					// *(position->item)=*(original_copy_pointer->item);
					this->position->item = original_copy_pointer->item;

					if ( original_copy_pointer == original_copy.position )
						save_position = position;

					// Set the previous pointer for the new node
					( this->position->previous ) = this->last;

					// Set the next pointer for the old node to the new node
					( this->last->next ) = this->position;

				}

				while ( ( original_copy_pointer->next ) != ( original_copy.root ) );

				// Complete the circle.  Set the next pointer of the newest node to the root and the previous pointer of the root to the newest node
				this->position->next = this->root;

				this->root->previous = position;

				this->list_size = original_copy.list_size;

				this->position = save_position;
			}
	}

	template <class CircularLinkedListType>
		bool CircularLinkedList<CircularLinkedListType>::operator= ( const CircularLinkedList& original_copy )
	{
		node * original_copy_pointer;
		node *save_position;

		if ( ( &original_copy ) != this )
		{

			this->clear();


			if ( original_copy.list_size == 0 )
			{
				this->root = 0;
				this->position = 0;
				this->list_size = 0;
			}

			else
				if ( original_copy.list_size == 1 )
				{
					this->root = new typename CircularLinkedList::node;
					// root->item = new CircularLinkedListType;
					this->root->next = this->root;
					this->root->previous = this->root;
					this->list_size = 1;
					this->position = this->root;
					// *(root->item)=*((original_copy.root)->item);
					this->root->item = original_copy.root->item;
				}

				else
				{
					// Setup the first part of the root node
					original_copy_pointer = original_copy.root;
					this->root = new typename CircularLinkedList::node;
					// root->item = new CircularLinkedListType;
					this->position = this->root;
					// *(root->item)=*((original_copy.root)->item);
					this->root->item = original_copy.root->item;

					if ( original_copy_pointer == original_copy.position )
						save_position = this->position;

					do
					{
						// Save the current element
						this->last = this->position;

						// Point to the next node in the source list
						original_copy_pointer = original_copy_pointer->next;

						// Create a new node and point position to it
						this->position = new typename CircularLinkedList::node;
						// position->item = new CircularLinkedListType;

						// Copy the item to the new node
						// *(position->item)=*(original_copy_pointer->item);
						this->position->item = original_copy_pointer->item;

						if ( original_copy_pointer == original_copy.position )
							save_position = this->position;

						// Set the previous pointer for the new node
						( this->position->previous ) = this->last;

						// Set the next pointer for the old node to the new node
						( this->last->next ) = this->position;

					}

					while ( ( original_copy_pointer->next ) != ( original_copy.root ) );

					// Complete the circle.  Set the next pointer of the newest node to the root and the previous pointer of the root to the newest node
					this->position->next = this->root;

					this->root->previous = this->position;

					this->list_size = original_copy.list_size;

					this->position = save_position;
				}
		}

		return true;
	}

	template <class CircularLinkedListType>
		void CircularLinkedList<CircularLinkedListType>::insert( const CircularLinkedListType& input )
	{
		node * new_node;

		if ( list_size == 0 )
		{
			this->root = new typename CircularLinkedList::node;
			// root->item = new CircularLinkedListType;
			//*(root->item)=input;
			this->root->item = input;
			this->root->next = this->root;
			this->root->previous = this->root;
			this->list_size = 1;
			this->position = this->root;
		}

		else
			if ( list_size == 1 )
			{
				this->position = new typename CircularLinkedList::node;
				// position->item = new CircularLinkedListType;
				this->root->next = this->position;
				this->root->previous = this->position;
				this->position->previous = this->root;
				this->position->next = this->root;
				// *(position->item)=input;
				this->position->item = input;
				this->root = this->position; // Since we're inserting into a 1 element list the old root is now the second item
				this->list_size = 2;
			}

			else
			{
				/*

				B
				|
				A --- C

				position->previous=A
				new_node=B
				position=C

				Note that the order of the following statements is important  */

				new_node = new typename CircularLinkedList::node;
				// new_node->item = new CircularLinkedListType;

				// *(new_node->item)=input;
				new_node->item = input;

				// Point next of A to B
				( this->position->previous ) ->next = new_node;

				// Point last of B to A
				new_node->previous = this->position->previous;

				// Point last of C to B
				this->position->previous = new_node;

				// Point next of B to C
				new_node->next = this->position;

				// Since the root pointer is bound to a node rather than an index this moves it back if you insert an element at the root

				if ( this->position == this->root )
				{
					this->root = new_node;
					this->position = this->root;
				}

				// Increase the recorded size of the list by one
				this->list_size++;
			}
	}

	template <class CircularLinkedListType>
		CircularLinkedListType& CircularLinkedList<CircularLinkedListType>::add ( const CircularLinkedListType& input )
	{
		node * new_node;

		if ( this->list_size == 0 )
		{
			this->root = new typename CircularLinkedList::node;
			// root->item = new CircularLinkedListType;
			// *(root->item)=input;
			this->root->item = input;
			this->root->next = this->root;
			this->root->previous = this->root;
			this->list_size = 1;
			this->position = this->root;
			// return *(position->item);
			return this->position->item;
		}

		else
			if ( list_size == 1 )
			{
				this->position = new typename CircularLinkedList::node;
				// position->item = new CircularLinkedListType;
				this->root->next = this->position;
				this->root->previous = this->position;
				this->position->previous = this->root;
				this->position->next = this->root;
				// *(position->item)=input;
				this->position->item = input;
				this->list_size = 2;
				this->position = this->root; // Don't move the position from the root
				// return *(position->item);
				return this->position->item;
			}

			else
			{
				/*

				   B
			       |
				A --- C

				new_node=B
				position=A
				position->next=C

				Note that the order of the following statements is important  */

				new_node = new typename CircularLinkedList::node;
				// new_node->item = new CircularLinkedListType;

				// *(new_node->item)=input;
				new_node->item = input;

				// Point last of B to A
				new_node->previous = this->position;

				// Point next of B to C
				new_node->next = ( this->position->next );

				// Point last of C to B
				( this->position->next ) ->previous = new_node;

				// Point next of A to B
				( this->position->next ) = new_node;

				// Increase the recorded size of the list by one
				this->list_size++;

				// return *(new_node->item);
				return new_node->item;
			}
	}

	template <class CircularLinkedListType>
		inline void CircularLinkedList<CircularLinkedListType>::replace( const CircularLinkedListType& input )
	{
		if ( this->list_size > 0 )
			// *(position->item)=input;
			this->position->item = input;
	}

	template <class CircularLinkedListType>
		void CircularLinkedList<CircularLinkedListType>::del()
	{
		node * new_position;

		if ( this->list_size == 0 )
			return ;

		else
			if ( this->list_size == 1 )
			{
				// delete root->item;
				delete this->root;
				this->root = this->position = 0;
				this->list_size = 0;
			}

			else
			{
				( this->position->previous ) ->next = this->position->next;
				( this->position->next ) ->previous = this->position->previous;
				new_position = this->position->next;

				if ( this->position == this->root )
					this->root = new_position;

				// delete position->item;
				delete this->position;

				this->position = new_position;

				this->list_size--;
			}
	}

	template <class CircularLinkedListType>
		bool CircularLinkedList<CircularLinkedListType>::is_in( const CircularLinkedListType& input )
	{
		node * return_value, *old_position;

		old_position = this->position;

		return_value = find_pointer( input );
		this->position = old_position;

		if ( return_value != 0 )
			return true;
		else
			return false; // Can't find the item don't do anything
	}

	template <class CircularLinkedListType>
		bool CircularLinkedList<CircularLinkedListType>::find( const CircularLinkedListType& input )
	{
		node * return_value;

		return_value = find_pointer( input );

		if ( return_value != 0 )
		{
			this->position = return_value;
			return true;
		}

		else
			return false; // Can't find the item don't do anything
	}

	template <class CircularLinkedListType>
		typename CircularLinkedList<CircularLinkedListType>::node* CircularLinkedList<CircularLinkedListType>::find_pointer( const CircularLinkedListType& input )
	{
		node * current;

		if ( this->list_size == 0 )
			return 0;

		current = this->root;

		// Search for the item starting from the root node and incrementing the pointer after every check
		// If you wind up pointing at the root again you looped around the list so didn't find the item, in which case return 0
		do
		{
			// if (*(current->item) == input) return current;

			if ( current->item == input )
				return current;

			current = current->next;
		}

		while ( current != this->root );

		return 0;

	}

	template <class CircularLinkedListType>
		inline unsigned int CircularLinkedList<CircularLinkedListType>::size( void )
	{
		return this->list_size;
	}

	template <class CircularLinkedListType>
		inline CircularLinkedListType& CircularLinkedList<CircularLinkedListType>::peek( void )
	{
		// return *(position->item);
		return this->position->item;
	}

	template <class CircularLinkedListType>
		const CircularLinkedListType CircularLinkedList<CircularLinkedListType>::pop( void )
	{
		CircularLinkedListType element;
		element = peek();
		del();
		return CircularLinkedListType( element ); // return temporary
	}

	// Prefix
	template <class CircularLinkedListType>
		CircularLinkedList<CircularLinkedListType>& CircularLinkedList<CircularLinkedListType>::operator++()
	{
		if ( this->list_size != 0 )
			position = position->next;

		return *this;
	}

	/*
	// Postfix
	template <class CircularLinkedListType>
	CircularLinkedList<CircularLinkedListType>& CircularLinkedList<CircularLinkedListType>::operator++(int)
	{
	CircularLinkedList<CircularLinkedListType> before;
	before=*this;
	operator++();
	return before;
	}
	*/

	template <class CircularLinkedListType>
		CircularLinkedList<CircularLinkedListType>& CircularLinkedList<CircularLinkedListType>::operator++( int )
	{
		return this->operator++();
	}

	// Prefix
	template <class CircularLinkedListType>
		CircularLinkedList<CircularLinkedListType>& CircularLinkedList<CircularLinkedListType>::operator--()
	{
		if ( this->list_size != 0 )
			this->position = this->position->previous;

		return *this;
	}

	/*
	// Postfix
	template <class CircularLinkedListType>
	CircularLinkedList<CircularLinkedListType>& CircularLinkedList<CircularLinkedListType>::operator--(int)
	{
	CircularLinkedList<CircularLinkedListType> before;
	before=*this;
	operator--();
	return before;
	}
	*/

	template <class CircularLinkedListType>
		CircularLinkedList<CircularLinkedListType>& CircularLinkedList<CircularLinkedListType>::operator--( int )
	{
		return this->operator--();
	}

	template <class CircularLinkedListType>
		void CircularLinkedList<CircularLinkedListType>::clear( void )
	{
		if ( this->list_size == 0 )
		{
			return ;
		}
		else
		{
			if ( this->list_size == 1 )  // {delete root->item; delete root;}
			{
				delete this->root;
			}
			else
			{
				node* current;

				current = this->root;

				do
				{
					node* temp = current;
					current = current->next;
					// delete temp->item;
					delete temp;
				}
				while ( current != this->root );
			}

			this->list_size = 0;
			this->root = 0;
			this->position = 0;
		}
	}

	template <class CircularLinkedListType>
		inline void CircularLinkedList<CircularLinkedListType>::concatenate( const CircularLinkedList<CircularLinkedListType>& L )
	{
		unsigned int counter;
		node* ptr;

		if ( L.list_size == 0 )
			return ;

		if ( this->list_size == 0 )
			* this = L;

		ptr = L.root;

		this->position = this->root->previous;

		// Cycle through each element in L and add it to the current list
		for ( counter = 0; counter < L.list_size; counter++ )
		{
			// Add item after the current item pointed to
			// add(*(ptr->item));

			add ( ptr->item )

				;

			// Update pointers.  Moving ptr keeps the current pointer at the end of the list since the add function does not move the pointer
			ptr = ptr->next;

			this->position = this->position->next;
		}
	}

	template <class CircularLinkedListType>
		inline void CircularLinkedList<CircularLinkedListType>::sort( void )
	{
		if ( this->list_size <= 1 )
			return ;

		// Call equal operator to assign result of mergesort to current object
		*this = mergesort( *this );

		this->position = this->root;
	}

	template <class CircularLinkedListType>
		CircularLinkedList<CircularLinkedListType> CircularLinkedList<CircularLinkedListType>::mergesort( const CircularLinkedList& L )
	{
		unsigned int counter;
		node* location;
		CircularLinkedList<CircularLinkedListType> L1;
		CircularLinkedList<CircularLinkedListType> L2;

		location = L.root;

		// Split the list into two equal size sublists, L1 and L2

		for ( counter = 0; counter < L.list_size / 2; counter++ )
		{
			// L1.add (*(location->item));
			L1.add ( location->item );
			location = location->next;
		}

		for ( ;counter < L.list_size; counter++ )
		{
			// L2.add(*(location->item));
			L2.add ( location->item );
			location = location->next;
		}

		// Recursively sort the sublists
		if ( L1.list_size > 1 )
			L1 = mergesort( L1 );

		if ( L2.list_size > 1 )
			L2 = mergesort( L2 );

		// Merge the two sublists
		return merge( L1, L2 );
	}

	template <class CircularLinkedListType>
		CircularLinkedList<CircularLinkedListType> CircularLinkedList<CircularLinkedListType>::merge( CircularLinkedList L1, CircularLinkedList L2 )
	{
		CircularLinkedList<CircularLinkedListType> X;
		CircularLinkedListType element;
		L1.position = L1.root;
		L2.position = L2.root;

		// While neither list is empty

		while ( ( L1.list_size != 0 ) && ( L2.list_size != 0 ) )
		{
			// Compare the first items of L1 and L2
			// Remove the smaller of the two items from the list

			if ( ( ( L1.root ) ->item ) < ( ( L2.root ) ->item ) )
				// if ((*((L1.root)->item)) < (*((L2.root)->item)))
			{
				// element = *((L1.root)->item);
				element = ( L1.root ) ->item;
				L1.del();
			}
			else
			{
				// element = *((L2.root)->item);
				element = ( L2.root ) ->item;
				L2.del();
			}

			// Add this item to the end of X
			X.add( element );

			X++;
		}

		// Add the remaining list to X
		if ( L1.list_size != 0 )
			X.concatenate( L1 );
		else
			X.concatenate( L2 );

		return X;
	}

	template <class LinkedListType>
		LinkedList<LinkedListType> LinkedList<LinkedListType>::mergesort( const LinkedList& L )
	{
		unsigned int counter;
		typename LinkedList::node* location;
		LinkedList<LinkedListType> L1;
		LinkedList<LinkedListType> L2;

		location = L.root;

		// Split the list into two equal size sublists, L1 and L2

		for ( counter = 0; counter < L.LinkedList_size / 2; counter++ )
		{
			// L1.add (*(location->item));
			L1.add ( location->item );
			location = location->next;
		}

		for ( ;counter < L.LinkedList_size; counter++ )
		{
			// L2.add(*(location->item));
			L2.add ( location->item );
			location = location->next;
		}

		// Recursively sort the sublists
		if ( L1.list_size > 1 )
			L1 = mergesort( L1 );

		if ( L2.list_size > 1 )
			L2 = mergesort( L2 );

		// Merge the two sublists
		return merge( L1, L2 );
	}

	template <class LinkedListType>
		LinkedList<LinkedListType> LinkedList<LinkedListType>::merge( LinkedList L1, LinkedList L2 )
	{
		LinkedList<LinkedListType> X;
		LinkedListType element;
		L1.position = L1.root;
		L2.position = L2.root;

		// While neither list is empty

		while ( ( L1.LinkedList_size != 0 ) && ( L2.LinkedList_size != 0 ) )
		{
			// Compare the first items of L1 and L2
			// Remove the smaller of the two items from the list

			if ( ( ( L1.root ) ->item ) < ( ( L2.root ) ->item ) )
				// if ((*((L1.root)->item)) < (*((L2.root)->item)))
			{
				element = ( L1.root ) ->item;
				// element = *((L1.root)->item);
				L1.del();
			}
			else
			{
				element = ( L2.root ) ->item;
				// element = *((L2.root)->item);
				L2.del();
			}

			// Add this item to the end of X
			X.add( element );
		}

		// Add the remaining list to X
		if ( L1.LinkedList_size != 0 )
			X.concatenate( L1 );
		else
			X.concatenate( L2 );

		return X;
	}


	// Prefix
	template <class LinkedListType>
		LinkedList<LinkedListType>& LinkedList<LinkedListType>::operator++()
	{
		if ( ( this->list_size != 0 ) && ( this->position->next != this->root ) )
			this->position = this->position->next;

		return *this;
	}

	/*
	// Postfix
	template <class LinkedListType>
	LinkedList<LinkedListType>& LinkedList<LinkedListType>::operator++(int)
	{
	LinkedList<LinkedListType> before;
	before=*this;
	operator++();
	return before;
	}
	*/ 
	// Postfix
	template <class LinkedListType>
		LinkedList<LinkedListType>& LinkedList<LinkedListType>::operator++( int )
	{
		return this->operator++();
	}

	// Prefix
	template <class LinkedListType>
		LinkedList<LinkedListType>& LinkedList<LinkedListType>::operator--()
	{
		if ( ( this->list_size != 0 ) && ( this->position != this->root ) )
			this->position = this->position->previous;

		return *this;
	}

	/*
	// Postfix
	template <class LinkedListType>
	LinkedList<LinkedListType>& LinkedList<LinkedListType>::operator--(int)
	{
	LinkedList<LinkedListType> before;
	before=*this;
	operator--();
	return before;
	}
	*/

	// Postfix
	template <class LinkedListType>
		LinkedList<LinkedListType>& LinkedList<LinkedListType>::operator--( int )
	{
		return this->operator--();
	}

} // End namespace

#endif
