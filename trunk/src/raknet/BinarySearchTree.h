/* -*- mode: c++; c-file-style: raknet; tab-always-indent: nil; -*- */
/**
 * @file
 * @brief Binary Seach Tree 
 *
 * Binary Search Tree ADT - By Kevin Jenkins (http://www.rakkar.org)
 * 9/04 Giblet - updated code to work with new compilers adhering more closely to ISO standard
 */




#ifndef __BINARY_SEARCH_TREE_H
#define __BINARY_SEARCH_TREE_H

#include "QueueLinkedList.h"

namespace BasicDataStructures
{
	/**
	 * Initilize with the following structure
	 *
	 * BinarySearchTree<TYPE>
	 *
	 * OR
	 *
	 * AVLBalancedBinarySearchTree<TYPE>
	 *
	 * Use the AVL balanced tree if you want the tree to be balanced after every deletion and addition.  This avoids the potential
	 * worst case scenario where ordered input to a binary search tree gives linear search time results.  It's not needed
	 * if input will be evenly distributed, in which case the search time is O (log n).  The search time for the AVL
	 * balanced binary tree is O (log n) irregardless of input.
	 *
	 * Has the following member functions
	 * unsigned int height(<index>) - Returns the height of the tree at the optional specified starting index.  Default is the root
	 * add(element) - adds an element to the BinarySearchTree
	 * bool del(element) - deletes the node containing element if the element is in the tree as defined by a comparison with the == operator.  Returns true on success, false if the element is not found
	 * bool is_in(element) - returns true if element is in the tree as defined by a comparison with the == operator.  Otherwise returns false
	 * display_inorder(array) - Fills an array with an inorder search of the elements in the tree.  USER IS REPONSIBLE FOR ALLOCATING THE ARRAY!.
	 * display_preorder(array) - Fills an array with an preorder search of the elements in the tree.  USER IS REPONSIBLE FOR ALLOCATING THE ARRAY!.
	 * display_postorder(array) - Fills an array with an postorder search of the elements in the tree. USER IS REPONSIBLE FOR ALLOCATING THE ARRAY!.
	 * display_breadth_first_search(array) - Fills an array with a breadth first search of the elements in the tree.  USER IS REPONSIBLE FOR ALLOCATING THE ARRAY!.
	 * clear - Destroys the tree.  Same as calling the destructor
	 * unsigned int height() - Returns the height of the tree
	 * unsigned int size() - returns the size of the BinarySearchTree
	 * get_pointer_to_node(element) - returns a pointer to the comparision element in the tree, allowing for direct modification when necessary with complex data types.
	 * Be warned, it is possible to corrupt the tree if the element used for comparisons is modified.  Returns NULL if the item is not found
	 *
	 *
	 * EXAMPLE
	 * @code
	 * BinarySearchTree<int> A;
	 * A.add(10);
	 * A.add(15);
	 * A.add(5);
	 * int* array = new int [A.size()];
	 * A.display_inorder(array);
	 * array[0]; // returns 5
	 * array[1]; // returns 10
	 * array[2]; // returns 15
	 * @endcode 
	 * compress - reallocates memory to fit the number of elements.  Best used when the number of elements decreases
	 *
	 * clear - empties the BinarySearchTree and returns storage
	 * The assignment and copy constructors are defined
	 *
	 * @note The template type must have the copy constructor and
	 * assignment operator defined and must work with >, <, and == All
	 * elements in the tree MUST be distinct The assignment operator is
	 * defined between BinarySearchTree and AVLBalancedBinarySearchTree
	 * as long as they are of the same template type. However, passing a
	 * BinarySearchTree to an AVLBalancedBinarySearchTree will lose its
	 * structure unless it happened to be AVL balanced to begin with
	 * Requires queue_linked_list.cpp for the breadth first search used
	 * in the copy constructor, overloaded assignment operator, and
	 * display_breadth_first_search.
	 *
	 *
	 */
	
	template <class BinarySearchTreeType>
	
	class BinarySearchTree
	{
	
	public:
		/**
		 * Describe a Binary Search Tree Node 
		 */
		
		struct node
		{
			/**
			 * The element stored in this node 
			 */
			BinarySearchTreeType* item;
			/**
			 * The left child node 
			 */
			node* left;
			/**
			 * The right child node 
			 */
			node* right;
		};
		
