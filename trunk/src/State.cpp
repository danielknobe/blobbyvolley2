#include "State.h"
#include "LocalInputSource.h"
#include "RenderManager.h"
#include "SoundManager.h"
#include "DuelMatch.h"
#include "ReplayRecorder.h"
#include "ReplayInputSource.h"
#include "ScriptedInputSource.h"
#include "IMGUI.h"


#include <physfs.h>
#include <ctime>

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
}

LocalGameState::LocalGameState(GameMode mode)
	: State()
{
	mRecorder = 0;

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
	else if (InputManager::getSingleton()->exit())
	{
		delete this;
		mCurrentState = new MainMenuState;
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

	if (imgui.doButton(GEN_ID, Vector2(500.0, 400.0), "start"))
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
	
	if (imgui.doButton(GEN_ID, Vector2(500.0, 430.0), "options"))
	{
		delete mCurrentState;
		mCurrentState = new OptionState();
	}

	if (imgui.doButton(GEN_ID, Vector2(500.0, 460.0), "watch replay"))
	{
		delete mCurrentState;
		mCurrentState = new ReplayMenuState();
	}
	
	if (imgui.doButton(GEN_ID, Vector2(500.0, 490.0), "exit")) 
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
	IMGUI::getSingleton().doText(GEN_ID, Vector2(250, 250), buf);
	IMGUI::getSingleton().doText(GEN_ID, Vector2(250, 300), "hat gewonnen!");
	
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
	imgui.doText(GEN_ID, Vector2(10.0, 10.0), "left player");
	imgui.doText(GEN_ID, Vector2(410.0, 10.0), "right player");

	imgui.doImage(GEN_ID, Vector2(12.0 + 4.0,
		mPlayerOptions[LEFT_PLAYER] * 28.0 + 12.0 + 50.0), "gfx/pfeil_rechts.bmp");
	imgui.doImage(GEN_ID, Vector2(12.0 + 404.0,
		mPlayerOptions[RIGHT_PLAYER] * 28.0 + 12.0 + 50.0), "gfx/pfeil_rechts.bmp");

	if (imgui.doButton(GEN_ID, Vector2(10.0, 50.0), "human"))
		mPlayerOptions[LEFT_PLAYER] = 0;
	for (int i = 1; i < mScriptNames.size(); i++)
	{
		if (imgui.doButton(GEN_ID << 4 + i,
				Vector2(10.0, 50.0 + i * 28.0), mScriptNames[i]))
			mPlayerOptions[LEFT_PLAYER] = i;
	}
	
	if (imgui.doButton(GEN_ID, Vector2(410.0, 50.0), "human"))
		mPlayerOptions[RIGHT_PLAYER] = 0;
	for (int i = 1; i < mScriptNames.size(); i++)
	{
		if (imgui.doButton(GEN_ID << 5 + i,
					Vector2(410.0, 50.0 + i * 28.0), mScriptNames[i]))
			mPlayerOptions[RIGHT_PLAYER] = i;
	}

	if (mReplayActivated)
	{
		imgui.doImage(GEN_ID, Vector2(108.0, 472.0), "gfx/pfeil_rechts.bmp");
	}
	if (imgui.doButton(GEN_ID, Vector2(100.0, 460.0), "record replays"))
	{
		mReplayActivated = !mReplayActivated;
	}

	if (imgui.doButton(GEN_ID, Vector2(200.0, 530.0), "ok"))
	{
		mSaveConfig = true;
		delete this;
		mCurrentState = new MainMenuState();
	}
	if (imgui.doButton(GEN_ID, Vector2(400.0, 530.0), "cancel"))
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
	snprintf(buf, 256, "replays/replay_%d.xml", time(0));
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

		if (imgui.doButton(GEN_ID, Vector2(200.0, 10.0), "play") && 
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
		else if (imgui.doButton(GEN_ID, Vector2(400.0, 10.0), "cancel"))
		{
			delete this;
			mCurrentState = new MainMenuState();
		}
		else for (int i = 0; i < mReplayFiles.size(); i++)
		{
			if (imgui.doButton(GEN_ID << 6 + i, Vector2(10.0, 50.0 + 30.0 * i),
						mReplayFiles[i]))
			{
				mSelectedReplay = i;
				break;
			}
		}
		if (imgui.doButton(GEN_ID, Vector2(620.0, 60.0), "delete") &&
					mSelectedReplay != -1)
		{
			PHYSFS_delete(std::string("replays/" +
				mReplayFiles[mSelectedReplay]).c_str());
			delete this;
			mCurrentState = new ReplayMenuState();
		}


	}
}
