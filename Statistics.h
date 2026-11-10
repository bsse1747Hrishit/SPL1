#pragma once
// ============================================================================
//  Statistics - dynamic player rankings and season analytics.
//   * Full rankings are produced with merge sort (stable, O(n log n)).
//   * Top-N leaderboards use a bounded min-heap: O(n log N).
//   * Country (association) aggregates are built in a hash table.
//   * Descriptive statistics: mean, standard deviation, median, histogram.
// ============================================================================
#include <string>
#include "DynArray.h"
#include "Player.h"

class Season;

namespace stats {

enum class RankBy { GOALS = 0, ASSISTS, CONTRIBUTIONS, CLEAN_SHEETS, MINUTES, DISCIPLINE, IMPACT, GOALS_PER90 };
const char* rankByName(RankBy r);
const int RANK_KINDS = 8;

// Impact score: a simple composite "player of the season" index
double impactScore(const Player& p);
double metric(RankBy r, const Player& p);
// a ranks above b?
bool ranksAbove(RankBy r, const Player& a, const Player& b);

void rankAll(const Season& s, RankBy r, ds::DynArray<const Player*>& out);   // merge sort
void topN(const Season& s, RankBy r, int n, ds::DynArray<const Player*>& out);  // heap

struct Summary {
    int matches = 0, goals = 0;
    double meanGoals = 0, stdGoals = 0, medianGoals = 0;
    int homeWins = 0, draws = 0, awayWins = 0;
    int yellows = 0, reds = 0, penalties = 0, ownGoals = 0, injuries = 0, subs = 0;
    int extraTimes = 0, shootouts = 0;
    int histogram[11] = {0};        // goals per match 0..9, 10 = 10+
    int goalsByPeriod[8] = {0};     // 1-15, 16-30, 31-45, 46-60, 61-75, 76-90, 91-105, 106-120
    int biggestWin = -1, highestScoring = -1;   // match ids
};
Summary summarize(const Season& s);

struct CountryStat {
    std::string country;
    int teams = 0, played = 0, won = 0, drawn = 0, lost = 0, gf = 0, ga = 0;
    double ppg() const { return played ? (3.0 * won + drawn) / played : 0.0; }
};
void countryStats(const Season& s, ds::DynArray<CountryStat>& out);

struct TeamStat {
    int team = -1;
    int played = 0, won = 0, drawn = 0, lost = 0, gf = 0, ga = 0, cleanSheets = 0;
    int yellows = 0, reds = 0;
    double eloChange = 0;
};
TeamStat teamStat(const Season& s, int team);

double mean(const ds::DynArray<double>& v);
double stddev(const ds::DynArray<double>& v);
double median(ds::DynArray<double> v);

}  // namespace stats