		/**
		 * Default Constructor 
		 */
		BinarySearchTree();
		/**
		 *  Destructor 
		 */
		virtual ~BinarySearchTree();
		/**
		 * Copy constructor 
		 * @param original_type The tree to duplicate 
		 */
		BinarySearchTree( const BinarySearchTree& original_type );
		/**
		 * Assignment operator 
		 * @param original_copy The object to copy 
		 * @return a reference to the current object. 
		 */
		BinarySearchTree& operator= ( const BinarySearchTree& original_copy );
		/**
		 * Retrieve the number of element of the tree. 
		 */
		const unsigned int size( void );
		/**
		 * Clear the tree removind all values. 
		 */
		void clear( void );
		/**
		 * Get the height of the tree or one of its subtree. 
		 * @param starting_node compute the height of the tree starting at node. 
		 * if @em starting_node is 0 then the height is computed from the root node. 
		 * @return the height of the tree. 
		 */
		const unsigned int height( node* starting_node = 0 );
		/**
		 * Add @em input to the tree and return its node. 
		 * @param input the value to add to the tree. 
		 * @return node a pointer to the node of the tree corresponding to @em input. 
		 */
		
		node* add ( const BinarySearchTreeType& input )
		
		;
		/**
		 * Remove @em input from the tree and return a pointer to the node
		 * previously containing @em input. 
		 * @param input The value to remove of the tree
		 * @return A pointer to the node previously containing @em input. 
		 */
		node* del( const BinarySearchTreeType& input );
		
		/**
		 * Test if the tree contains @em input. 
		 * @param input The element to search for. 
		 * @return true if the element is in the tree false otherwise. 
		 */
		bool is_in( const BinarySearchTreeType& input );
		
		/**
		 * Store all element of the tree in @em return_array. The element
		 * of the resulting arrayare in order.
		 * @param return_array The resulting array of elements. 
		 */
		void display_inorder( BinarySearchTreeType* return_array );
		
		/**
		 * Store all element of the tree in @em return_array. The element 
		 * of the resulting array follow a prefix order walk throught the nodes of the tree. 
		 * @param return_array The resulting array of elements. 
		 */
		void display_preorder( BinarySearchTreeType* return_array );
		
		/**
		 * Store all element of the tree in @em return_array. The element 
		 * of the resulting array follow a postfix order walk throught the nodes of the tree.
		 * @param return_array The resulting array of elements. 
		 */
		void display_postorder( BinarySearchTreeType* return_array );
		
		/**
		 * Display the tree using a breadth first search.
		 * Put the children of the current node into the queue.
		 * Pop the queue, put its children into the queue, repeat until queue is empty.
		 * @param return_array The resulting array of elements.  
		 */
		void display_breadth_first_search( BinarySearchTreeType* return_array );
		
		/**
		 * Get a pointer to the node containing @em element. 
		 * @param element The element to find 
		 * @return a pointer to the element if found, 0 otherwise. 
		 */
		BinarySearchTreeType*& get_pointer_to_node( const BinarySearchTreeType& element );
		
	protected:
		/**
		 * Root of the tree. 
		 */
		node* root;
		
		enum Direction_Types
		{
			NOT_FOUND, LEFT, RIGHT, ROOT
		}
		
		direction;
		const unsigned int height_recursive( node* current );
		/**
		 * Store the number of node composing the tree. 
		 */
		unsigned int BinarySearchTree_size;
		
		/**
		 * Find recursively a node of the tree. 
		 * @param element The value to search for 
		 * @param[out] parent a pointer to the parent node 
		 * @return The node storing the @em element. 
		 */
		node*& find( const BinarySearchTreeType& element, node** parent );
		/**
		 * Find the parent node of an element 
		 * @param element The element to search for. 
		 * @return The node parent of the node containing element 
		 */
		node*& find_parent( const BinarySearchTreeType& element );
		/**
		 * Does the recursive operation of filling the array. Used by display_postorder. 
		 * @param current The node to start the postfix order walk throught of the tree. 
		 * @param return_array The resulting array storing elements. 
		 * @param index The index of the next element in the array.
		 */
		void display_postorder_recursive( node* current, BinarySearchTreeType* return_array, unsigned int& index );
		/**
		 * Reorganized the nodes of the tree following an insertion or a deletion of a node. 
		 * @param current the node to fix. 
		 */
		void fix_tree( node* current );
		
	};
	
	/**
	 * This class is similar to the BinarySearchTree one in its interface. However it provided Balanced Tree. The tree 
	 * storing values has a minimum height at all time. Searching a value in such a tree is more efficient than in standard 
	 * BinarySearchTree. . 
	 * 
	 * @note 
	 * for more information consult the base class documentation. 
	 *
	 */
	template <class BinarySearchTreeType>
	
