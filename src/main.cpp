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

int main(int argc, char* argv[])
{
	PHYSFS_init(argv[0]);
	PHYSFS_addToSearchPath("data", 0);
	PHYSFS_setWriteDir("data");
	
	SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO);
	
	RenderManager *rmanager = RenderManager::createRenderManagerGL2D();
//	RenderManager *rmanager = RenderManager::createRenderManagerSDL();
	rmanager->init(800, 600, false);
	SoundManager* smanager = SoundManager::createSoundManager();
	smanager->init();
	smanager->playSound("sounds/bums.wav", 0.0);
	smanager->playSound("sounds/pfiff.wav", 0.0);

	InputManager* inputmgr = InputManager::createInputManager();
	InputSource* leftInput = new LocalInputSource(0);
	InputSource* rightInput = new LocalInputSource(1);
	
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
	while (running)
	{
		running = inputmgr->running();
		inputmgr->updateInput();
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
		if(squish > 15)
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
		rmanager->setBlobColor(0, Color(
			int((sin(time*2) + 1.0) * 128),
			int((sin(time*4) + 1.0) * 128),
			int((sin(time*3) + 1.0) * 128)));
		rmanager->setBlobColor(1, Color(
			int((cos(time*2) + 1.0) * 128),
			int((cos(time*4) + 1.0) * 128),
			int((cos(time*3) + 1.0) * 128)));
			
		rmanager->draw();
		rmanager->refresh();
		correctFramerate();
		
		if ((leftScore >= 15 || rightScore >= 15) &&
			(leftScore >= rightScore + 2 ||
			rightScore >= leftScore + 2))
			running = 0;
	}

	rmanager->deinit();
	smanager->deinit();
	
	SDL_Quit();
	PHYSFS_deinit();

}

