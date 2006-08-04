#include "State.h"
#include "LocalInputSource.h"
#include "RenderManager.h"
#include "SoundManager.h"
#include "GUIManager.h"
#include "DuelMatch.h"

State::State()
{

}

State* State::getCurrentState()
{
	if (!mCurrentState)
		mCurrentState = new MainMenuState;
	return mCurrentState;		
}

State* State::mCurrentState = 0;

LocalGameState::~LocalGameState()
{
	delete mLeftInput;
	delete mRightInput;
	delete mMatch;
}

LocalGameState::LocalGameState(GameMode mode)
	: State()
{
	UserConfig gameConfig;
	gameConfig.loadFile("config.xml");
	mLeftColor = Color(gameConfig.getInteger("r1"),
		gameConfig.getInteger("g1"),
		gameConfig.getInteger("b1"));
	mRightColor = Color(gameConfig.getInteger("r2"),
		gameConfig.getInteger("g2"),
		gameConfig.getInteger("b2"));
	mLeftOscillate = gameConfig.getBool("left_blobby_oscillate");
	mRightOscillate = gameConfig.getBool("right_blobby_oscillate");

	RenderManager::getSingleton().setBlobColor(0, mLeftColor);
	RenderManager::getSingleton().setBlobColor(1, mRightColor);
	RenderManager::getSingleton().redraw();

	GUIManager::getSingleton()->drawCursor(false);
	mLeftInput = new LocalInputSource(LEFT_PLAYER);
	mRightInput = new LocalInputSource(RIGHT_PLAYER);

	SoundManager::getSingleton().playSound("sounds/pfiff.wav", 0.2);
	mMatch = new DuelMatch(mLeftInput, mRightInput, true);
}

void LocalGameState::step()
{
	RenderManager* rmanager = &RenderManager::getSingleton();
	SoundManager* smanager = &SoundManager::getSingleton();
	
	mMatch->step();

	float time = float(SDL_GetTicks()) / 1000.0;
	if (mLeftOscillate)
		rmanager->setBlobColor(0, Color(
			int((sin(time*2) + 1.0) * 128),
			int((sin(time*4) + 1.0) * 128),
			int((sin(time*3) + 1.0) * 128)));
	if (mRightOscillate)
		rmanager->setBlobColor(1, Color(
			int((cos(time*2) + 1.0) * 128),
			int((cos(time*4) + 1.0) * 128),
			int((cos(time*3) + 1.0) * 128)));
		
	PlayerSide side = mMatch->winningPlayer();
	if (side != NO_PLAYER)
	{
		delete this;
		mCurrentState = new WinState(side);
	}
	if (InputManager::getSingleton()->exit())
	{
		delete this;
		mCurrentState = new MainMenuState;
	}
}


MainMenuState::MainMenuState()
{
	GUIManager* gmgr = GUIManager::getSingleton();
	gmgr->clear();
	gmgr->createOverlay(0.5, Vector2(0.0, 0.0), Vector2(800.0, 600.0));
	gmgr->createImage("gfx/titel.bmp", Vector2(250.0, 210.0));
	gmgr->drawCursor(true);
	mStartButton = gmgr->createTextButton("start", 5, Vector2(500.0, 460.0));
	mExitButton = gmgr->createTextButton("exit", 4, Vector2(500, 490.0));
}

MainMenuState::~MainMenuState()
{
	GUIManager::getSingleton()->clear();
}

void MainMenuState::step()
{
	GUIManager* gmgr = GUIManager::getSingleton();
	RenderManager* rmanager = &RenderManager::getSingleton();
	rmanager->setBall(Vector2(-2000.0, 0.0), 0.0);
	rmanager->setBlob(0, Vector2(-2000, 0.0), 0.0);
	rmanager->setBlob(1, Vector2(-2000, 0.0), 0.0);
	rmanager->setScore(0, 0, false, false);
	
	if (gmgr->getClick(mStartButton))
	{
		delete mCurrentState;
		mCurrentState = new LocalGameState(MODE_NORMAL_DUEL);
	}
	
	else if (gmgr->getClick(mExitButton)) 
	{
		// TODO: Create a global exit function
		
		rmanager->deinit();
		SoundManager::getSingleton().deinit();
		SDL_Quit();
		exit(0);
	}

}

WinState::WinState(int player)
{
	GUIManager* gmgr = GUIManager::getSingleton();
	
	gmgr->clear();
	gmgr->createOverlay(0.5, Vector2(200, 150), Vector2(650, 450));
	gmgr->createImage("gfx/pokal.bmp", Vector2(200, 250));
	char buf[64];
	snprintf(buf, 64, "Spieler %d", player + 1);
	gmgr->createText(buf, Vector2(250, 250));
	gmgr->createText("hat gewonnen!", Vector2(250, 300));	
}

void WinState::step()
{
	InputManager* inputmgr = InputManager::getSingleton();
	if (inputmgr->click() || inputmgr->select())
	{
		delete mCurrentState;
		mCurrentState = new MainMenuState();
	}
}
