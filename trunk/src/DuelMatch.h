#include "PhysicWorld.h"
#include "Vector.h"

// This class represents a single game between two players
// It applys the rules itself and provides an interface for querying
// different parameters. For this purpose it is designed as something
// similar to a singleton, but it can be instantiated
// multiple times on a server or be completely unavailable

class InputSource;

class DuelMatch
{
public:
	// This constructor takes the input sources used to get player input
	// The parameter output tells DuelMatch if it should report its
	// results to the user through RenderManager and SoundManager.
	// A deacivation of the output is useful on dedicated servers
	
	// Although DuelMatch is only ment to implement the ruleset
	// and combine input and physics, it would be unpractical to
	// export the attributes necessary for output. 
	
	DuelMatch(InputSource* linput, InputSource* rinput,
					bool output);
				
	~DuelMatch();
	
	// This steps through one frame
	void step();

	// This reports the index of the winning player and -1 if the
	// game is still running
	PlayerSide winningPlayer();
	
	// This methods report the current game state and a useful for
	// the input manager, which needs information about the blob
	// positions and for lua export, which makes them accessable
	// for scripted input sources
	
	int getScore(int player);
	int servingPlayer();
	
	int getHitcount(int player);
	
	Vector2 getBallPosition();
	Vector2 getBallVelocity();
	Vector2 getBlobPosition(int player);
	
	// This functions returns true if the player launched
	// and is jumping at the moment
	bool getBlobJump(int player);
	
private:
	PhysicWorld mPhysicWorld;
	
	InputSource* mLeftInput;
	InputSource* mRightInput;

	int mLeftScore;
	int mRightScore;
	int mServingPlayer;

	int mLeftHitcount;
	int mRightHitcount;

	int mSquishLeft;
	int mSquishRight;
	
	bool mOutput;
	int mWinningPlayer;
};
