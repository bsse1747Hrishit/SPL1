#pragma once
// ============================================================================
//  Season - owns the whole tournament state and its flow:
//    League phase (8 matchdays) -> Knockout play-offs -> Round of 16 ->
//    Quarter-finals -> Semi-finals -> Final -> Complete
//
//  Determinism: the draw uses Random(seed) and the knockout draw uses
//  Random(seed * K), independent of match simulation. So a save file only
//  needs the seed + the event log of every played match; loading replays the
//  log through the same rule engine and rebuilds identical state.
// ============================================================================
#include <string>
#include "DynArray.h"
#include "HashMap.h"
#include "Queue.h"
#include "Random.h"
#include "Bracket.h"
#include "Standings.h"
#include "Match.h"
#include "MatchRecorder.h"
#include "MatchSimulator.h"
#include "Team.h"

enum class Phase { LEAGUE = 0, PLAYOFF = 1, R16 = 2, QF = 3, SF = 4, FINAL = 5, COMPLETE = 6 };
const char* phaseName(Phase p);

struct Settings {
    int tableMode = 1;        // 0 = never, 1 = after every match, 2 = after every matchday
    int mcIterations = 2000;  // Monte-Carlo simulations
};

class Season {
public:
    Season() = default;
    ~Season();
    Season(const Season&) = delete;
    Season& operator=(const Season&) = delete;

    bool create(unsigned long long seed, std::string& log);

    // ---- state ------------------------------------------------------------
    ds::DynArray<Team> teams;
    ds::HashMap<std::string, int> teamIndex;     // CODE / lower-case name -> team id
    ds::HashMap<std::string, int> playerIndex;   // lower-case name -> global player id
    ds::DynArray<Match*> matches;                // all matches by id
    ds::DynArray<int> playOrder;                 // ids in the order played
    ds::Queue<int> pending;                      // fixture queue: next match first
    Standings table;
    Bracket bracket;
    Phase phase = Phase::LEAGUE;
    ds::Random rng;                              // match simulation RNG
    unsigned long long seed = 0;
    Settings settings;
    int leagueRanking[36];
    bool leagueDone = false;
    ds::DynArray<std::string> news;              // phase transitions, champions
    ds::DynArray<std::string> playerNews;        // bans, injuries

    // ---- access -------------------------------------------------------------
    Team& team(int id) { return teams[id]; }
    const Team& team(int id) const { return teams[id]; }
    Player& player(int gid) { return teams[gid / 100].squad[gid % 100]; }
    const Player& player(int gid) const { return teams[gid / 100].squad[gid % 100]; }
    Match& match(int id) { return *matches[id]; }
    const Match& match(int id) const { return *matches[id]; }
    int findTeam(const std::string& text) const;          // code or name
    void findPlayers(const std::string& text, ds::DynArray<int>& out) const;  // substring
    Match* nextMatch();
    int currentMatchday() const;
    int playedCount() const { return playOrder.size(); }
    int champion() const;
    ds::DynArray<int> currentRanking() const { return table.ranking(teams); }

    // ---- match flow -----------------------------------------------------
    // True when a knockout decider is level at this point (ET / penalties needed).
    bool needsDecider(const MatchRecorder& rec) const;
    // Completes a match from whatever state it is in (normal time, ET, pens).
    void autoComplete(MatchRecorder& rec, MatchSimulator& sim);
    // Emergency registrations (call before choosing line-ups).
    void prepareMatch(Match& m);
    bool playAuto(Match& m);                              // fully simulated
    // Applies a finished match to stats, table, Elo, discipline, bracket.
    void commitMatch(Match& m, MatchRecorder& rec);

    // Aggregate goals of `team` in a knockout tie (counting played legs).
    int tieGoals(int tieNode, int team) const;
    std::string tieSummary(int tieNode) const;

private:
    int createMatch(Stage s, int leg, int md, int home, int away, bool neutral, int tieNode);
    void startKnockoutRound(Phase p);
    void advance();
    void resolveTie(Match& m);
    void resetYellowCards();
    void rebuildIndexes();
};
