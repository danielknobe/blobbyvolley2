#include "MatchSeries.h"

#include <cstdlib>

MatchFormat matchFormatForRules(const std::string& rulesFile)
{
	return rulesFile == "modern_volley.lua" ? MatchFormat::MODERN_VOLLEY : MatchFormat::STANDARD;
}

MatchSeries::MatchSeries(MatchFormat format, bool enabled)
	: mFormat(format), mEnabled(enabled), mCurrentSet(1), mSetsWon{0, 0}
{
}

bool MatchSeries::isSeries() const
{
	return mEnabled;
}

int MatchSeries::currentSet() const
{
	return mCurrentSet;
}

int MatchSeries::scoreToWin() const
{
	if (mFormat == MatchFormat::MODERN_VOLLEY)
		return mCurrentSet == 5 ? 15 : 25;
	return 15;
}

PlayerSide MatchSeries::firstServer() const
{
	const PlayerSide player = mCurrentSet % 2 == 1 ? LEFT_PLAYER : RIGHT_PLAYER;
	return playerOnSide(player);
}

bool MatchSeries::sidesSwapped() const
{
	return isSeries() && mCurrentSet % 2 == 0;
}

PlayerSide MatchSeries::playerOnSide(PlayerSide side) const
{
	if (side == NO_PLAYER || !sidesSwapped())
		return side;
	return side == LEFT_PLAYER ? RIGHT_PLAYER : LEFT_PLAYER;
}

bool MatchSeries::isSetWon(int leftScore, int rightScore) const
{
	const int target = scoreToWin();
	return (leftScore >= target || rightScore >= target) && std::abs(leftScore - rightScore) >= 2;
}

void MatchSeries::finishSet(PlayerSide winner, int leftScore, int rightScore)
{
	if (!isSeries() || matchWinner() != NO_PLAYER || winner == NO_PLAYER || !isSetWon(leftScore, rightScore))
		return;

	++mSetsWon[winner];
	if (matchWinner() == NO_PLAYER)
		++mCurrentSet;
}

int MatchSeries::setsWon(PlayerSide player) const
{
	if (player == LEFT_PLAYER)
		return mSetsWon[0];
	if (player == RIGHT_PLAYER)
		return mSetsWon[1];
	return 0;
}

PlayerSide MatchSeries::matchWinner() const
{
	if (mSetsWon[0] >= 3)
		return LEFT_PLAYER;
	if (mSetsWon[1] >= 3)
		return RIGHT_PLAYER;
	return NO_PLAYER;
}
