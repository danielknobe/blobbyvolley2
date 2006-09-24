#include "State.h"
#include "LocalInputSource.h"
#include "RenderManager.h"
#include "SoundManager.h"
#include "DuelMatch.h"
#include "ReplayRecorder.h"
#include "ReplayInputSource.h"
#include "ScriptedInputSource.h"
#include "IMGUI.h"
#include "SpeedController.h"

#include <physfs.h>
#include <ctime>
#include <sstream>

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
	if (mRecorder)
		delete mRecorder;

	InputManager::getSingleton()->endGame();
}

LocalGameState::LocalGameState(GameMode mode)
	: State()
{
	mRecorder = 0;
	mPaused = false;

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

	SoundManager::getSingleton().playSound("sounds/pfiff.wav", 0.2);
	
	InputSource* linput;
	InputSource* rinput;

	if (gameConfig.getBool("left_player_human"))
	{
		linput = new LocalInputSource(LEFT_PLAYER);
	}
	else
	{
		linput = new ScriptedInputSource("scripts/" +
			gameConfig.getString("left_script_name"), LEFT_PLAYER);
	}
	
	if (gameConfig.getBool("right_player_human"))
	{
		rinput = new LocalInputSource(RIGHT_PLAYER);
	}
	else
	{
		rinput = new ScriptedInputSource("scripts/" +
			gameConfig.getString("right_script_name"),
						RIGHT_PLAYER);
	}
	if (gameConfig.getBool("record_replay"))
	{
		mode = MODE_RECORDING_DUEL;
	}
	mRecorder = new ReplayRecorder(mode, ReplayMenuState::getRecordName());
	
	mLeftInput = mRecorder->createReplayInputSource(LEFT_PLAYER,
			linput);
	mRightInput = mRecorder->createReplayInputSource(RIGHT_PLAYER,
			rinput);

	mMatch = new DuelMatch(mLeftInput, mRightInput, true, true);
}

void LocalGameState::step()
{
	RenderManager* rmanager = &RenderManager::getSingleton();
	

	if (mPaused)
	{
		IMGUI& imgui = IMGUI::getSingleton();
		imgui.doOverlay(GEN_ID, Vector2(180, 200), Vector2(670, 400));
		imgui.doText(GEN_ID, Vector2(234, 280), "Wirklich Beenden?");
		if (InputManager::getSingleton()->select() ||
				InputManager::getSingleton()->click())
			mPaused = false;
	}
	else
	{
		mMatch->step();
	}

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
	else if (InputManager::getSingleton()->exit())
	{
		if (mPaused == true)
		{
			delete this;
			mCurrentState = new MainMenuState;
		}
		else
			mPaused = true;
	}
	else if (mRecorder->endOfFile())
	{
		delete this;
		mCurrentState = new MainMenuState;
	}
}


MainMenuState::MainMenuState()
{
	IMGUI::getSingleton().resetSelection();
}

MainMenuState::~MainMenuState()
{
}

void MainMenuState::step()
{
	RenderManager::getSingleton().drawGame(false);
	IMGUI& imgui = IMGUI::getSingleton();
	
	imgui.doCursor();
	imgui.doImage(GEN_ID, Vector2(400.0, 300.0), "gfx/strand2.bmp");
	imgui.doOverlay(GEN_ID, Vector2(0.0, 0.0), Vector2(800.0, 600.0));
	imgui.doImage(GEN_ID, Vector2(250.0, 210.0), "gfx/titel.bmp");

	if (imgui.doButton(GEN_ID, Vector2(484.0, 400.0), "start"))
	{
		delete mCurrentState;
		try
		{
			mCurrentState = new LocalGameState(MODE_NORMAL_DUEL);
		}
		catch (ScriptException except)
		{
			FILE* file = fopen("lualog.txt", "wb");
			fprintf(file, "Lua Error: %s\n",
				except.luaerror.c_str());
			fclose(file);
			mCurrentState = new MainMenuState();
		}
	}
	
	if (imgui.doButton(GEN_ID, Vector2(484.0, 430.0), "options"))
	{
		delete mCurrentState;
		mCurrentState = new OptionState();
	}

	if (imgui.doButton(GEN_ID, Vector2(484.0, 460.0), "watch replay"))
	{
		delete mCurrentState;
		mCurrentState = new ReplayMenuState();
	}
	
	if (imgui.doButton(GEN_ID, Vector2(484.0, 490.0), "exit")) 
	{
		RenderManager::getSingleton().deinit();
		SoundManager::getSingleton().deinit();
		SDL_Quit();
		exit(0);
	}
}

WinState::WinState(PlayerSide player)
{
	mPlayer = player;
}

void WinState::step()
{
	char buf[64];
	snprintf(buf, 64, "Spieler %d", mPlayer + 1);
	
	IMGUI::getSingleton().doOverlay(GEN_ID, Vector2(200, 150), Vector2(650, 450));
	IMGUI::getSingleton().doImage(GEN_ID, Vector2(200, 250), "gfx/pokal.bmp");
	IMGUI::getSingleton().doText(GEN_ID, Vector2(274, 250), buf);
	IMGUI::getSingleton().doText(GEN_ID, Vector2(274, 300), "hat gewonnen!");
	
	InputManager* inputmgr = InputManager::getSingleton();
	if (inputmgr->click() || inputmgr->select())
	{
		delete mCurrentState;
		mCurrentState = new MainMenuState();
	}
}