	class AVLBalancedBinarySearchTree : public BinarySearchTree<BinarySearchTreeType>
	{
	
	public:
		/**
		 * Default constructor 
		 */
		AVLBalancedBinarySearchTree()
		{}
		
		/**
		 * Destructor 
		 */
		virtual ~AVLBalancedBinarySearchTree();
		/**
		 * Add an element to the tree and balanced the tree. 
		 * @param input the new element
		 */
		
		void add ( const BinarySearchTreeType& input )
		
		;
		/**
		 * Remove an element of the tree and balanced the tree. 
		 * @param input the element to remove 
		 */
		void del( const BinarySearchTreeType& input );
		
		/**
		 * Assignement operator
		 * @param original_copy The tree to copy 
		 * @return a reference to the current object. 
		 */
		BinarySearchTree<BinarySearchTreeType>& operator= ( BinarySearchTree<BinarySearchTreeType>& original_copy )
		{
			return BinarySearchTree<BinarySearchTreeType>::operator= ( original_copy );
		}
		
	private:
		void balance_tree( typename BinarySearchTree<BinarySearchTreeType>::node* current, bool rotateOnce );
		void rotate_right( typename BinarySearchTree<BinarySearchTreeType>::node *C );
		void rotate_left( typename BinarySearchTree<BinarySearchTreeType>::node* C );
		void double_rotate_right( typename BinarySearchTree<BinarySearchTreeType>::node *A );
		void double_rotate_left( typename BinarySearchTree<BinarySearchTreeType>::node* A );
		bool right_higher( typename BinarySearchTree<BinarySearchTreeType>::node* A );
		bool left_higher( typename BinarySearchTree<BinarySearchTreeType>::node* A );
	};
	
	template <class BinarySearchTreeType>
	void AVLBalancedBinarySearchTree<BinarySearchTreeType>::balance_tree( typename BinarySearchTree<BinarySearchTreeType>::node* current, bool rotateOnce )
	{
		unsigned int left_height, right_height;
		
		while ( current )
		{
			if ( current->left == 0 )
				left_height = 0;
			else
				left_height = height( current->left );
				
			if ( current->right == 0 )
				right_height = 0;
			else
				right_height = height( current->right );
				
			if ( right_height - left_height == 2 )
			{
				if ( right_higher( current->right ) )
					rotate_left( current->right );
				else
					double_rotate_left( current );
					
				if ( rotateOnce )
					break;
			}
			
			else
				if ( right_height - left_height == -2 )
				{
					if ( left_higher( current->left ) )
						rotate_right( current->left );
					else
						double_rotate_right( current );
						
					if ( rotateOnce )
						break;
				}
				
			if ( current == this->root )
				break;
				
			current = find_parent( *( current->item ) );
			
		}
	}
	
	template <class BinarySearchTreeType>
	void AVLBalancedBinarySearchTree<BinarySearchTreeType>::add ( const BinarySearchTreeType& input )
	{
	
		typename BinarySearchTree<BinarySearchTreeType>::node * current = BinarySearchTree<BinarySearchTreeType>::add ( input )
		
				;
		balance_tree( current, true );
	}
	
	template <class BinarySearchTreeType>
	void AVLBalancedBinarySearchTree<BinarySearchTreeType>::del( const BinarySearchTreeType& input )
	{
		typename BinarySearchTree<BinarySearchTreeType>::node * current = BinarySearchTree<BinarySearchTreeType>::del( input );
		balance_tree( current, false );
		
	}
	
	template <class BinarySearchTreeType>
	bool AVLBalancedBinarySearchTree<BinarySearchTreeType>::right_higher( typename BinarySearchTree<BinarySearchTreeType>::node *A )
	{
		if ( A == 0 )
			return false;
			
		return height( A->right ) > height( A->left );
	}
	
	template <class BinarySearchTreeType>
	bool AVLBalancedBinarySearchTree<BinarySearchTreeType>::left_higher( typename BinarySearchTree<BinarySearchTreeType>::node *A )
	{
		if ( A == 0 )
			return false;
			
		return height( A->left ) > height( A->right );
	}
	
