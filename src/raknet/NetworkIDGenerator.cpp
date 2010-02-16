/* -*- mode: c++; c-file-style: raknet; tab-always-indent: nil; -*- */
/**
 * @file 
 * @brief NetworkIDGenerator Implementation 
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

#include "NetworkIDGenerator.h"

// alloca
#ifdef _WIN32
#include <malloc.h>
#else
#include <stdlib.h>
#endif

#include <assert.h>

// Note you will need to save and load this if your game supports saving and loading so you start at the same index you left off.
// If you don't do this you can overwrite indices
ObjectID NetworkIDGenerator::staticItemID = 0;
BasicDataStructures::AVLBalancedBinarySearchTree<ObjectIDNode> NetworkIDGenerator::IDTree;

int operator==( const ObjectIDNode& left, const ObjectIDNode& right )
{
	if ( left.objectID == right.objectID )
		return !0;

	return 0;
}

int operator > ( const ObjectIDNode& left, const ObjectIDNode& right )
{
	if ( left.objectID > right.objectID )
		return !0;

	return 0;
}

int operator < ( const ObjectIDNode& left, const ObjectIDNode& right )
{
	if ( left.objectID < right.objectID )
		return !0;

	return 0;
}

ObjectIDNode::ObjectIDNode()
{
	object = 0;
}

ObjectIDNode::ObjectIDNode( ObjectID _objectID, NetworkIDGenerator *_object )
{
	objectID = _objectID;
	object = _object;
}


//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

NetworkIDGenerator::NetworkIDGenerator()
{
	callGenerationCode=true;
}

//-------------------------------------------------------------------------------------


NetworkIDGenerator::~NetworkIDGenerator()
{
	if ( serverAssignedID && callGenerationCode==false)
	{
		ObjectIDNode * object = NetworkIDGenerator::IDTree.get_pointer_to_node( ObjectIDNode( ( objectID ), 0 ) );

		if ( object->object == this )
			IDTree.del( ObjectIDNode( object->objectID, 0 ) );

		// else
		// printf("Warning: Deleting object with ID %i that does not match the object with that ID in the tree.  Leaving the existing node in the tree.  Possible cause: Assigning the ID of this object to another object and then deleting this object. Correct action: Delete this object before assigning its ID to another object.", objectID);
	}
}

//////////////////////////////////////////////////////////////////////
// Public Methods
//////////////////////////////////////////////////////////////////////

ObjectID NetworkIDGenerator::GetNetworkID( void )
{
	if (callGenerationCode)
	{
		GenerateID();
		callGenerationCode=false;
	}

	// Dedicated client, hasn't had the ID assigned yet
	if ( serverAssignedID == false &&
		IsObjectIDRecipient() && IsObjectIDRecipientActive() &&
		( IsObjectIDAuthority()==false || IsObjectIDAuthorityActive() == false ) )
		return UNASSIGNED_OBJECT_ID;
	else
		return objectID;
};

//-------------------------------------------------------------------------------------

ObjectID NetworkIDGenerator::GetStaticNetworkID( void )
{
	return staticItemID;
}

//-------------------------------------------------------------------------------------

void NetworkIDGenerator::SetStaticNetworkID( ObjectID i )
{
	staticItemID = i;
}

//-------------------------------------------------------------------------------------

bool NetworkIDGenerator::RequiresSetParent(void) const
{
	return false;
}

//-------------------------------------------------------------------------------------

void NetworkIDGenerator::SetNetworkID( ObjectID id )
{
	if (callGenerationCode)
	{
		GenerateID();
		callGenerationCode=false;
	}

	if ( id == UNASSIGNED_OBJECT_ID )
	{
		// puts("Warning: NetworkIDGenerator passed UNASSIGNED_OBJECT_ID.  SetID ignored");
		return ;
	}

	if ( serverAssignedID == true && objectID == id )
	{
		// printf("NetworkIDGenerator passed %i which already exists in the tree.  SetID ignored", id);
		return ;
	}

	ObjectIDNode* collision = NetworkIDGenerator::IDTree.get_pointer_to_node( ObjectIDNode( ( id ), 0 ) );

	if ( collision )   // Tree should have only unique values
	{
		//printf("Warning: NetworkIDGenerator::SetID passed %i, which has an existing node in the tree.  Old node removed, which will cause the item pointed to to be inaccessible to the network", id);
		IDTree.del( ObjectIDNode( collision->objectID, collision->object ) );
		collision->object->serverAssignedID = false;
	}

	if ( serverAssignedID == false || objectID == UNASSIGNED_OBJECT_ID )   // Object has not had an ID assigned so does not already exist in the tree
	{
		objectID = id;
		IDTree.add( ObjectIDNode( objectID, this ) );
	}
	else // Object already exists in the tree and has an assigned ID
	{
		IDTree.del( ObjectIDNode( objectID, this ) ); // Delete the node with whatever ID the existing object is using
		objectID = id;
		IDTree.add( ObjectIDNode( objectID, this ) );
	}

	serverAssignedID = true;
}

//-------------------------------------------------------------------------------------
void NetworkIDGenerator::SetParent( void *_parent )
{
	parent=_parent;

#ifdef _DEBUG


	// Avoid duplicate parents in the tree
	unsigned size = IDTree.size();
	ObjectIDNode *nodeArray;
	nodeArray = (ObjectIDNode*) alloca(sizeof(ObjectIDNode) * size);
	IDTree.display_breadth_first_search( nodeArray );
	unsigned i;
	for (i=0; i < size; i++)
	{
		// If this assert hits then this _parent is already in the tree.  Classes instance should never contain more than one NetworkIDGenerator
		assert(nodeArray->object->GetParent()!=parent);
	}
#endif
}

//-------------------------------------------------------------------------------------
void* NetworkIDGenerator::GetParent( void ) const
{
#ifdef _DEBUG
	assert(RequiresSetParent());
#endif
	return parent;
}


//-------------------------------------------------------------------------------------
void NetworkIDGenerator::GenerateID(void)
{
	if ( ( IsObjectIDAuthority() && IsObjectIDAuthorityActive() )
		||
		( IsObjectIDRecipient() && IsObjectIDRecipientActive() == false ) )
	{
		ObjectIDNode* collision;

		serverAssignedID = true;
		do
		{
			objectID = staticItemID++;
			collision = NetworkIDGenerator::IDTree.get_pointer_to_node( ObjectIDNode( ( objectID ), 0 ) );
		}
		while ( collision );

		IDTree.add( ObjectIDNode( objectID, this ) );
	}

	else
		serverAssignedID = false;
}


//-------------------------------------------------------------------------------------

void* GET_OBJECT_FROM_ID( ObjectID x )
{
	if ( x == UNASSIGNED_OBJECT_ID )
		return 0;

	ObjectIDNode *n = NetworkIDGenerator::IDTree.get_pointer_to_node( ObjectIDNode( ( x ), 0 ) );

	if ( n )
	{
		if (n->object->RequiresSetParent())
		{
#ifdef _DEBUG
			// If this assert hit then this object requires a call to SetParent and it never got one.
			assert(n->object->GetParent());
#endif
			return n->object->GetParent();
		}
		else
			return n->object;
	}

	return 0;
}


//////////////////////////////////////////////////////////////////////
// EOF
//////////////////////////////////////////////////////////////////////