OptionState::OptionState()
{
	IMGUI::getSingleton().resetSelection();
	mSaveConfig = false;
	mOptionConfig.loadFile("config.xml");
	mPlayerOptions[LEFT_PLAYER] = 0;
	mPlayerOptions[RIGHT_PLAYER] = 0;
	mScriptNames.push_back("");
	std::string leftScript = mOptionConfig.getString("left_script_name");
	std::string rightScript = mOptionConfig.getString("right_script_name");
	char** filenames = PHYSFS_enumerateFiles("scripts");
	for (int i = 0; filenames[i] != 0; ++i)
	{
		std::string tmp(filenames[i]);
		if (tmp.find(".lua") != std::string::npos)
		{
			mScriptNames.push_back(tmp);
			int pos = mScriptNames.size() - 1;
			if (tmp == leftScript)
				mPlayerOptions[LEFT_PLAYER] = pos;
			if (tmp == rightScript)
				mPlayerOptions[RIGHT_PLAYER] = pos;
		}
			
	}
	if (mOptionConfig.getBool("left_player_human"))
		mPlayerOptions[LEFT_PLAYER] = 0;
	if (mOptionConfig.getBool("right_player_human"))
		mPlayerOptions[RIGHT_PLAYER] = 0;
	PHYSFS_freeList(filenames);
	mReplayActivated = mOptionConfig.getBool("record_replay");
}

OptionState::~OptionState()
{
	if (mSaveConfig)
	{
		mOptionConfig.setBool("record_replay", mReplayActivated);
		if (mPlayerOptions[LEFT_PLAYER] == 0)
		{
			mOptionConfig.setBool(
				"left_player_human", true);
		}
		else
		{
			mOptionConfig.setBool(
				"left_player_human", false);
			mOptionConfig.setString("left_script_name",
				mScriptNames[mPlayerOptions[LEFT_PLAYER]]);
		}
		
		if (mPlayerOptions[RIGHT_PLAYER] == 0)
		{
			mOptionConfig.setBool(
				"right_player_human", true);
		}
		else
		{
			mOptionConfig.setBool(
				"right_player_human", false);
			mOptionConfig.setString("right_script_name",
				mScriptNames[mPlayerOptions[RIGHT_PLAYER]]);
		}
		mOptionConfig.saveFile("config.xml");
	}
}

void OptionState::step()
{
	IMGUI& imgui = IMGUI::getSingleton();
	imgui.doCursor();
	imgui.doImage(GEN_ID, Vector2(400.0, 300.0), "gfx/strand2.bmp");
	imgui.doOverlay(GEN_ID, Vector2(0.0, 0.0), Vector2(800.0, 600.0));
	imgui.doText(GEN_ID, Vector2(34.0, 10.0), "left player");
	imgui.doText(GEN_ID, Vector2(434.0, 10.0), "right player");

	//Nur zum test:
	/*static std::string bla = "TEST";
	static unsigned cpos = 3;
	imgui.doEditbox(GEN_ID, Vector2(450.0 , 300.0), bla, cpos);*/

	imgui.doImage(GEN_ID, Vector2(12.0 + 4.0,
		mPlayerOptions[LEFT_PLAYER] * 28.0 + 12.0 + 50.0), "gfx/pfeil_rechts.bmp");
	imgui.doImage(GEN_ID, Vector2(12.0 + 404.0,
		mPlayerOptions[RIGHT_PLAYER] * 28.0 + 12.0 + 50.0), "gfx/pfeil_rechts.bmp");

	if (imgui.doButton(GEN_ID, Vector2(34.0, 50.0), "human"))
		mPlayerOptions[LEFT_PLAYER] = 0;
	for (unsigned int i = 1; i < mScriptNames.size(); i++)
	{
		if (imgui.doButton(GEN_ID << 4 + i,
				Vector2(34.0, 50.0 + i * 28.0), mScriptNames[i]))
			mPlayerOptions[LEFT_PLAYER] = i;
	}
	
	if (imgui.doButton(GEN_ID, Vector2(434.0, 50.0), "human"))
		mPlayerOptions[RIGHT_PLAYER] = 0;
	for (unsigned int i = 1; i < mScriptNames.size(); i++)
	{
		if (imgui.doButton(GEN_ID << 5 + i,
					Vector2(434.0, 50.0 + i * 28.0), mScriptNames[i]))
			mPlayerOptions[RIGHT_PLAYER] = i;
	}

	imgui.doText(GEN_ID, Vector2(34.0, 300.0), "Volume:");
	float volume = mOptionConfig.getFloat("global_volume");
	if (imgui.doScrollbar(GEN_ID, Vector2(34.0, 340.0), volume))
	{
		mOptionConfig.setFloat("global_volume", volume);
		SoundManager::getSingleton().setVolume(volume);
		SoundManager::getSingleton().playSound("sounds/bums.wav", 1.0);
	}
	imgui.doText(GEN_ID, Vector2(34.0, 380.0), "Gamespeed:");
	float gamefps = (mOptionConfig.getInteger("gamefps")-30)/300.0;
	if (gamefps < 0)
		gamefps = 0;
	if (imgui.doScrollbar(GEN_ID, Vector2(34.0, 420.0), gamefps))
	{
		int gamefpsint = (int)(gamefps*300.0+30);
		mOptionConfig.setInteger("gamefps", gamefpsint);
		SpeedController::getMainInstance()->setGameSpeed(gamefpsint);
	}
	if (imgui.doButton(GEN_ID, Vector2(34.0, 460.0), "record replays"))
	{
		mReplayActivated = !mReplayActivated;
	}
	if (mReplayActivated)
	{
		imgui.doImage(GEN_ID, Vector2(16.0, 472.0), "gfx/pfeil_rechts.bmp");
	}

	if (imgui.doButton(GEN_ID, Vector2(434.0, 420.0), "input options"))
	{
		mSaveConfig = true;
		delete this;
		mCurrentState = new InputOptionsState();
		return;
	}
	if (imgui.doButton(GEN_ID, Vector2(434.0, 460.0), "graphic options"))
	{
		mSaveConfig = true;
		delete this;
		mCurrentState = new GraphicOptionsState();
		return;
	}

	if (imgui.doButton(GEN_ID, Vector2(224.0, 530.0), "ok"))
	{
		mSaveConfig = true;
		delete this;
		mCurrentState = new MainMenuState();
		return;
	}
	if (imgui.doButton(GEN_ID, Vector2(424.0, 530.0), "cancel"))
	{
		delete this;
		mCurrentState = new MainMenuState();
	}
}