	template <class BinarySearchTreeType>
	void AVLBalancedBinarySearchTree<BinarySearchTreeType>::rotate_right( typename BinarySearchTree<BinarySearchTreeType>::node *C )
	{
		typename BinarySearchTree<BinarySearchTreeType>::node * A, *B, *D;
		/*
		  RIGHT ROTATION
		
		  A = parent(b)
		  b= parent(c)
		  c  = node to rotate around
		
		  A
		  | // Either direction
		  B
		  /   \
		  C
		  /   \
		  D
		
		  TO
		
		  A
		  | // Either Direction
		  C
		  /   \
		  B
		  /   \
		  D
		
		
		  <Leave all other branches branches AS-IS whether they point to another node or simply 0>
		
		*/
		
		B = find_parent( *( C->item ) );
		A = find_parent( *( B->item ) );
		D = C->right;
		
		if ( A )
		{
			// Direction was set by the last find_parent call
			
			if ( this->direction == this->LEFT )
				A->left = C;
			else
				A->right = C;
		}
		
		else
			this->root = C;  // If B has no parent parent then B must have been the root node
			
		B->left = D;
		
		C->right = B;
	}
	
	template <class BinarySearchTreeType>
	void AVLBalancedBinarySearchTree<BinarySearchTreeType>::double_rotate_right( typename BinarySearchTree<BinarySearchTreeType>::node *A )
	{
		// The left side of the left child must be higher for the tree to balance with a right rotation.  If it isn't, rotate it left before the normal rotation so it is.
		rotate_left( A->left->right );
		rotate_right( A->left );
	}
	
	template <class BinarySearchTreeType>
	void AVLBalancedBinarySearchTree<BinarySearchTreeType>::rotate_left( typename BinarySearchTree<BinarySearchTreeType>::node *C )
	{
		typename BinarySearchTree<BinarySearchTreeType>::node * A, *B, *D;
		/*
		  RIGHT ROTATION
		
		  A = parent(b)
		  b= parent(c)
		  c  = node to rotate around
		
		  A
		  | // Either direction
		  B
		  /   \
		  C
		  /  \
		  D
		
		  TO
		
		  A
		  | // Either Direction
		  C
		  /   \
		  B
		  /   \
		  D
		
		
		  <Leave all other branches branches AS-IS whether they point to another node or simply 0>
		
		*/
		
		B = find_parent( *( C->item ) );
		A = find_parent( *( B->item ) );
		D = C->left;
		
		if ( A )
		{
			// Direction was set by the last find_parent call
			
			if ( this->direction == this->LEFT )
				A->left = C;
			else
				A->right = C;
		}
		
		else
			this->root = C;  // If B has no parent parent then B must have been the root node
			
		B->right = D;
		
		C->left = B;
	}
	
	template <class BinarySearchTreeType>
	void AVLBalancedBinarySearchTree<BinarySearchTreeType>::double_rotate_left( typename BinarySearchTree<BinarySearchTreeType>::node *A )
	{
		// The left side of the right child must be higher for the tree to balance with a left rotation.  If it isn't, rotate it right before the normal rotation so it is.
		rotate_right( A->right->left );
		rotate_left( A->right );
	}
	
	template <class BinarySearchTreeType>
	AVLBalancedBinarySearchTree<BinarySearchTreeType>::~AVLBalancedBinarySearchTree()
	{
		this->clear();
	}
	
	template <class BinarySearchTreeType>
	const unsigned int BinarySearchTree<BinarySearchTreeType>::size( void )
	{
		return BinarySearchTree_size;
	}
	
	template <class BinarySearchTreeType>
	const unsigned int BinarySearchTree<BinarySearchTreeType>::height( typename BinarySearchTree::node* starting_node )
	{
		if ( BinarySearchTree_size == 0 || starting_node == 0 )
			return 0;
		else
			return height_recursive( starting_node );
	}
	
	// Recursively return the height of a binary tree
	template <class BinarySearchTreeType>
	const unsigned int BinarySearchTree<BinarySearchTreeType>::height_recursive( typename BinarySearchTree::node* current )
	{
		unsigned int left_height = 0, right_height = 0;
		
		if ( ( current->left == 0 ) && ( current->right == 0 ) )
			return 1; // Leaf
			
		if ( current->left != 0 )
			left_height = 1 + height_recursive( current->left );
			
		if ( current->right != 0 )
			right_height = 1 + height_recursive( current->right );
			
		if ( left_height > right_height )
			return left_height;
		else
			return right_height;
	}
	
	template <class BinarySearchTreeType>
	BinarySearchTree<BinarySearchTreeType>::BinarySearchTree()
	{
		BinarySearchTree_size = 0;
		root = 0;
	}
	
