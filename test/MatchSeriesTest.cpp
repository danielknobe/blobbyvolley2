#include <boost/test/unit_test.hpp>

#include "MatchSeries.h"

BOOST_AUTO_TEST_SUITE(match_series_test)

BOOST_AUTO_TEST_CASE(modern_set_requires_two_point_lead)
{
	MatchSeries series(MatchFormat::MODERN_VOLLEY, true);
	BOOST_CHECK(series.isSetWon(25, 23));
	BOOST_CHECK(!series.isSetWon(25, 24));
	BOOST_CHECK(series.isSetWon(26, 24));
}

BOOST_AUTO_TEST_CASE(modern_fifth_set_uses_fifteen_points)
{
	MatchSeries series(MatchFormat::MODERN_VOLLEY, true);
	series.finishSet(LEFT_PLAYER, 25, 20);
	series.finishSet(RIGHT_PLAYER, 20, 25);
	series.finishSet(LEFT_PLAYER, 25, 21);
	series.finishSet(RIGHT_PLAYER, 23, 25);
	BOOST_CHECK_EQUAL(series.currentSet(), 5);
	BOOST_CHECK_EQUAL(series.scoreToWin(), 15);
	BOOST_CHECK(!series.isSetWon(15, 14));
	BOOST_CHECK(series.isSetWon(16, 14));
}

BOOST_AUTO_TEST_CASE(series_supports_three_zero_three_one_and_three_two)
{
	MatchSeries straight(MatchFormat::MODERN_VOLLEY, true);
	for (int i = 0; i < 3; ++i)
		straight.finishSet(LEFT_PLAYER, 25, 20);
	BOOST_CHECK_EQUAL(straight.matchWinner(), LEFT_PLAYER);

	MatchSeries fourSets(MatchFormat::MODERN_VOLLEY, true);
	fourSets.finishSet(RIGHT_PLAYER, 20, 25);
	for (int i = 0; i < 3; ++i)
		fourSets.finishSet(LEFT_PLAYER, 25, 20);
	BOOST_CHECK_EQUAL(fourSets.matchWinner(), LEFT_PLAYER);

	MatchSeries fiveSets(MatchFormat::MODERN_VOLLEY, true);
	fiveSets.finishSet(LEFT_PLAYER, 25, 20);
	fiveSets.finishSet(RIGHT_PLAYER, 20, 25);
	fiveSets.finishSet(LEFT_PLAYER, 25, 20);
	fiveSets.finishSet(RIGHT_PLAYER, 20, 25);
	fiveSets.finishSet(LEFT_PLAYER, 15, 10);
	BOOST_CHECK_EQUAL(fiveSets.matchWinner(), LEFT_PLAYER);
}

BOOST_AUTO_TEST_CASE(standard_series_stays_at_fifteen)
{
	MatchSeries series(MatchFormat::STANDARD, true);
	BOOST_CHECK_EQUAL(series.scoreToWin(), 15);
	series.finishSet(RIGHT_PLAYER, 13, 15);
	BOOST_CHECK_EQUAL(series.scoreToWin(), 15);
}

BOOST_AUTO_TEST_CASE(rule_files_select_the_series_format)
{
	BOOST_CHECK(matchFormatForRules("default.lua") == MatchFormat::STANDARD);
	BOOST_CHECK(matchFormatForRules("classic.lua") == MatchFormat::STANDARD);
	BOOST_CHECK(matchFormatForRules("modern_volley.lua") == MatchFormat::MODERN_VOLLEY);
}

BOOST_AUTO_TEST_CASE(sets_can_be_disabled_independently_from_rules)
{
	MatchSeries modern(MatchFormat::MODERN_VOLLEY, false);
	BOOST_CHECK(!modern.isSeries());
}

BOOST_AUTO_TEST_CASE(players_switch_courts_between_sets)
{
	MatchSeries series(MatchFormat::MODERN_VOLLEY, true);
	BOOST_CHECK(!series.sidesSwapped());
	BOOST_CHECK_EQUAL(series.playerOnSide(LEFT_PLAYER), LEFT_PLAYER);
	BOOST_CHECK_EQUAL(series.firstServer(), LEFT_PLAYER);

	series.finishSet(LEFT_PLAYER, 25, 20);
	BOOST_CHECK(series.sidesSwapped());
	BOOST_CHECK_EQUAL(series.playerOnSide(LEFT_PLAYER), RIGHT_PLAYER);
	BOOST_CHECK_EQUAL(series.playerOnSide(RIGHT_PLAYER), LEFT_PLAYER);
	BOOST_CHECK_EQUAL(series.firstServer(), LEFT_PLAYER);
	BOOST_CHECK_EQUAL(series.playerOnSide(series.firstServer()), RIGHT_PLAYER);

	series.finishSet(RIGHT_PLAYER, 25, 20);
	BOOST_CHECK(!series.sidesSwapped());
	BOOST_CHECK_EQUAL(series.playerOnSide(LEFT_PLAYER), LEFT_PLAYER);
}

BOOST_AUTO_TEST_SUITE_END()