ReplayMenuState::ReplayMenuState()
{
	IMGUI::getSingleton().resetSelection();
	mReplaying = false;
	mReplayMatch = 0;
	mReplayRecorder = 0;
	mSelectedReplay = -1;
	char** filenames = PHYSFS_enumerateFiles("replays");
	for (int i = 0; filenames[i] != 0; ++i)
	{
		std::string tmp(filenames[i]);
		if (tmp.find(".xml") != std::string::npos)
		{
			mReplayFiles.push_back(tmp);
		}
	}
	std::sort(mReplayFiles.rbegin(), mReplayFiles.rend());
	
	UserConfig gameConfig;
	gameConfig.loadFile("config.xml");
	mLeftOscillate = gameConfig.getBool("left_blobby_oscillate");
	mRightOscillate = gameConfig.getBool("right_blobby_oscillate");
	RenderManager::getSingleton().setBlobColor(LEFT_PLAYER, 
		Color(gameConfig.getInteger("r1"),
		gameConfig.getInteger("g1"),
		gameConfig.getInteger("b1")));
	RenderManager::getSingleton().setBlobColor(RIGHT_PLAYER,
		Color(gameConfig.getInteger("r2"),
		gameConfig.getInteger("g2"),
		gameConfig.getInteger("b2")));
}

ReplayMenuState::~ReplayMenuState()
{
	if (mReplayMatch)
	{
		delete mReplayMatch;
	}
	if (mReplayRecorder)
	{
		delete mReplayRecorder;
	}
}

std::string ReplayMenuState::getRecordName()
{
	char buf[256];
	snprintf(buf, 256, "replays/replay_%d.xml", static_cast<int>(time(0)));
	return std::string(buf);
}

void ReplayMenuState::step()
{
	if (mReplaying)
	{
		RenderManager* rmanager = &RenderManager::getSingleton();
		
		mReplayMatch->step();
		float time = float(SDL_GetTicks()) / 1000.0;
		if (mLeftOscillate)
		{
			rmanager->setBlobColor(0, Color(
				int((sin(time*2) + 1.0) * 128),
				int((sin(time*4) + 1.0) * 128),
				int((sin(time*3) + 1.0) * 128)));
		}
		if (mRightOscillate)
		{
			rmanager->setBlobColor(1, Color(
				int((cos(time*2) + 1.0) * 128),
				int((cos(time*4) + 1.0) * 128),
				int((cos(time*3) + 1.0) * 128)));
		}
		
		PlayerSide side = mReplayMatch->winningPlayer();
		if (side != NO_PLAYER)
		{
			delete this;
			mCurrentState = new WinState(side);
		}
		else if (InputManager::getSingleton()->exit())
		{
			delete this;
			mCurrentState = new MainMenuState();
		}
		else if (mReplayRecorder->endOfFile())
		{
			delete this;
			mCurrentState = new MainMenuState();
		}
	}
	else
	{
		IMGUI& imgui = IMGUI::getSingleton();
		imgui.doCursor();
		imgui.doImage(GEN_ID, Vector2(400.0, 300.0), "gfx/strand2.bmp");
		imgui.doOverlay(GEN_ID, Vector2(0.0, 0.0), Vector2(800.0, 600.0));
		if (mSelectedReplay != -1)
		{
			imgui.doImage(GEN_ID, Vector2(16.0, 62.0 + 30.0 * mSelectedReplay),
					"gfx/pfeil_rechts.bmp");
		}

		if (imgui.doButton(GEN_ID, Vector2(224.0, 10.0), "play") && 
					mSelectedReplay != -1)
		{
			mReplayRecorder = new ReplayRecorder(MODE_REPLAY_DUEL,
				std::string("replays/" +
					mReplayFiles[mSelectedReplay]));
			InputSource* linput = mReplayRecorder->
					createReplayInputSource(LEFT_PLAYER,
					new DummyInputSource);
			 InputSource* rinput = mReplayRecorder->
			 		createReplayInputSource(RIGHT_PLAYER,
			 		new DummyInputSource);
			mReplaying = true;
			mReplayMatch = new DuelMatch(linput, rinput,
							true, true);
			SoundManager::getSingleton().playSound(
					"sounds/pfiff.wav", 0.2);
		}
		else if (imgui.doButton(GEN_ID, Vector2(424.0, 10.0), "cancel"))
		{
			delete this;
			mCurrentState = new MainMenuState();
		}
		else for (unsigned int i = 0; i < mReplayFiles.size(); i++)
		{
			if (imgui.doButton(GEN_ID << 6 + i, Vector2(34.0, 50.0 + 30.0 * i),
						mReplayFiles[i]))
			{
				mSelectedReplay = i;
				break;
			}
		}
		if (imgui.doButton(GEN_ID, Vector2(644.0, 60.0), "delete") &&
					mSelectedReplay != -1)
		{
			PHYSFS_delete(std::string("replays/" +
				mReplayFiles[mSelectedReplay]).c_str());
			delete this;
			mCurrentState = new ReplayMenuState();
		}
	}
}

