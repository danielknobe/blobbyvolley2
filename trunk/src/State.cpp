#include "State.h"
#include "LocalInputSource.h"
#include "DuelMatch.h"
#include "ReplayRecorder.h"
#include "ReplayInputSource.h"
#include "ScriptedInputSource.h"
#include "SoundManager.h"
#include "IMGUI.h"
#include "NetworkState.h"
#include "OptionsState.h"

#include <physfs.h>
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
	delete mRecorder;
	InputManager::getSingleton()->endGame();
}

LocalGameState::LocalGameState()
	: State()
{
	mRecorder = 0;
	mPaused = false;
	mSaveReplay = false;
	mWinner = false;
	std::stringstream temp;
	temp << time(0);
	mFilename = temp.str();

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

	SoundManager::getSingleton().playSound("sounds/pfiff.wav", ROUND_START_SOUND_VOLUME);
	
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

	mRecorder = new ReplayRecorder(MODE_RECORDING_DUEL);
	mLeftInput = mRecorder->createReplayInputSource(LEFT_PLAYER,
			linput);
	mRightInput = mRecorder->createReplayInputSource(RIGHT_PLAYER,
			rinput);

	mMatch = new DuelMatch(mLeftInput, mRightInput, true, true);
	IMGUI::getSingleton().resetSelection();
}