	template <class BinarySearchTreeType>
	BinarySearchTree<BinarySearchTreeType>::~BinarySearchTree()
	{
		this->clear();
	}
	
	template <class BinarySearchTreeType>
	BinarySearchTreeType*& BinarySearchTree<BinarySearchTreeType>::get_pointer_to_node( const BinarySearchTreeType& element )
	{
		static typename BinarySearchTree::node * tempnode;
		static BinarySearchTreeType* dummyptr = 0;
		tempnode = find ( element, &tempnode );
		
		if ( this->direction == this->NOT_FOUND )
			return dummyptr;
			
		return tempnode->item;
	}
	
	template <class BinarySearchTreeType>
	typename BinarySearchTree<BinarySearchTreeType>::node*& BinarySearchTree<BinarySearchTreeType>::find( const BinarySearchTreeType& element, typename BinarySearchTree<BinarySearchTreeType>::node** parent )
	{
		static typename BinarySearchTree::node * current;
		
		current = this->root;
		*parent = 0;
		this->direction = this->ROOT;
		
		if ( BinarySearchTree_size == 0 )
		{
			this->direction = this->NOT_FOUND;
			return current = 0;
		}
		
		// Check if the item is at the root
		if ( element == *( current->item ) )
		{
			this->direction = this->ROOT;
			return current;
		}

#pragma warning( disable : 4127 ) // warning C4127: conditional expression is constant
		while ( true )
		{
			// Move pointer
			
			if ( element < *( current->item ) )
			{
				*parent = current;
				this->direction = this->LEFT;
				current = current->left;
			}
			
			else
				if ( element > *( current->item ) )
				{
					*parent = current;
					this->direction = this->RIGHT;
					current = current->right;
				}
				
			if ( current == 0 )
				break;
				
			// Check if new position holds the item
			if ( element == *( current->item ) )
			{
				return current;
			}
		}
		
		
		this->direction = this->NOT_FOUND;
		return current = 0;
	}
	
	template <class BinarySearchTreeType>
	typename BinarySearchTree<BinarySearchTreeType>::node*& BinarySearchTree<BinarySearchTreeType>::find_parent( const BinarySearchTreeType& element )
	{
		static typename BinarySearchTree::node * parent;
		find ( element, &parent );
		return parent;
	}
	
	// Performs a series of value swaps starting with current to fix the tree if needed
	template <class BinarySearchTreeType>
	void BinarySearchTree<BinarySearchTreeType>::fix_tree( typename BinarySearchTree::node* current )
	{
		BinarySearchTreeType temp;
		
		while ( 1 )
		{
			if ( ( ( current->left ) != 0 ) && ( *( current->item ) < *( current->left->item ) ) )
			{
				// Swap the current value with the one to the left
				temp = *( current->left->item );
				*( current->left->item ) = *( current->item );
				*( current->item ) = temp;
				current = current->left;
			}
			
			else
				if ( ( ( current->right ) != 0 ) && ( *( current->item ) > *( current->right->item ) ) )
				{
					// Swap the current value with the one to the right
					temp = *( current->right->item );
					*( current->right->item ) = *( current->item );
					*( current->item ) = temp;
					current = current->right;
				}
				
				else
					break;  // current points to the right place so quit
		}
	}
	