GraphicOptionsState::GraphicOptionsState()
{
	IMGUI::getSingleton().resetSelection();
	mSaveConfig = false;
	mOptionConfig.loadFile("config.xml");
	mFullscreen = mOptionConfig.getBool("fullscreen");
	mRenderer = mOptionConfig.getString("device");
	mR1 = mOptionConfig.getInteger("r1");
	mG1 = mOptionConfig.getInteger("g1");
	mB1 = mOptionConfig.getInteger("b1");
	mR2 = mOptionConfig.getInteger("r2");
	mG2 = mOptionConfig.getInteger("g2");
	mB2 = mOptionConfig.getInteger("b2");
	mLeftMorphing = mOptionConfig.getBool("left_blobby_oscillate");
	mRightMorphing = mOptionConfig.getBool("right_blobby_oscillate");
}

GraphicOptionsState::~GraphicOptionsState()
{
	if (mSaveConfig)
	{
		if ((mOptionConfig.getBool("fullscreen") != mFullscreen) ||
			(mOptionConfig.getString("device") != mRenderer))
		{
			mOptionConfig.setBool("fullscreen", mFullscreen);
			mOptionConfig.setString("device", mRenderer);
			if (mRenderer == "OpenGL")
				RenderManager::createRenderManagerGL2D()->init(800, 600, mFullscreen);
			else
				RenderManager::createRenderManagerSDL()->init(800, 600, mFullscreen);
		}
		mOptionConfig.setInteger("r1", mR1);
		mOptionConfig.setInteger("g1", mG1);
		mOptionConfig.setInteger("b1", mB1);
		mOptionConfig.setInteger("r2", mR2);
		mOptionConfig.setInteger("g2", mG2);
		mOptionConfig.setInteger("b2", mB2);
		mOptionConfig.setBool("left_blobby_oscillate", mLeftMorphing);
		mOptionConfig.setBool("right_blobby_oscillate", mRightMorphing);

		mOptionConfig.saveFile("config.xml");
	}
}

void GraphicOptionsState::step()
{
	IMGUI& imgui = IMGUI::getSingleton();
	imgui.doCursor();
	imgui.doImage(GEN_ID, Vector2(400.0, 300.0), "gfx/strand2.bmp");
	imgui.doOverlay(GEN_ID, Vector2(0.0, 0.0), Vector2(800.0, 600.0));

	imgui.doText(GEN_ID, Vector2(34.0, 50.0), "Video Settings");
	
	if (imgui.doButton(GEN_ID, Vector2(34.0, 80.0), "Fullscreen Mode"))
		mFullscreen = true;
	if (imgui.doButton(GEN_ID, Vector2(34.0, 110.0), "Window Mode"))
		mFullscreen = false;
	if (mFullscreen)
		imgui.doImage(GEN_ID, Vector2(18.0, 92.0), "gfx/pfeil_rechts.bmp");
	else
		imgui.doImage(GEN_ID, Vector2(18.0, 122.0), "gfx/pfeil_rechts.bmp");

	imgui.doText(GEN_ID, Vector2(444.0, 50.0), "Render Device");
	if (imgui.doButton(GEN_ID, Vector2(444.0, 80.0), "OpenGL"))
		mRenderer = "OpenGL";
	if (imgui.doButton(GEN_ID, Vector2(444.0, 110.0), "SDL"))
		mRenderer = "SDL";
	if (mRenderer == "OpenGL")
		imgui.doImage(GEN_ID, Vector2(428.0, 92.0), "gfx/pfeil_rechts.bmp");
	else
		imgui.doImage(GEN_ID, Vector2(428.0, 122.0), "gfx/pfeil_rechts.bmp");

	//Blob colors:
	imgui.doText(GEN_ID, Vector2(280.0, 170.0), "blob colors");
	//left blob:
	imgui.doText(GEN_ID, Vector2(34.0, 220.0), "left player");
	{
		imgui.doText(GEN_ID, Vector2(34.0, 250), "red");
		float r1 = (float)mR1/255;
		imgui.doScrollbar(GEN_ID, Vector2(160.0, 250.0), r1);
		mR1 = (int)(r1*255);
	}
	{
		imgui.doText(GEN_ID, Vector2(34.0, 280), "green");
		float g1 = (float)mG1/255;
		imgui.doScrollbar(GEN_ID, Vector2(160.0, 280.0), g1);
		mG1 = (int)(g1*255);
	}
	{
		imgui.doText(GEN_ID, Vector2(34.0, 310), "blue");
		float b1 = (float)mB1/255;
		imgui.doScrollbar(GEN_ID, Vector2(160.0, 310), b1);
		mB1 = (int)(b1*255);
	}
	imgui.doText(GEN_ID, Vector2(34.0, 350), "morphing blob?");
	if (imgui.doButton(GEN_ID, Vector2(72.0, 380), "yes"))
		mLeftMorphing = true;
	if (imgui.doButton(GEN_ID, Vector2(220.0, 380), "no"))
		mLeftMorphing = false;
	if (mLeftMorphing)
		imgui.doImage(GEN_ID, Vector2(54.0, 392.0), "gfx/pfeil_rechts.bmp");
	else
		imgui.doImage(GEN_ID, Vector2(204.0, 392.0), "gfx/pfeil_rechts.bmp");
	//draw left blob:
	{
		float time = float(SDL_GetTicks()) / 1000.0;
		Color ourCol = Color(mR1, mG1, mB1);
		if (mLeftMorphing)
			ourCol = Color(int((sin(time*2) + 1.0) * 128),
							int((sin(time*4) + 1.0) * 128),
							int((sin(time*3) + 1.0) * 128));
		imgui.doBlob(GEN_ID, Vector2(120, 490), ourCol);
	}

	//right blob:
	imgui.doText(GEN_ID, Vector2(434.0, 220.0), "right player");
	{
		imgui.doText(GEN_ID, Vector2(434.0, 250), "red");
		float r2 = (float)mR2/255;
		imgui.doScrollbar(GEN_ID, Vector2(560.0, 250.0), r2);
		mR2 = (int)(r2*255);
	}
	{
		imgui.doText(GEN_ID, Vector2(434.0, 280), "green");
		float g2 = (float)mG2/255;
		imgui.doScrollbar(GEN_ID, Vector2(560.0, 280.0), g2);
		mG2 = (int)(g2*255);
	}
	{
		imgui.doText(GEN_ID, Vector2(434.0, 310), "blue");
		float b2 = (float)mB2/255;
		imgui.doScrollbar(GEN_ID, Vector2(560.0, 310), b2);
		mB2 = (int)(b2*255);
	}
	imgui.doText(GEN_ID, Vector2(434.0, 350), "morphing blob?");
	if (imgui.doButton(GEN_ID, Vector2(472.0, 380), "yes"))
		mRightMorphing = true;
	if (imgui.doButton(GEN_ID, Vector2(620.0, 380), "no"))
		mRightMorphing = false;
	if (mRightMorphing)
		imgui.doImage(GEN_ID, Vector2(454.0, 392.0), "gfx/pfeil_rechts.bmp");
	else
		imgui.doImage(GEN_ID, Vector2(604.0, 392.0), "gfx/pfeil_rechts.bmp");
	//draw right blob:
	{
		float time = float(SDL_GetTicks()) / 1000.0;
		Color ourCol = Color(mR2, mG2, mB2);
		if (mRightMorphing)
			ourCol = Color(int((cos(time*2) + 1.0) * 128),
							int((cos(time*4) + 1.0) * 128),
							int((cos(time*3) + 1.0) * 128));
		imgui.doBlob(GEN_ID, Vector2(660, 490), ourCol);
	}

	if (imgui.doButton(GEN_ID, Vector2(224.0, 530.0), "ok"))
	{
		mSaveConfig = true;
		delete this;
		mCurrentState = new OptionState();
	}
	if (imgui.doButton(GEN_ID, Vector2(424.0, 530.0), "cancel"))
	{
		delete this;
		mCurrentState = new OptionState();
	}
}

