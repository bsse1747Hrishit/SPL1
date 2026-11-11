#pragma once
// ============================================================================
//  Match data: stage, teams, result, line-ups and the full event log
//  (goals, cards, substitutions, injuries) plus any penalty shoot-out.
// ============================================================================
#include <string>
#include "LinkedList.h"

enum class Stage { LEAGUE = 0, PLAYOFF = 1, R16 = 2, QF = 3, SF = 4, FINAL = 5 };
const char* stageName(Stage s);
const char* stageShort(Stage s);

enum class EventType {
    GOAL = 0,       // player = scorer, player2 = assist (-1 = none)
    PEN_GOAL,       // penalty scored
    OWN_GOAL,       // side = team of the unlucky player, goal counts for the other side
    PEN_MISS,       // penalty missed / saved
    YELLOW,
    SECOND_YELLOW,  // second booking -> sent off
    RED,            // straight red, extra = ban length
    SUB,            // player = off, player2 = on
    INJURY          // extra = matches out
};
const char* eventTag(EventType t);

struct MatchEvent {
    EventType type = EventType::GOAL;
    int minute = 0;
    int side = 0;       // 0 = home, 1 = away
    int player = -1;    // squad index within that side's team
    int player2 = -1;
    int extra = 0;
};

struct PenaltyKick {
    int side = 0;
    int player = -1;
    bool scored = false;
};

const int XI = 11;
const int MAX_BENCH = 12;

struct Lineup {
    int starters[XI];
    int bench[MAX_BENCH];
    int benchCount = 0;
    Lineup() {
        for (int& s : starters) s = -1;
        for (int& b : bench) b = -1;
    }
};

enum class PlayMode { NONE = 0, SIMULATED = 1, LIVE = 2, QUICK = 3 };
const char* modeName(PlayMode m);

struct Match {
    int id = -1;
    Stage stage = Stage::LEAGUE;
    int leg = 0;            // 0 = single match, 1 / 2 = two-legged tie
    int matchday = 0;       // league: 1..8, knockout: round number
    int home = -1, away = -1;
    bool neutral = false;   // the final
    int tieNode = -1;       // knockout bracket node

    bool played = false;
    int hg = 0, ag = 0;
    bool extraTime = false;
    bool shootout = false;
    int penH = 0, penA = 0;
    bool abandoned = false;
    int forfeitSide = -1;   // side that fell below 7 players
    PlayMode mode = PlayMode::NONE;

    Lineup lineup[2];
    ds::LinkedList<MatchEvent> events;
    ds::LinkedList<PenaltyKick> kicks;

    int teamOf(int side) const { return side == 0 ? home : away; }
    int goalsOf(int team) const { return team == home ? hg : (team == away ? ag : 0); }
    int sideOf(int team) const { return team == home ? 0 : (team == away ? 1 : -1); }
};
