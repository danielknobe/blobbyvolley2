#include <SDL/SDL.h>

#ifdef WIN32
#undef main
#endif 

#include <physfs.h>

#include "RenderManager.h"
#include "SoundManager.h"
#include "PhysicWorld.h"
#include "InputManager.h"
#include "LocalInputSource.h"
#include "NetworkManager.h"
#include "UserConfig.h"
#include "GUIManager.h"

enum GameMode
{
	MODE_MAINMENU,
	MODE_LOCALGAME
	
};



void correctFramerate()
{	int rate = 60;
	float rateTicks = 1000.0 / ((float)rate);
	static int frameCount = 0;
	static int lastTicks = SDL_GetTicks();
	
	int currentTicks;
	int targetTicks;
	++frameCount;
	currentTicks = SDL_GetTicks();
	targetTicks = (int)(((float)frameCount) * rateTicks) + lastTicks;
	if (currentTicks <= targetTicks)
		SDL_Delay(targetTicks - currentTicks);
	else
	{
		frameCount = 0;
		lastTicks = SDL_GetTicks();
	}
}

int mainmenu()
{
	GUIManager* gmgr = GUIManager::getSingleton();
	RenderManager* rmanager = &RenderManager::getSingleton();
	rmanager->setBall(Vector2(-2000.0, 0.0), 0.0);
	rmanager->setBlob(0, Vector2(-2000, 0.0), 0.0);
	rmanager->setBlob(1, Vector2(-2000, 0.0), 0.0);
	rmanager->setScore(0, 0, false, false);
	int running = 1;
	gmgr->clear();
	gmgr->createOverlay(0.5, Vector2(0.0, 0.0), Vector2(800.0, 600.0));
	gmgr->createImage("gfx/titel.bmp", Vector2(250.0, 210.0));
//	gmgr->createText("Freigegeben zum Probespielen!", Vector2(20, 370));
	int startButton = gmgr->createTextButton("start", 5, Vector2(500.0, 460.0));
	int exitButton = gmgr->createTextButton("exit", 4, Vector2(500, 490.0));
	
	
	while (running)
	{
		InputManager::getSingleton()->updateInput();
		gmgr->processInput();
		
		if (gmgr->getClick(startButton)) 
			return 1;
		if (gmgr->getClick(exitButton)) 
			return 2;
		if (!InputManager::getSingleton()->running())
			return 2;	
		
		rmanager->draw();
		gmgr->render();
		rmanager->refresh();
		correctFramerate();
	}
	
}

int gameStep(UserConfig& gameConfig, PhysicWorld& pworld);

int main(int argc, char* argv[])
{
	PHYSFS_init(argv[0]);
	PHYSFS_addToSearchPath("data", 0);
	PHYSFS_setWriteDir("data");
	
	SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO);

	// Default is OpenGL and false
	// choose renderer
	RenderManager *rmanager;
	UserConfig gameConfig;
	gameConfig.loadFile("config.xml");
	if(gameConfig.getString("device") == "SDL")
		rmanager = RenderManager::createRenderManagerSDL();
	else if (gameConfig.getString("device") == "GP2X")
		rmanager = RenderManager::createRenderManagerGP2X();
	else if (gameConfig.getString("device") == "OpenGL")
		rmanager = RenderManager::createRenderManagerGL2D();
	else
		rmanager = RenderManager::createRenderManagerGL2D();
	GUIManager::createGUIManager();

	// fullscreen?
	if(gameConfig.getString("fullscreen") == "true")
		rmanager->init(800, 600, true);
	else
		rmanager->init(800, 600, false);
	// Only for Alpha2 Release!!!
	rmanager->setBlobColor(0, 
		Color(gameConfig.getInteger("r1"),
		gameConfig.getInteger("g1"),
		gameConfig.getInteger("b1")));
	rmanager->setBlobColor(1,	
		Color(gameConfig.getInteger("r2"),
		gameConfig.getInteger("g2"),
		gameConfig.getInteger("b2")));
		
	SoundManager* smanager = SoundManager::createSoundManager();
	smanager->init();
	smanager->playSound("sounds/bums.wav", 0.0);
	smanager->playSound("sounds/pfiff.wav", 0.0);

	InputManager* inputmgr = InputManager::createInputManager();
	
	
	PhysicWorld pworld;
	int squish=0;

	int leftScore = 0;
	int rightScore = 0;
	int leftHitcount = 0;
	int rightHitcount = 0;
	
	int servingPlayer = -1;
	
	pworld.reset(0);
	pworld.step();
	
	int running = 1;
	
	GameMode mode = MODE_MAINMENU;
	
	
	while (running)
	{
		inputmgr->updateInput();
		
		switch (mode)
		{
			case MODE_MAINMENU:
			{
				int menuResult = mainmenu();
				if (menuResult == 2)
					running = MODE_MAINMENU;
				if (menuResult == 1)
					mode = MODE_LOCALGAME;

				break;
			}
			case MODE_LOCALGAME:
			{
				if (inputmgr->exit())
					mode = MODE_MAINMENU;
				if (!InputManager::getSingleton()->running())
					running = 0;	
				int gameResult = gameStep(gameConfig, pworld);
				if (gameResult == 0 || gameResult == 1)
					mode = MODE_MAINMENU;
				break;
			}
		}
		rmanager->draw();
		rmanager->refresh();
		correctFramerate();

	}

	rmanager->deinit();
	smanager->deinit();
	
	SDL_Quit();
	PHYSFS_deinit();

}

