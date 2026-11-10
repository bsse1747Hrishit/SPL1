#pragma once
// ============================================================================
//  Reports - every view of the data is written to a std::ostream, so the
//  same code prints to the console AND exports text-file reports.
// ============================================================================
#include <iostream>
#include <string>
#include "MonteCarlo.h"
#include "Statistics.h"

class Season;
struct Match;
struct MatchEvent;
struct Player;

namespace reports {

std::string scoreLine(const Season& s, const Match& m);          // "A 2-1 B (aet)"
std::string eventText(const Season& s, const Match& m, const MatchEvent& e);
void matchLine(std::ostream& os, const Season& s, const Match& m);

void table(std::ostream& os, const Season& s, int highlightA = -1, int highlightB = -1);
void fixtures(std::ostream& os, const Season& s, int matchday);   // 0 = all league, -1 = knockout
void teamFixtures(std::ostream& os, const Season& s, int team);
void drawSummary(std::ostream& os, const Season& s);
void matchReport(std::ostream& os, const Season& s, const Match& m);
void squad(std::ostream& os, const Season& s, int team);
void teamReport(std::ostream& os, const Season& s, int team);
void availability(std::ostream& os, const Season& s);
void playerRanking(std::ostream& os, const Season& s, stats::RankBy r, int n);
void playerProfile(std::ostream& os, const Season& s, const Player& p);
void seasonStats(std::ostream& os, const Season& s);
void countryTable(std::ostream& os, const Season& s);
void eloTable(std::ostream& os, const Season& s);
void bracketTree(std::ostream& os, const Season& s);
void bracketRounds(std::ostream& os, const Season& s);
void prediction(std::ostream& os, const Season& s, int home, int away, bool neutral);
void monteCarlo(std::ostream& os, const Season& s, const MCResult& r, const MCResult* before);
void dataStructures(std::ostream& os, const Season& s);
void fullSeasonReport(std::ostream& os, const Season& s);

}  // namespace reports
