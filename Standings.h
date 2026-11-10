#pragma once
// ============================================================================
//  Standings - the league-phase table.
//  Tiebreakers (in order): points, goal difference, goals scored,
//  head-to-head (mini-table between the tied teams: points, GD, goals),
//  then alphabetical order. Sorting uses our stable merge sort; head-to-head
//  records live in a hash table keyed by the (team, team) pair.
// ============================================================================
#include "DynArray.h"
#include "HashMap.h"
#include "Team.h"

struct TableRow {
    int team = -1;
    int played = 0, won = 0, drawn = 0, lost = 0;
    int gf = 0, ga = 0, pts = 0;
    char form[6] = {0, 0, 0, 0, 0, 0};    // last 5 results, newest last
    int gd() const { return gf - ga; }
    void pushForm(char c);
};

struct HeadToHead {
    int goalsLow = 0;    // goals by the team with the lower id
    int goalsHigh = 0;
    int meetings = 0;
};

class Standings {
public:
    explicit Standings(int teams = 36) { reset(teams); }
    void reset(int teams);
    void record(int home, int away, int hg, int ag);

    const TableRow& row(int team) const { return rows_[team]; }
    int size() const { return rows_.size(); }

    // Team ids ordered 1st..last with all tiebreakers applied.
    ds::DynArray<int> ranking(const ds::DynArray<Team>& teams) const;

    // h2h goals scored by `a` against `b` (and meetings count).
    bool headToHead(int a, int b, int& goalsA, int& goalsB) const;

    // Zone label for a 0-based league position.
    static const char* zone(int position);

private:
    static long long pairKey(int a, int b) { return a < b ? a * 1000LL + b : b * 1000LL + a; }
    void resolveTie(ds::DynArray<int>& ids, int from, int to, const ds::DynArray<Team>& teams) const;

    ds::DynArray<TableRow> rows_;
    ds::HashMap<long long, HeadToHead> h2h_;
};