// Returns the winning player or if the game is not finished returns -1
int gameStep(UserConfig& gameConfig, PhysicWorld& pworld)
{
	static InputSource* leftInput = new LocalInputSource(0);
	static InputSource* rightInput = new LocalInputSource(1);
	
	static int leftScore = 0;
	static int rightScore = 0;
	static int servingPlayer = -1;
	
	static int leftHitcount = 0;
	static int rightHitcount = 0;
	
	static int squish = 0;
	
	RenderManager* rmanager = &RenderManager::getSingleton();
	SoundManager* smanager = &SoundManager::getSingleton();
	pworld.setLeftInput(leftInput->getInput());
	pworld.setRightInput(rightInput->getInput());
	
	rmanager->setBlob(0, pworld.getLeftBlob(),
		pworld.getLeftBlobState());
	rmanager->setBlob(1, pworld.getRightBlob(),
		pworld.getRightBlobState());
	rmanager->setBall(pworld.getBall(), pworld.getBallRotation());
	rmanager->setScore(leftScore, rightScore, 
		servingPlayer == 0, servingPlayer == 1);
		if(0 == squish) // protection Of A Bug (squish)
	{
		if (pworld.ballHitLeftPlayer())
		{
			smanager->playSound("sounds/bums.wav", 
				pworld.lastHitIntensity());
			leftHitcount++;
			rightHitcount = 0;	
			squish=1;
		}		
		if (pworld.ballHitRightPlayer())
		{
			smanager->playSound("sounds/bums.wav",
				pworld.lastHitIntensity());
			rightHitcount++;
			leftHitcount = 0;		
			squish = 1;
	        }
	}
	else
	{
		squish += 1;
	if(squish > 13)
		squish=0;
	}

	if (pworld.ballHitLeftGround() || leftHitcount > 3)
	{
		pworld.dampBall();
		smanager->playSound("sounds/pfiff.wav", 0.3);
		if (servingPlayer == 1)
			rightScore++;
		servingPlayer = 1;
		pworld.setBallValidity(0);
		rightHitcount = 0;
		leftHitcount = 0;
	}
		
	if (pworld.ballHitRightGround() || rightHitcount > 3)
	{
		pworld.dampBall();
		smanager->playSound("sounds/pfiff.wav", 0.3);
		if (servingPlayer == 0)
			leftScore++;
		servingPlayer = 0;
		pworld.setBallValidity(0);
		rightHitcount = 0;
		leftHitcount = 0;		
	}
	
	if (pworld.roundFinished())
		pworld.reset(servingPlayer);
	
	pworld.step();

	float time = float(SDL_GetTicks()) / 1000.0;
	if (gameConfig.getBool("left_blobby_oscillate"))
		rmanager->setBlobColor(0, Color(
			int((sin(time*2) + 1.0) * 128),
			int((sin(time*4) + 1.0) * 128),
			int((sin(time*3) + 1.0) * 128)));
	if (gameConfig.getBool("right_blobby_oscillate"))
		rmanager->setBlobColor(1, Color(
			int((cos(time*2) + 1.0) * 128),
			int((cos(time*4) + 1.0) * 128),
			int((cos(time*3) + 1.0) * 128)));
		
		
	if (leftScore >= 15 && leftScore >= rightScore + 2)
		return 0;
	if (rightScore >= 15 && rightScore >= leftScore + 2)
		return 1;	
	return -1;
}