	template <class BinarySearchTreeType>
	typename BinarySearchTree<BinarySearchTreeType>::node* BinarySearchTree<BinarySearchTreeType>::del( const BinarySearchTreeType& input )
	{
		typename BinarySearchTree::node * node_to_delete, *current, *parent;
		
		if ( BinarySearchTree_size == 0 )
			return 0;
			
		if ( BinarySearchTree_size == 1 )
		{
			clear();
			return 0;
		}
		
		node_to_delete = find( input, &parent );
		
		if ( direction == NOT_FOUND )
			return 0;  // Couldn't find the element
			
		current = node_to_delete;
		
		// Replace the deleted node with the appropriate value
		if ( ( current->right ) == 0 && ( current->left ) == 0 )    // Leaf node, just remove it
		{
		
			if ( parent )
			{
				if ( direction == LEFT )
					parent->left = 0;
				else
					parent->right = 0;
			}
			
			delete node_to_delete->item;
			delete node_to_delete;
			BinarySearchTree_size--;
			return parent;
		}
		else
			if ( ( current->right ) != 0 && ( current->left ) == 0 )   // Node has only one child, delete it and cause the parent to point to that child
			{
			
				if ( parent )
				{
					if ( direction == RIGHT )
						parent->right = current->right;
					else
						parent->left = current->right;
				}
				
				else
					root = current->right; // Without a parent this must be the root node
					
				delete node_to_delete->item;
				
				delete node_to_delete;
				
				BinarySearchTree_size--;
				
				return parent;
			}
			else
				if ( ( current->right ) == 0 && ( current->left ) != 0 )   // Node has only one child, delete it and cause the parent to point to that child
				{
				
					if ( parent )
					{
						if ( direction == RIGHT )
							parent->right = current->left;
						else
							parent->left = current->left;
					}
					
					else
						root = current->left; // Without a parent this must be the root node
						
					delete node_to_delete->item;
					
					delete node_to_delete;
					
					BinarySearchTree_size--;
					
					return parent;
				}
				else // Go right, then as left as far as you can
				{
					parent = current;
					direction = RIGHT;
					current = current->right; // Must have a right branch because the if statements above indicated that it has 2 branches
					
					while ( current->left )
					{
						direction = LEFT;
						parent = current;
						current = current->left;
					}
					
					// Replace the value held by the node to delete with the value pointed to by current;
					*( node_to_delete->item ) = *( current->item );
					
					// Delete current.
					// If it is a leaf node just delete it
					if ( current->right == 0 )
					{
						if ( direction == RIGHT )
							parent->right = 0;
						else
							parent->left = 0;
							
						delete current->item;
						
						delete current;
						
						BinarySearchTree_size--;
						
						return parent;
					}
					
					else
					{
						// Skip this node and make its parent point to its right branch
						
						if ( direction == RIGHT )
							parent->right = current->right;
						else
							parent->left = current->right;
							
						delete current->item;
						
						delete current;
						
						BinarySearchTree_size--;
						
						return parent;
					}
				}
	}
	
	template <class BinarySearchTreeType>
	typename BinarySearchTree<BinarySearchTreeType>::node* BinarySearchTree<BinarySearchTreeType>::add ( const BinarySearchTreeType& input )
	{
		typename BinarySearchTree::node * current, *parent;
		
		// Add the new element to the tree according to the following alogrithm:
		// 1.  If the current node is empty add the new leaf
		// 2.  If the element is less than the current node then go down the left branch
		// 3.  If the element is greater than the current node then go down the right branch
		
		if ( BinarySearchTree_size == 0 )
		{
			BinarySearchTree_size = 1;
			root = new typename BinarySearchTree::node;
			root->item = new BinarySearchTreeType;
			*( root->item ) = input;
			root->left = 0;
			root->right = 0;
			
			return root;
		}
		
		else
		{
			// start at the root
			current = parent = root;

#pragma warning( disable : 4127 ) // warning C4127: conditional expression is constant
			while ( true )    // This loop traverses the tree to find a spot for insertion
			{
			
				if ( input < *( current->item ) )
				{
					if ( current->left == 0 )
					{
						current->left = new typename BinarySearchTree::node;
						current->left->item = new BinarySearchTreeType;
						current = current->left;
						current->left = 0;
						current->right = 0;
						*( current->item ) = input;
						
						BinarySearchTree_size++;
						return current;
					}
					
					else
					{
						parent = current;
						current = current->left;
					}
				}
				
				else
					if ( input > *( current->item ) )
					{
						if ( current->right == 0 )
						{
							current->right = new typename BinarySearchTree::node;
							current->right->item = new BinarySearchTreeType;
							current = current->right;
							current->left = 0;
							current->right = 0;
							*( current->item ) = input;
							
							BinarySearchTree_size++;
							return current;
						}
						
						else
						{
							parent = current;
							current = current->right;
						}
					}
					
					else
						return 0; // ((input == current->item) == true) which is not allowed since the tree only takes discrete values.  Do nothing
			}
		}
	}
	
