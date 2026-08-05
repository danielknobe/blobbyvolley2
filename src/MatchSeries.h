#pragma once

#include <string>

#include "Global.h"

enum class MatchFormat
{
	STANDARD,
	MODERN_VOLLEY
};

MatchFormat matchFormatForRules(const std::string& rulesFile);

class MatchSeries
{
public:
	explicit MatchSeries(MatchFormat format = MatchFormat::STANDARD, bool enabled = false);

	bool isSeries() const;
	int currentSet() const;
	int scoreToWin() const;
	bool sidesSwapped() const;
	PlayerSide playerOnSide(PlayerSide side) const;
	PlayerSide firstServer() const;

	bool isSetWon(int leftScore, int rightScore) const;
	void finishSet(PlayerSide winner, int leftScore, int rightScore);
	int setsWon(PlayerSide player) const;
	PlayerSide matchWinner() const;

private:
	MatchFormat mFormat;
	bool mEnabled;
	int mCurrentSet;
	int mSetsWon[MAX_PLAYERS];
};
