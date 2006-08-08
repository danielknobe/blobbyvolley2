#include "State.h"
#include "LocalInputSource.h"
#include "RenderManager.h"
#include "SoundManager.h"
#include "GUIManager.h"
#include "DuelMatch.h"
#include "ReplayRecorder.h"
#include "ReplayInputSource.h"
#include "ScriptedInputSource.h"

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

	GUIManager::getSingleton()->drawCursor(false);
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
	GUIManager* gmgr = GUIManager::getSingleton();
	gmgr->clear();
	gmgr->createOverlay(0.5, Vector2(0.0, 0.0), Vector2(800.0, 600.0));
	gmgr->createImage("gfx/titel.bmp", Vector2(250.0, 210.0));
	gmgr->drawCursor(true);
	mStartButton = gmgr->createTextButton("start",
					5, Vector2(500.0, 400.0));
	mOptionButton = 
		gmgr->createTextButton("options", 7, Vector2(500.0, 430.0));
	mWatchReplayButton = 
		gmgr->createTextButton("watch replay", 12, Vector2(500.0, 460.0));
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
	
	else if (gmgr->getClick(mOptionButton))
	{
		delete mCurrentState;
		mCurrentState = new OptionState();
	}

	else if (gmgr->getClick(mWatchReplayButton))
	{
		delete mCurrentState;
		 mCurrentState = new ReplayMenuState();
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

OptionState::OptionState()
{
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
	rebuildGUI();
}

OptionState::~OptionState()
{
	GUIManager::getSingleton()->clear();
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

void OptionState::rebuildGUI()
{
	GUIManager* guimgr = GUIManager::getSingleton();
	guimgr->clear();
	guimgr->createOverlay(0.5, Vector2(0.0, 0.0), Vector2(800.0, 600.0));
	guimgr->createText("left player", Vector2(10.0, 10.0));
	guimgr->createText("right player", Vector2(410.0, 10.0));
	guimgr->createImage("gfx/pfeil_rechts.bmp", Vector2(12.0 + 4.0,
		mPlayerOptions[LEFT_PLAYER] * 28.0 + 12.0 + 50.0));
	guimgr->createImage("gfx/pfeil_rechts.bmp", Vector2(12.0 + 404.0,
		mPlayerOptions[RIGHT_PLAYER] * 28.0 + 12.0 + 50.0));

	mLeftPlayerButtons.push_back(
		guimgr->createTextButton("human", 5, Vector2(10.0, 50.0)));
	for (int i = 1; i < mScriptNames.size(); i++)
	{
		mLeftPlayerButtons.push_back(guimgr->createTextButton(
			mScriptNames[i], mScriptNames[i].length(),
			Vector2(10.0, 50.0 + i * 28.0)));
	}
	
	mRightPlayerButtons.push_back(
		guimgr->createTextButton("human", 5, Vector2(410.0, 50.0)));
	for (int i = 1; i < mScriptNames.size(); i++)
	{
		mRightPlayerButtons.push_back(guimgr->createTextButton(
			mScriptNames[i], mScriptNames[i].length(),
			Vector2(410.0, 50.0 + i * 28.0)));
	}

	if (mReplayActivated)
	{
		guimgr->createImage("gfx/pfeil_rechts.bmp",
						Vector2(108.0, 472.0));
	}
	mReplayButton = guimgr->createTextButton("record replays", 14,
						Vector2(100.0, 460.0));
	mOkButton = guimgr->createTextButton("ok", 2, Vector2(200.0, 530.0));
	mCancelButton = guimgr->createTextButton("cancel", 6, 
					Vector2(400.0, 530.0));
}

void OptionState::step()
{
	GUIManager* guimgr = GUIManager::getSingleton();
	for (int i = 0; i < mLeftPlayerButtons.size(); i++)
	{
		if (guimgr->getClick(mLeftPlayerButtons[i]))
		{
			mPlayerOptions[LEFT_PLAYER] = i;
			rebuildGUI();
			break;
		}
	}
	
	for (int i = 0; i < mRightPlayerButtons.size(); i++)
	{
		if (guimgr->getClick(mRightPlayerButtons[i]))
		{
			mPlayerOptions[RIGHT_PLAYER] = i;
			rebuildGUI();
			break;
		}
	}
	
	if (guimgr->getClick(mReplayButton))
	{
		mReplayActivated = !mReplayActivated;
		rebuildGUI();
	}
	
	if (guimgr->getClick(mOkButton))
	{
		mSaveConfig = true;
		delete this;
		mCurrentState = new MainMenuState();
	}
	else if (guimgr->getClick(mCancelButton))
	{
		delete this;
		mCurrentState = new MainMenuState();
	}
}

ReplayMenuState::ReplayMenuState()
{
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

	rebuildGUI();
	
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
	GUIManager::getSingleton()->clear();
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

void ReplayMenuState::rebuildGUI()
{
	GUIManager* guimgr = GUIManager::getSingleton();
	guimgr->drawCursor(true);
	guimgr->clear();
	guimgr->createOverlay(0.5, Vector2(0.0, 0.0), Vector2(800.0, 600.0));
	mPlayButton = guimgr->createTextButton("play", 4,
					Vector2(200.0, 10.0));
	mCancelButton = guimgr->createTextButton("cancel", 6,
					Vector2(400.0, 10.0));
	mDeleteButton = guimgr->createTextButton("delete", 6,
					Vector2(620.0, 60.0));
	for (int i = 0; i < mReplayFiles.size(); i++)
	{
		mReplayButtons.push_back(guimgr->createTextButton(
				mReplayFiles[i], mReplayFiles[i].length(),
				Vector2(10.0, 50.0 + 30.0 * i)));
	}
	
	if (mSelectedReplay != -1)
	{
		guimgr->createImage("gfx/pfeil_rechts.bmp",
			Vector2(16.0, 62.0 + 30.0 * mSelectedReplay));
	}
}

void ReplayMenuState::step()
{
	GUIManager* guimgr = GUIManager::getSingleton();
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
		for (int i = 0; i < mReplayButtons.size(); i++)
		{
			if (guimgr->getClick(mReplayButtons[i]))
			{
				mSelectedReplay = i;
				rebuildGUI();
				break;
			}
		}
		if (guimgr->getClick(mDeleteButton) &&
					mSelectedReplay != -1)
		{
			PHYSFS_delete(std::string("replays/" +
				mReplayFiles[mSelectedReplay]).c_str());
			delete this;
			mCurrentState = new ReplayMenuState();
		}
		else if (guimgr->getClick(mCancelButton))
		{
			delete this;
			mCurrentState = new MainMenuState();
		}
		else if (guimgr->getClick(mPlayButton) && 
					mSelectedReplay != -1)
		{
			guimgr->clear();
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
			guimgr->drawCursor(false);
		}
	}
}