	template <class BinarySearchTreeType>
	bool BinarySearchTree<BinarySearchTreeType>::is_in( const BinarySearchTreeType& input )
	{
		typename BinarySearchTree::node * parent;
		find( input, &parent );
		
		if ( direction != NOT_FOUND )
			return true;
		else
			return false;
	}
	
	
	template <class BinarySearchTreeType>
	void BinarySearchTree<BinarySearchTreeType>::display_inorder( BinarySearchTreeType* return_array )
	{
		typename BinarySearchTree::node * current, *parent;
		bool just_printed = false;
		
		unsigned int index = 0;
		
		current = root;
		
		if ( BinarySearchTree_size == 0 )
			return ; // Do nothing for an empty tree
			
		else
			if ( BinarySearchTree_size == 1 )
			{
				return_array[ 0 ] = *( root->item );
				return ;
			}
			
			
		direction = ROOT;  // Reset the direction
		
		while ( index != BinarySearchTree_size )
		{
			// direction is set by the find function and holds the direction of the parent to the last node visited.  It is used to prevent revisiting nodes
			
			if ( ( current->left != 0 ) && ( direction != LEFT ) && ( direction != RIGHT ) )
			{
				//  Go left if the following 2 conditions are true
				//  I can go left
				//  I did not just move up from a right child
				//  I did not just move up from a left child
				
				current = current->left;
				direction = ROOT;  // Reset the direction
			}
			
			else
				if ( ( direction != RIGHT ) && ( just_printed == false ) )
				{
					// Otherwise, print the current node if the following 3 conditions are true:
					// I did not just move up from a right child
					// I did not print this ndoe last cycle
					
					return_array[ index++ ] = *( current->item );
					just_printed = true;
				}
				
				else
					if ( ( current->right != 0 ) && ( direction != RIGHT ) )
					{
						// Otherwise, go right if the following 2 conditions are true
						// I did not just move up from a right child
						// I can go right
						
						current = current->right;
						direction = ROOT;  // Reset the direction
						just_printed = false;
					}
					
					else
					{
						//  Otherwise I've done everything I can.  Move up the tree one node
						parent = find_parent( *( current->item ) );
						current = parent;
						just_printed = false;
					}
		}
	}
	
	template <class BinarySearchTreeType>
	void BinarySearchTree<BinarySearchTreeType>::display_preorder( BinarySearchTreeType* return_array )
	{
		typename BinarySearchTree::node * current, *parent;
		
		unsigned int index = 0;
		
		current = root;
		
		if ( BinarySearchTree_size == 0 )
			return ; // Do nothing for an empty tree
			
		else
			if ( BinarySearchTree_size == 1 )
			{
				return_array[ 0 ] = *( root->item );
				return ;
			}
			
			
		direction = ROOT;  // Reset the direction
		return_array[ index++ ] = *( current->item );
		
		while ( index != BinarySearchTree_size )
		{
			// direction is set by the find function and holds the direction of the parent to the last node visited.  It is used to prevent revisiting nodes
			
			if ( ( current->left != 0 ) && ( direction != LEFT ) && ( direction != RIGHT ) )
			{
			
				current = current->left;
				direction = ROOT;
				
				// Everytime you move a node print it
				return_array[ index++ ] = *( current->item );
			}
			
			else
				if ( ( current->right != 0 ) && ( direction != RIGHT ) )
				{
					current = current->right;
					direction = ROOT;
					
					// Everytime you move a node print it
					return_array[ index++ ] = *( current->item );
				}
				
				else
				{
					//  Otherwise I've done everything I can.  Move up the tree one node
					parent = find_parent( *( current->item ) );
					current = parent;
				}
		}
	}
	