InputOptionsState::InputOptionsState()
{
	IMGUI::getSingleton().resetSelection();
	mSaveConfig = false;
	mOptionConfig.loadFile("inputconfig.xml");
	//left data:
	mLeftBlobbyDevice = mOptionConfig.getString("left_blobby_device");
	mLeftBlobbyMouseJumpbutton = mOptionConfig.getInteger("left_blobby_mouse_jumpbutton");
	mLeftBlobbyKeyboardLeft = mOptionConfig.getString("left_blobby_keyboard_left");
	mLeftBlobbyKeyboardRight = mOptionConfig.getString("left_blobby_keyboard_right");
	mLeftBlobbyKeyboardJump = mOptionConfig.getString("left_blobby_keyboard_jump");
	mLeftBlobbyJoystickLeft = mOptionConfig.getString("left_blobby_joystick_left");
	mLeftBlobbyJoystickRight = mOptionConfig.getString("left_blobby_joystick_right");
	mLeftBlobbyJoystickJump = mOptionConfig.getString("left_blobby_joystick_jump");
	//right data:
	mRightBlobbyDevice = mOptionConfig.getString("right_blobby_device");
	mRightBlobbyMouseJumpbutton = mOptionConfig.getInteger("right_blobby_mouse_jumpbutton");
	mRightBlobbyKeyboardLeft = mOptionConfig.getString("right_blobby_keyboard_left");
	mRightBlobbyKeyboardRight = mOptionConfig.getString("right_blobby_keyboard_right");
	mRightBlobbyKeyboardJump = mOptionConfig.getString("right_blobby_keyboard_jump");
	mRightBlobbyJoystickLeft = mOptionConfig.getString("right_blobby_joystick_left");
	mRightBlobbyJoystickRight = mOptionConfig.getString("right_blobby_joystick_right");
	mRightBlobbyJoystickJump = mOptionConfig.getString("right_blobby_joystick_jump");
}

InputOptionsState::~InputOptionsState()
{
	if (mSaveConfig)
	{
		//left data:
		mOptionConfig.setString("left_blobby_device", mLeftBlobbyDevice);
		mOptionConfig.setInteger("left_blobby_mouse_jumpbutton", mLeftBlobbyMouseJumpbutton);
		mOptionConfig.setString("left_blobby_keyboard_left", mLeftBlobbyKeyboardLeft);
		mOptionConfig.setString("left_blobby_keyboard_right", mLeftBlobbyKeyboardRight);
		mOptionConfig.setString("left_blobby_keyboard_jump", mLeftBlobbyKeyboardJump);
		mOptionConfig.setString("left_blobby_joystick_left", mLeftBlobbyJoystickLeft);
		mOptionConfig.setString("left_blobby_joystick_right", mLeftBlobbyJoystickRight);
		mOptionConfig.setString("left_blobby_joystick_jump", mLeftBlobbyJoystickJump);
		//right data:
		mOptionConfig.setString("right_blobby_device", mRightBlobbyDevice);
		mOptionConfig.setInteger("right_blobby_mouse_jumpbutton", mRightBlobbyMouseJumpbutton);
		mOptionConfig.setString("right_blobby_keyboard_left", mRightBlobbyKeyboardLeft);
		mOptionConfig.setString("right_blobby_keyboard_right", mRightBlobbyKeyboardRight);
		mOptionConfig.setString("right_blobby_keyboard_jump", mRightBlobbyKeyboardJump);
		mOptionConfig.setString("right_blobby_joystick_left", mRightBlobbyJoystickLeft);
		mOptionConfig.setString("right_blobby_joystick_right", mRightBlobbyJoystickRight);
		mOptionConfig.setString("right_blobby_joystick_jump", mRightBlobbyJoystickJump);

		mOptionConfig.saveFile("inputconfig.xml");
	}
}

