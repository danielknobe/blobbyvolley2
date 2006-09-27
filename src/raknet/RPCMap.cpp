#include "RPCMap.h"
#include <string.h>

RPCMap::RPCMap()
{
}
RPCMap::~RPCMap()
{
	Clear();
}
void RPCMap::Clear(void)
{
	unsigned i;
	RPCNode *node;
	for (i=0; i < rpcSet.size(); i++)
	{
		node=rpcSet[i];
		if (node)
		{
			delete [] node->uniqueIdentifier;
			delete node;
		}		
	}
	rpcSet.clear();
}
RPCNode *RPCMap::GetNodeFromIndex(RPCIndex index)
{
	if ((unsigned)index < rpcSet.size())
		return rpcSet[(unsigned)index];
	return 0;
}
RPCNode *RPCMap::GetNodeFromFunctionName(char *uniqueIdentifier)
{
	unsigned index;
	index=(unsigned)GetIndexFromFunctionName(uniqueIdentifier);
	if (index!=UNDEFINED_RPC_INDEX)
		return rpcSet[index];
	return 0;
}
RPCIndex RPCMap::GetIndexFromFunctionName(char *uniqueIdentifier)
{
	unsigned index;
	for (index=0; index < rpcSet.size(); index++)
		if (rpcSet[index] && strcmp(rpcSet[index]->uniqueIdentifier, uniqueIdentifier)==0)
			return (RPCIndex) index;
	return UNDEFINED_RPC_INDEX;
}

// Called from the user thread for the local system
void RPCMap::AddIdentifierWithFunction(char *uniqueIdentifier, void *functionPointer, bool isPointerToMember)
{
#ifdef _DEBUG
	assert(rpcSet.size()+1 < MAX_RPC_MAP_SIZE); // If this hits change the typedef of RPCIndex to use an unsigned short
	assert(uniqueIdentifier && uniqueIdentifier[0]);
	assert(functionPointer);
#endif

	unsigned index, existingNodeIndex;
	RPCNode *node;

	existingNodeIndex=GetIndexFromFunctionName(uniqueIdentifier);
	if (existingNodeIndex!=UNDEFINED_RPC_INDEX) // Insert at any free spot.
	{
		// Trying to insert an identifier at any free slot and that identifier already exists
		// The user should not insert nodes that already exist in the list
#ifdef _DEBUG
//		assert(0);
#endif
		return;
	}

	node = new RPCNode;
	node->uniqueIdentifier = new char [strlen(uniqueIdentifier)+1];
	strcpy(node->uniqueIdentifier, uniqueIdentifier);
	node->functionPointer=functionPointer;
	node->isPointerToMember=isPointerToMember;

	// Insert into an empty spot if possible
	for (index=0; index < rpcSet.size(); index++)
	{
		if (rpcSet[index]==0)
		{
			rpcSet.replace(node, 0, index);
			return;
		}
	}

	rpcSet.insert(node); // No empty spots available so just add to the end of the list

}
void RPCMap::AddIdentifierAtIndex(char *uniqueIdentifier, RPCIndex insertionIndex)
{
#ifdef _DEBUG
	assert(uniqueIdentifier && uniqueIdentifier[0]);
#endif

	unsigned existingNodeIndex;
	RPCNode *node, *oldNode;

	existingNodeIndex=GetIndexFromFunctionName(uniqueIdentifier);

	if (existingNodeIndex==insertionIndex)
		return; // Already there

	if (existingNodeIndex!=UNDEFINED_RPC_INDEX)
	{
		// Delete the existing one
		oldNode=rpcSet[existingNodeIndex];
		rpcSet[existingNodeIndex]=0;
		delete [] oldNode->uniqueIdentifier;
		delete oldNode;
	}

	node = new RPCNode;
	node->uniqueIdentifier = new char [strlen(uniqueIdentifier)+1];
	strcpy(node->uniqueIdentifier, uniqueIdentifier);
	node->functionPointer=0;

	// Insert at a user specified spot
	if (insertionIndex < rpcSet.size())
	{
		// Overwrite what is there already
		oldNode=rpcSet[insertionIndex];
		if (oldNode)
		{
			delete [] oldNode->uniqueIdentifier;
			delete oldNode;
		}
		rpcSet[insertionIndex]=node;
	}
	else
	{
		// Insert after the end of the list and use 0 as a filler for the empty spots
		rpcSet.replace(node, 0, insertionIndex);
	}
}

void RPCMap::RemoveNode(char *uniqueIdentifier)
{
	unsigned index;
	index=GetIndexFromFunctionName(uniqueIdentifier);
    #ifdef _DEBUG
	assert(index!=UNDEFINED_RPC_INDEX); // If this hits then the user was removing an RPC call that wasn't currently registered
	#endif
	RPCNode *node;
	node = rpcSet[index];
	delete [] node->uniqueIdentifier;
	delete node;
	rpcSet[index]=0;
}