	template <class BinarySearchTreeType>
	inline void BinarySearchTree<BinarySearchTreeType>::display_postorder( BinarySearchTreeType* return_array )
	{
		unsigned int index = 0;
		
		if ( BinarySearchTree_size == 0 )
			return ; // Do nothing for an empty tree
			
		else
			if ( BinarySearchTree_size == 1 )
			{
				return_array[ 0 ] = *( root->item );
				return ;
			}
			
		display_postorder_recursive( root, return_array, index );
	}
	
	
	// Recursively do a postorder traversal
	template <class BinarySearchTreeType>
	void BinarySearchTree<BinarySearchTreeType>::display_postorder_recursive( typename BinarySearchTree::node* current, BinarySearchTreeType* return_array, unsigned int& index )
	{
		if ( current->left != 0 )
			display_postorder_recursive( current->left, return_array, index );
			
		if ( current->right != 0 )
			display_postorder_recursive( current->right, return_array, index );
			
		return_array[ index++ ] = *( current->item );
		
	}
	
	
	template <class BinarySearchTreeType>
	void BinarySearchTree<BinarySearchTreeType>::display_breadth_first_search( BinarySearchTreeType* return_array )
	{
		typename BinarySearchTree::node * current;
		unsigned int index = 0;
		
		// Display the tree using a breadth first search
		// Put the children of the current node into the queue
		// Pop the queue, put its children into the queue, repeat until queue is empty
		
		if ( BinarySearchTree_size == 0 )
			return ; // Do nothing for an empty tree
			
		else
			if ( BinarySearchTree_size == 1 )
			{
				return_array[ 0 ] = *( root->item );
				return ;
			}
			
			else
			{
				BasicDataStructures::QueueLinkedList<node *> tree_queue;
				
				// Add the root of the tree I am copying from
				tree_queue.push( root );
				
				do
				{
					current = tree_queue.pop();
					return_array[ index++ ] = *( current->item );
					
					// Add the child or children of the tree I am copying from to the queue
					
					if ( current->left != 0 )
						tree_queue.push( current->left );
						
					if ( current->right != 0 )
						tree_queue.push( current->right );
						
				}
				
				while ( tree_queue.size() > 0 );
			}
	}
	
	
	template <class BinarySearchTreeType>
	BinarySearchTree<BinarySearchTreeType>::BinarySearchTree( const BinarySearchTree& original_copy )
	{
		typename BinarySearchTree::node * current;
		// Copy the tree using a breadth first search
		// Put the children of the current node into the queue
		// Pop the queue, put its children into the queue, repeat until queue is empty
		
		// This is a copy of the constructor.  A bug in Visual C++ made it so if I just put the constructor call here the variable assignments were ignored.
		BinarySearchTree_size = 0;
		root = 0;
		
		if ( original_copy.BinarySearchTree_size == 0 )
		{
			BinarySearchTree_size = 0;
		}
		
		else
		{
			BasicDataStructures::QueueLinkedList<node *> tree_queue;
			
			// Add the root of the tree I am copying from
			tree_queue.push( original_copy.root );
			
			do
			{
				current = tree_queue.pop();
				
				add ( *( current->item ) )
				
				;
				
				// Add the child or children of the tree I am copying from to the queue
				if ( current->left != 0 )
					tree_queue.push( current->left );
					
				if ( current->right != 0 )
					tree_queue.push( current->right );
					
			}
			
			while ( tree_queue.size() > 0 );
		}
	}
	
	template <class BinarySearchTreeType>
	BinarySearchTree<BinarySearchTreeType>& BinarySearchTree<BinarySearchTreeType>::operator= ( const BinarySearchTree& original_copy )
	{
		typename BinarySearchTree::node * current;
		
		if ( ( &original_copy ) == this )
			return this;
			
		clear();  // Remove the current tree
		
		// This is a copy of the constructor.  A bug in Visual C++ made it so if I just put the constructor call here the variable assignments were ignored.
		BinarySearchTree_size = 0;
		
		root = 0;
		
		
		// Copy the tree using a breadth first search
		// Put the children of the current node into the queue
		// Pop the queue, put its children into the queue, repeat until queue is empty
		if ( original_copy.BinarySearchTree_size == 0 )
		{
			BinarySearchTree_size = 0;
		}
		
		else
		{
			BasicDataStructures::QueueLinkedList<node *> tree_queue;
			
			// Add the root of the tree I am copying from
			tree_queue.push( original_copy.root );
			
			do
			{
				current = tree_queue.pop();
				
				add ( *( current->item ) )
				
				;
				
				// Add the child or children of the tree I am copying from to the queue
				if ( current->left != 0 )
					tree_queue.push( current->left );
					
				if ( current->right != 0 )
					tree_queue.push( current->right );
					
			}
			
			while ( tree_queue.size() > 0 );
		}
		
		return this;
	}
	
	template <class BinarySearchTreeType>
	inline void BinarySearchTree<BinarySearchTreeType>::clear ( void )
	{
		typename BinarySearchTree::node * current, *parent;
		
		current = root;
		
		while ( BinarySearchTree_size > 0 )
		{
			if ( BinarySearchTree_size == 1 )
			{
				delete root->item;
				delete root;
				root = 0;
				BinarySearchTree_size = 0;
			}
			
			else
			{
				if ( current->left != 0 )
				{
					current = current->left;
				}
				
				else
					if ( current->right != 0 )
					{
						current = current->right;
					}
					
					else // leaf
					{
						// Not root node so must have a parent
						parent = find_parent( *( current->item ) );
						
						if ( ( parent->left ) == current )
							parent->left = 0;
						else
							parent->right = 0;
							
						delete current->item;
						
						delete current;
						
						current = parent;
						
						BinarySearchTree_size--;
					}
			}
		}
	}
	
} // End namespace

#endif
