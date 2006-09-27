/* -*- mode: c++; c-file-style: raknet; tab-always-indent: nil; -*- */
/**
* @file 
* @brief NetworkIDGenerator Implementation 
*
 * This file is part of RakNet Copyright 2003 Rakkarsoft LLC and Kevin Jenkins.
 *
 * Usage of Raknet is subject to the appropriate licence agreement.
 * "Shareware" Licensees with Rakkarsoft LLC are subject to the
 * shareware license found at
 * http://www.rakkarsoft.com/shareWareLicense.html which you agreed to
 * upon purchase of a "Shareware license" "Commercial" Licensees with
 * Rakkarsoft LLC are subject to the commercial license found at
 * http://www.rakkarsoft.com/sourceCodeLicense.html which you agreed
 * to upon purchase of a "Commercial license"
 * Custom license users are subject to the terms therein.
 * All other users are
 * subject to the GNU General Public License as published by the Free
 * Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 *
 * Refer to the appropriate license agreement for distribution,
 * modification, and warranty rights.
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