void InputOptionsState::step()
{
	IMGUI& imgui = IMGUI::getSingleton();
	imgui.doCursor();
	imgui.doImage(GEN_ID, Vector2(400.0, 300.0), "gfx/strand2.bmp");
	imgui.doOverlay(GEN_ID, Vector2(0.0, 0.0), Vector2(800.0, 600.0));

	std::string lastActionKey = InputManager::getSingleton()->getLastActionKey();

	//check if a capture window is open, to set all widgets inactive:
	if (mLeftBlobbyKeyboardLeft != "" && mLeftBlobbyKeyboardRight != "" && mLeftBlobbyKeyboardJump != "" && mLeftBlobbyJoystickLeft != "" && mLeftBlobbyJoystickRight != "" && mLeftBlobbyJoystickJump != "" && mLeftBlobbyMouseJumpbutton != -1 && mRightBlobbyKeyboardLeft != "" && mRightBlobbyKeyboardRight != "" && mRightBlobbyKeyboardJump != "" && mRightBlobbyJoystickLeft != "" && mRightBlobbyJoystickRight != "" && mRightBlobbyJoystickJump != "" && mRightBlobbyMouseJumpbutton != -1)
	{
		imgui.doCursor(true);
		imgui.doInactiveMode(false);
	}
	else
	{
		imgui.doInactiveMode(true);
		imgui.doCursor(false);
	}

	//left player side:
	imgui.doText(GEN_ID, Vector2(34.0, 10.0), "left player");
	if (imgui.doButton(GEN_ID, Vector2(80.0, 60.0), mLeftBlobbyDevice))
	{
		if (mLeftBlobbyDevice == "mouse")
			mLeftBlobbyDevice = "keyboard";
		else if (mLeftBlobbyDevice == "keyboard")
			mLeftBlobbyDevice = "joystick";
		else if (mLeftBlobbyDevice == "joystick")
			if (mRightBlobbyDevice != "mouse")
				mLeftBlobbyDevice = "mouse";
			else
				mLeftBlobbyDevice = "keyboard";
	}
	//if mouse device is selected:
	if (mLeftBlobbyDevice == "mouse")
	{
		imgui.doText(GEN_ID, Vector2(34.0, 120.0), "Jump Button");
		std::ostringstream text;
		if (mLeftBlobbyMouseJumpbutton >= 0)
			text << "Button " << mLeftBlobbyMouseJumpbutton;
		else
			text << "Button ";
		if (imgui.doButton(GEN_ID, Vector2(50, 150.0), text.str()))
		{
			oldInteger = mLeftBlobbyMouseJumpbutton;
			mLeftBlobbyMouseJumpbutton = -2;
		}
	}
	if ((mLeftBlobbyMouseJumpbutton == -2) && (InputManager::getSingleton()->getLastMouseButton() == -1))
		mLeftBlobbyMouseJumpbutton = -1;
	//if keyboard device is selected:
	if (mLeftBlobbyDevice == "keyboard")
	{
		imgui.doText(GEN_ID, Vector2(34.0, 120.0), "Left Key");
		if (imgui.doButton(GEN_ID, Vector2(50, 150.0), std::string("Key ")+mLeftBlobbyKeyboardLeft))
		{
			lastActionKey = "";
			oldString = mLeftBlobbyKeyboardLeft;
			mLeftBlobbyKeyboardLeft = "";
		}
		imgui.doText(GEN_ID, Vector2(34.0, 190.0), "Right Key");
		if (imgui.doButton(GEN_ID, Vector2(50, 220.0), std::string("Key ")+mLeftBlobbyKeyboardRight))
		{
			lastActionKey = "";
			oldString = mLeftBlobbyKeyboardRight; 
			mLeftBlobbyKeyboardRight = "";
		}
		imgui.doText(GEN_ID, Vector2(34.0, 260.0), "Jump Key");
		if (imgui.doButton(GEN_ID, Vector2(50, 290.0), std::string("Key ")+mLeftBlobbyKeyboardJump))
		{
			lastActionKey = "";
			oldString = mLeftBlobbyKeyboardJump; 
			mLeftBlobbyKeyboardJump = "";
		}
	}
	//if joystick device is selected:
	if (mLeftBlobbyDevice == "joystick")
	{
		imgui.doText(GEN_ID, Vector2(34.0, 120.0), "Left Button");
		if (imgui.doButton(GEN_ID, Vector2(50, 150.0), mLeftBlobbyJoystickLeft))
		{
			oldString = mLeftBlobbyJoystickLeft; 
			mLeftBlobbyJoystickLeft = "";
		}
		imgui.doText(GEN_ID, Vector2(34.0, 190.0), "Right Button");
		if (imgui.doButton(GEN_ID, Vector2(50, 220.0), mLeftBlobbyJoystickRight))
		{
			oldString = mLeftBlobbyJoystickRight; 
			mLeftBlobbyJoystickRight = "";
		}
		imgui.doText(GEN_ID, Vector2(34.0, 260.0), "Jump Button");
		if (imgui.doButton(GEN_ID, Vector2(50, 290.0), mLeftBlobbyJoystickJump))
		{
			oldString = mLeftBlobbyJoystickJump; 
			mLeftBlobbyJoystickJump = "";
		}
	}

	//right player side:
	imgui.doText(GEN_ID, Vector2(434.0, 10.0), "right player");
	if (imgui.doButton(GEN_ID, Vector2(480.0, 60.0), mRightBlobbyDevice))
	{
		if (mRightBlobbyDevice == "mouse")
			mRightBlobbyDevice = "keyboard";
		else if (mRightBlobbyDevice == "keyboard")
			mRightBlobbyDevice = "joystick";
		else if (mRightBlobbyDevice == "joystick")
			if (mLeftBlobbyDevice != "mouse")
				mRightBlobbyDevice = "mouse";
			else
				mRightBlobbyDevice = "keyboard";
	}
	//if mouse device is selected:
	if (mRightBlobbyDevice == "mouse")
	{
		imgui.doText(GEN_ID, Vector2(434.0, 120.0), "Jump Button");
		std::ostringstream text;
		if (mRightBlobbyMouseJumpbutton >= 0)
			text << "Button " << mRightBlobbyMouseJumpbutton;
		else
			text << "Button ";
		if (imgui.doButton(GEN_ID, Vector2(450, 150.0), text.str()))
		{
			oldInteger = mRightBlobbyMouseJumpbutton; 
			mRightBlobbyMouseJumpbutton = -2;
		}
	}
	if ((mRightBlobbyMouseJumpbutton == -2) && (InputManager::getSingleton()->getLastMouseButton() == -1))
		mRightBlobbyMouseJumpbutton = -1;
	//if keyboard device is selected:
	if (mRightBlobbyDevice == "keyboard")
	{
		imgui.doText(GEN_ID, Vector2(434.0, 120.0), "Right Key");
		if (imgui.doButton(GEN_ID, Vector2(450, 150.0), std::string("Key ")+mRightBlobbyKeyboardLeft))
		{
			lastActionKey = "";
			oldString = mRightBlobbyKeyboardLeft; 
			mRightBlobbyKeyboardLeft = "";
		}
		imgui.doText(GEN_ID, Vector2(434.0, 190.0), "Right Key");
		if (imgui.doButton(GEN_ID, Vector2(450, 220.0), std::string("Key ")+mRightBlobbyKeyboardRight))
		{
			lastActionKey = "";
			oldString = mRightBlobbyKeyboardRight; 
			mRightBlobbyKeyboardRight = "";
		}
		imgui.doText(GEN_ID, Vector2(434.0, 260.0), "Jump Key");
		if (imgui.doButton(GEN_ID, Vector2(450, 290.0), std::string("Key ")+mRightBlobbyKeyboardJump))
		{
			lastActionKey = "";
			oldString = mRightBlobbyKeyboardJump; 
			mRightBlobbyKeyboardJump = "";
		}
	}
	//if joystick device is selected:
	if (mRightBlobbyDevice == "joystick")
	{
		imgui.doText(GEN_ID, Vector2(434.0, 120.0), "Left Button");
		if (imgui.doButton(GEN_ID, Vector2(450, 150.0), mRightBlobbyJoystickLeft))
		{
			oldString = mRightBlobbyJoystickLeft; 
			mRightBlobbyJoystickLeft = "";
		}
		imgui.doText(GEN_ID, Vector2(434.0, 190.0), "Right Button");
		if (imgui.doButton(GEN_ID, Vector2(450, 220.0), mRightBlobbyJoystickRight))
		{
			oldString = mRightBlobbyJoystickRight; 
			mRightBlobbyJoystickRight = "";
		}
		imgui.doText(GEN_ID, Vector2(434.0, 260.0), "Jump Button");
		if (imgui.doButton(GEN_ID, Vector2(450, 290.0), mRightBlobbyJoystickJump))
		{
			oldString = mRightBlobbyJoystickJump; 
			mRightBlobbyJoystickJump = "";
		}
	}
	
	//Capture dialogs:
	if (mLeftBlobbyMouseJumpbutton == -1)
	{
		imgui.doOverlay(GEN_ID, Vector2(100.0, 150.0), Vector2(700.0, 450.0));
		imgui.doText(GEN_ID, Vector2(180.0, 250.0), "Press Mouse Button");
		imgui.doText(GEN_ID, Vector2(290.0, 300.0), "For Jump");
		mLeftBlobbyMouseJumpbutton = InputManager::getSingleton()->getLastMouseButton();
		if (InputManager::getSingleton()->exit())
			mLeftBlobbyMouseJumpbutton = oldInteger;
	}
	if (mLeftBlobbyKeyboardLeft == "")
	{
		imgui.doOverlay(GEN_ID, Vector2(100.0, 150.0), Vector2(700.0, 450.0));
		imgui.doText(GEN_ID, Vector2(250.0, 250.0), "Press Key For");
		imgui.doText(GEN_ID, Vector2(270.0, 300.0), "Moving Left");
		mLeftBlobbyKeyboardLeft = lastActionKey;
		if (InputManager::getSingleton()->exit())
			mLeftBlobbyKeyboardLeft = oldString; 
	}
	if (mLeftBlobbyKeyboardRight == "")
	{
		imgui.doOverlay(GEN_ID, Vector2(100.0, 150.0), Vector2(700.0, 450.0));
		imgui.doText(GEN_ID, Vector2(250.0, 250.0), "Press Key For");
		imgui.doText(GEN_ID, Vector2(270.0, 300.0), "Moving Right");
		mLeftBlobbyKeyboardRight = lastActionKey;
		if (InputManager::getSingleton()->exit())
			mLeftBlobbyKeyboardRight = oldString;
	}
	if (mLeftBlobbyKeyboardJump == "")
	{
		imgui.doOverlay(GEN_ID, Vector2(100.0, 150.0), Vector2(700.0, 450.0));
		imgui.doText(GEN_ID, Vector2(250.0, 250.0), "Press Key For");
		imgui.doText(GEN_ID, Vector2(270.0, 300.0), "Jumping");
		mLeftBlobbyKeyboardJump = lastActionKey;
		if (InputManager::getSingleton()->exit())
			mLeftBlobbyKeyboardJump = oldString;
	}
	if (mLeftBlobbyJoystickLeft == "")
	{
		imgui.doOverlay(GEN_ID, Vector2(100.0, 150.0), Vector2(700.0, 450.0));
		imgui.doText(GEN_ID, Vector2(250.0, 250.0), "Press Button For");
		imgui.doText(GEN_ID, Vector2(270.0, 300.0), "Moving Left");
		mLeftBlobbyJoystickLeft = InputManager::getSingleton()->getLastJoyAction();
		if (InputManager::getSingleton()->exit())
			mLeftBlobbyJoystickLeft = oldString;
	}
	if (mLeftBlobbyJoystickRight == "")
	{
		imgui.doOverlay(GEN_ID, Vector2(100.0, 150.0), Vector2(700.0, 450.0));
		imgui.doText(GEN_ID, Vector2(250.0, 250.0), "Press Button For");
		imgui.doText(GEN_ID, Vector2(270.0, 300.0), "Moving Right");
		mLeftBlobbyJoystickRight = InputManager::getSingleton()->getLastJoyAction();
		if (InputManager::getSingleton()->exit())
			mLeftBlobbyJoystickRight = oldString;
	}
	if (mLeftBlobbyJoystickJump == "")
	{
		imgui.doOverlay(GEN_ID, Vector2(100.0, 150.0), Vector2(700.0, 450.0));
		imgui.doText(GEN_ID, Vector2(250.0, 250.0), "Press Button For");
		imgui.doText(GEN_ID, Vector2(270.0, 300.0), "Jumping");
		mLeftBlobbyJoystickJump = InputManager::getSingleton()->getLastJoyAction();
		if (InputManager::getSingleton()->exit())
			mLeftBlobbyJoystickJump = oldString;
	}
	if (mRightBlobbyMouseJumpbutton == -1)
	{
		imgui.doOverlay(GEN_ID, Vector2(100.0, 150.0), Vector2(700.0, 450.0));
		imgui.doText(GEN_ID, Vector2(180.0, 250.0), "Press Mouse Button");
		imgui.doText(GEN_ID, Vector2(290.0, 300.0), "For Jump");
		mRightBlobbyMouseJumpbutton = InputManager::getSingleton()->getLastMouseButton();
		if (InputManager::getSingleton()->exit())
			mRightBlobbyMouseJumpbutton = oldInteger;
	}
	if (mRightBlobbyKeyboardLeft == "")
	{
		imgui.doOverlay(GEN_ID, Vector2(100.0, 150.0), Vector2(700.0, 450.0));
		imgui.doText(GEN_ID, Vector2(250.0, 250.0), "Press Key For");
		imgui.doText(GEN_ID, Vector2(270.0, 300.0), "Moving Right");
		mRightBlobbyKeyboardLeft = lastActionKey;
		if (InputManager::getSingleton()->exit())
			mRightBlobbyKeyboardLeft = oldString;
	}
	if (mRightBlobbyKeyboardRight == "")
	{
		imgui.doOverlay(GEN_ID, Vector2(100.0, 150.0), Vector2(700.0, 450.0));
		imgui.doText(GEN_ID, Vector2(250.0, 250.0), "Press Key For");
		imgui.doText(GEN_ID, Vector2(270.0, 300.0), "Moving Right");
		mRightBlobbyKeyboardRight = lastActionKey;
		if (InputManager::getSingleton()->exit())
			mRightBlobbyKeyboardRight = oldString;
	}
	if (mRightBlobbyKeyboardJump == "")
	{
		imgui.doOverlay(GEN_ID, Vector2(100.0, 150.0), Vector2(700.0, 450.0));
		imgui.doText(GEN_ID, Vector2(250.0, 250.0), "Press Key For");
		imgui.doText(GEN_ID, Vector2(270.0, 300.0), "Jumping");
		mRightBlobbyKeyboardJump = lastActionKey;
		if (InputManager::getSingleton()->exit())
			mRightBlobbyKeyboardJump = oldString;
	}
	if (mRightBlobbyJoystickLeft == "")
	{
		imgui.doOverlay(GEN_ID, Vector2(100.0, 150.0), Vector2(700.0, 450.0));
		imgui.doText(GEN_ID, Vector2(250.0, 250.0), "Press Button For");
		imgui.doText(GEN_ID, Vector2(270.0, 300.0), "Moving Right");
		mRightBlobbyJoystickLeft = InputManager::getSingleton()->getLastJoyAction();
		if (InputManager::getSingleton()->exit())
			mRightBlobbyJoystickLeft = oldString;
	}
	if (mRightBlobbyJoystickRight == "")
	{
		imgui.doOverlay(GEN_ID, Vector2(100.0, 150.0), Vector2(700.0, 450.0));
		imgui.doText(GEN_ID, Vector2(250.0, 250.0), "Press Button For");
		imgui.doText(GEN_ID, Vector2(270.0, 300.0), "Moving Right");
		mRightBlobbyJoystickRight = InputManager::getSingleton()->getLastJoyAction();
		if (InputManager::getSingleton()->exit())
			mRightBlobbyJoystickRight = oldString;
	}
	if (mRightBlobbyJoystickJump == "")
	{
		imgui.doOverlay(GEN_ID, Vector2(100.0, 150.0), Vector2(700.0, 450.0));
		imgui.doText(GEN_ID, Vector2(250.0, 250.0), "Press Button For");
		imgui.doText(GEN_ID, Vector2(270.0, 300.0), "Jumping");
		mRightBlobbyJoystickJump = InputManager::getSingleton()->getLastJoyAction();
		if (InputManager::getSingleton()->exit())
			mRightBlobbyJoystickJump = oldString;
	}
	
	if (imgui.doButton(GEN_ID, Vector2(224.0, 530.0), "ok"))
	{
		mSaveConfig = true;
		delete this;
		mCurrentState = new OptionState();
	}
	if (imgui.doButton(GEN_ID, Vector2(424.0, 530.0), "cancel"))
	{
		delete this;
		mCurrentState = new OptionState();
	}
}
