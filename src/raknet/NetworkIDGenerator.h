/* -*- mode: c++; c-file-style: raknet; tab-always-indent: nil; -*- */
/**
 * @file
 * @brief Creates unique network IDs.  Must be derived from to implement integration into network system.
 * NetworkObject is an add-on system that uses RakNet and interfaces with another add-on system: DistributedNetworkObject
 * NetworkIDGenerator is a core system in RakNet and does not interface with any external systems
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
#if !defined(__NETWORK_ID_GENERATOR)
#define      __NETWORK_ID_GENERATOR

#include "BinarySearchTree.h"
#include "NetworkTypes.h"

class NetworkIDGenerator;
/**
 * @brief  Define a node of an Object Tree. 
 * Used internally to contain objects in the tree.  Ignore this
 */
struct ObjectIDNode
{
	/**
	 * id 
	 */
	ObjectID objectID;
	/**
	 * a pointer to the object associated to id 
	 */
	NetworkIDGenerator *object;
	/**
	 * Default Constructor
	 */
	ObjectIDNode();
	/**
	 * Create a node based on the objectID and a pointer to the NetworkIDGenerator 
	 * @param ObjectID id of the node
	 * @param Object the network object 
	 */
	ObjectIDNode( ObjectID _objectID, NetworkIDGenerator *_object );
	/**
	 * Test if two nodes are equals 
	 * @param left a node
	 * @param right a node 
	 * @return true if the nodes are the same 
	 */
	friend int operator==( const ObjectIDNode& left, const ObjectIDNode& right );
	/**
	 * Test if one node is greater than the other 
	 * @param left a node
	 * @param right a node
	 * @return True if @em left is greater than @em right 
	 */
	friend int operator > ( const ObjectIDNode& left, const ObjectIDNode& right );
	/**
	 * Test if one node is lesser than the other 
	 * @param left a node
	 * @param right a node
	 * @return True if @em left is lesser than @em right 
	 */
	friend int operator < ( const ObjectIDNode& left, const ObjectIDNode& right );
};

/**
 * A NetworkIDGenerator is used to identify uniquely an object on the
 * network You can easly create object with network unique id by
 * subclassing this class.
 */

class NetworkIDGenerator
{
public:
	/**
	 * Default Constructor
	 */
	NetworkIDGenerator();
	/**
	 * Default Destructor
	 */
	virtual ~NetworkIDGenerator();
	/**
	 * Get the id of the object 
	 * @return the id of the current object 
	 */
	virtual ObjectID GetNetworkID( void );
	/**
	 * Associate an id to an object 
	 * @param id the new id of the network object.
	 * @note Only the server code should
	 * call this.
	 *
	 */
	virtual void SetNetworkID( ObjectID id );

	/**
	* If you want this to be a member object of another class
	* Then call SetParent.  It will return the parent rather than itself.
	* Useful if you can't derive, such as with multiple inheritance
	*
	*/
	virtual void SetParent( void *_parent );

	/**
	* Return what was passed to SetParent
	*
	*/
	virtual void* GetParent( void ) const;

	/**
	 * Store all object in an AVL Tree using id as key 
	 */
	static BasicDataStructures::AVLBalancedBinarySearchTree<ObjectIDNode> IDTree;
	/**
	 * These function is only meant to be used when saving games as you
	 * should save the HIGHEST value staticItemID has achieved upon save
	 * and reload it upon load.  Save AFTER you've created all the items
	 * derived from this class you are going to create.  
	 * @return the HIGHEST Object Id currently used 
	 */
	static ObjectID GetStaticNetworkID( void );
	/**
	 * These function is only meant to be used when loading games. Load
	 * BEFORE you create any new objects that are not SetIDed based on
	 * the save data. 
	 * @param i the highest number of NetworkIDGenerator reached. 
	 */
	static void SetStaticNetworkID( ObjectID i );

	/**
	* OVERLOAD or REWRITE these to reflect the status of the server and client in your own game - or even peers if you transfer who assigns new IDs
	IsObjectIDAuthority should return true if this is a system that either is or will create IDs to other systems
	IsObjectIDAuthorityActive should return true if this is a system will create IDs to other systems and is currently doing so
	IsObjectIDRecipient should return true if this is a system that either is or will accept IDs from other systems
	IsObjectIDRecipientActive should return true if this is a system that is currently accepting IDs from other systems
	*/
	virtual bool IsObjectIDAuthority(void) const=0; // Usually means is this a server?
	virtual bool IsObjectIDAuthorityActive(void) const=0; // Usually means is this a server and that the server is running?
	virtual bool IsObjectIDRecipient(void) const=0; // Usually means is this a client?
	virtual bool IsObjectIDRecipientActive(void) const=0; // Usually means is this a client and that client is connected?

	// Return true if you require that SetParent is called before this object is used.  You should do this if you want this to be
	// a member object of another class rather than derive from this class.
	virtual bool RequiresSetParent(void) const;
	
protected:
	/**
	 * The  network ID of this object
	 */
	ObjectID objectID;
	/**
	 * Store whether or not its a server assigned ID 
	 */
	bool serverAssignedID;
    void *parent;
	void GenerateID(void);
	bool callGenerationCode; // This is crap but is necessary because virtual functions don't work in the constructor
	
private:
	/**
	 * Number of network object. 
	 */
	static ObjectID staticItemID;
};

/**
 * Retrieve a NetworkIDGenerator from its ID 
 * @param x the object id 
 * @return a pointer to a NetworkIDGenerator or 0 if there is no corresponding Id. 
 */
void* GET_OBJECT_FROM_ID( ObjectID x );

#endif // !defined(AFX_NetworkIDGenerator_H__29266376_3F9E_42CD_B208_B58957E1935B__INCLUDED_)