void LocalGameState::step()
{
	RenderManager* rmanager = &RenderManager::getSingleton();
	
	IMGUI& imgui = IMGUI::getSingleton();
	if (mSaveReplay)
	{
		imgui.doOverlay(GEN_ID, Vector2(150, 200), Vector2(650, 400));
		imgui.doText(GEN_ID, Vector2(190, 220), "Name of the Replay:");
		static unsigned cpos;
		imgui.doEditbox(GEN_ID, Vector2(180, 270), 18, mFilename, cpos);
		if (imgui.doButton(GEN_ID, Vector2(220, 330), "OK"))
		{
			if (mFilename != "")
			{
				mRecorder->save(std::string("replays/") + mFilename + std::string(".xml"));
			}
			mSaveReplay = false;
			imgui.resetSelection();
		}
		if (imgui.doButton(GEN_ID, Vector2(440, 330), "Cancel"))
		{
			mSaveReplay = false;
			imgui.resetSelection();
		}
		imgui.doCursor();
	}
	else if (mPaused)
	{
		imgui.doOverlay(GEN_ID, Vector2(180, 200), Vector2(670, 400));
		imgui.doText(GEN_ID, Vector2(234, 260), "Wirklich Beenden?");
		if (imgui.doButton(GEN_ID, Vector2(480, 300), "Nein"))
			mPaused = false;
		if (imgui.doButton(GEN_ID, Vector2(260, 300), "Ja"))
		{
			delete this;
			mCurrentState = new MainMenuState;
		}
		if (imgui.doButton(GEN_ID, Vector2(293, 340), "Save Replay"))
		{
			mSaveReplay = true;
			imgui.resetSelection();
		}
		imgui.doCursor();
	}
	else if (mWinner)
	{
		std::stringstream tmp;
		tmp << "player " << mMatch->winningPlayer() + 1;
		imgui.doOverlay(GEN_ID, Vector2(200, 150), Vector2(700, 450));
		imgui.doImage(GEN_ID, Vector2(200, 250), "gfx/pokal.bmp");
		imgui.doText(GEN_ID, Vector2(274, 250), tmp.str());
		imgui.doText(GEN_ID, Vector2(274, 300), "has won the game!");
		if (imgui.doButton(GEN_ID, Vector2(290, 350), "ok"))
		{
			delete mCurrentState;
			mCurrentState = new MainMenuState();
		}
		if (imgui.doButton(GEN_ID, Vector2(400, 350), "try again"))
		{
			delete mCurrentState;
			mCurrentState = new LocalGameState();
		}
		if (imgui.doButton(GEN_ID, Vector2(260, 390), "save replay"))
		{
			mSaveReplay = true;
			imgui.resetSelection();
		}
		imgui.doCursor();
	}
	else if (InputManager::getSingleton()->exit())
	{
		if (mSaveReplay)
		{
			mSaveReplay = false;
			IMGUI::getSingleton().resetSelection();
		}
		else if (mPaused)
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
	else
	{
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
		if (mMatch->winningPlayer() != NO_PLAYER)
			mWinner = true;
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
	imgui.doImage(GEN_ID, Vector2(400.0, 300.0), "background");
	imgui.doOverlay(GEN_ID, Vector2(0.0, 0.0), Vector2(800.0, 600.0));
	imgui.doImage(GEN_ID, Vector2(250.0, 210.0), "gfx/titel.bmp");
	if (imgui.doButton(GEN_ID, Vector2(484, 370.0), "network game"))
	{
		delete mCurrentState;
		mCurrentState = new NetworkSearchState();
	}
	if (imgui.doButton(GEN_ID, Vector2(484.0, 400.0), "start"))
	{
		delete mCurrentState;
		try
		{
			mCurrentState = new LocalGameState();
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

ReplayMenuState::ReplayMenuState()
{
	IMGUI::getSingleton().resetSelection();
	mReplaying = false;
	mReplayMatch = 0;
	mReplayRecorder = 0;
	mSelectedReplay = 0;
	char** filenames = PHYSFS_enumerateFiles("replays");
	for (int i = 0; filenames[i] != 0; ++i)
	{
		std::string tmp(filenames[i]);
		if (tmp.find(".xml") != std::string::npos)
		{
			mReplayFiles.push_back(std::string(tmp.begin(), tmp.end()-4));
		}
	}
	if (mReplayFiles.size() == 0)
		mSelectedReplay = -1;
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

void ReplayMenuState::loadCurrentReplay()
{
	mReplayRecorder = new ReplayRecorder(MODE_REPLAY_DUEL);
	mReplayRecorder->load(std::string("replays/" + mReplayFiles[mSelectedReplay] + ".xml"));
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
			"sounds/pfiff.wav", ROUND_START_SOUND_VOLUME);
}

void ReplayMenuState::step()
{
	IMGUI& imgui = IMGUI::getSingleton();
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
			std::stringstream tmp;
			tmp << "Spieler " << side+1;
			imgui.doOverlay(GEN_ID, Vector2(200, 150), Vector2(650, 450));
			imgui.doImage(GEN_ID, Vector2(200, 250), "gfx/pokal.bmp");
			imgui.doText(GEN_ID, Vector2(274, 250), tmp.str());
			imgui.doText(GEN_ID, Vector2(274, 300), "hat gewonnen!");
			if (imgui.doButton(GEN_ID, Vector2(290, 350), "OK"))
			{
				mReplaying = false;
				delete mReplayMatch;
				delete mReplayRecorder; 
				imgui.resetSelection();
			}
			if (imgui.doButton(GEN_ID, Vector2(400, 350), "NOCHMAL"))
			{
				delete mReplayMatch;
				delete mReplayRecorder; 
				loadCurrentReplay();
				imgui.resetSelection();
			}
			imgui.doCursor();
		}
		else if ((InputManager::getSingleton()->exit()) || (mReplayRecorder->endOfFile()))
		{
			mReplaying = false;
			delete mReplayMatch;
			delete mReplayRecorder; 
			imgui.resetSelection();
		}
	}
	else
	{
		imgui.doCursor();
		imgui.doImage(GEN_ID, Vector2(400.0, 300.0), "background");
		imgui.doOverlay(GEN_ID, Vector2(0.0, 0.0), Vector2(800.0, 600.0));

		if (imgui.doButton(GEN_ID, Vector2(224.0, 10.0), "play") && 
					mSelectedReplay != -1)
		{
			loadCurrentReplay();
			imgui.resetSelection();
		}
		else if (imgui.doButton(GEN_ID, Vector2(424.0, 10.0), "cancel"))
		{
			delete this;
			mCurrentState = new MainMenuState();
		}
		else 
			imgui.doSelectbox(GEN_ID, Vector2(34.0, 50.0), Vector2(634.0, 550.0), mReplayFiles, mSelectedReplay);
		if (imgui.doButton(GEN_ID, Vector2(644.0, 60.0), "delete"))
		{
			if (!mReplayFiles.empty())
			if (PHYSFS_delete(std::string("replays/" + mReplayFiles[mSelectedReplay] + ".xml").c_str()))
			{
				mReplayFiles.erase(mReplayFiles.begin()+mSelectedReplay);
				if (mSelectedReplay >= mReplayFiles.size())
					mSelectedReplay = mReplayFiles.size()-1;
			}
		}
	}
}
