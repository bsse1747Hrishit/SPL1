#pragma once
// ============================================================================
//  Player - a squad member with season statistics and availability status
//  (suspension / injury), which line-up selection enforces automatically.
// ============================================================================
#include <string>

enum class Position { GK = 0, DF = 1, MF = 2, FW = 3 };

inline const char* positionName(Position p) {
    switch (p) {
        case Position::GK: return "GK";
        case Position::DF: return "DF";
        case Position::MF: return "MF";
        case Position::FW: return "FW";
    }
    return "??";
}

inline bool parsePosition(const std::string& s, Position& out) {
    if (s == "GK") { out = Position::GK; return true; }
    if (s == "DF") { out = Position::DF; return true; }
    if (s == "MF") { out = Position::MF; return true; }
    if (s == "FW") { out = Position::FW; return true; }
    return false;
}

struct PlayerStats {
    int apps = 0;          // appearances (start or sub)
    int starts = 0;
    int subApps = 0;
    int minutes = 0;
    int goals = 0;
    int penGoals = 0;
    int assists = 0;
    int ownGoals = 0;
    int penMissed = 0;
    int yellows = 0;
    int reds = 0;
    int cleanSheets = 0;   // goalkeepers only
    int injuries = 0;
    int matchesMissed = 0; // through suspension or injury
};

struct Player {
    int id = -1;           // global id = teamId * 100 + squadIndex
    int teamId = -1;
    int squadIndex = -1;
    std::string name;
    Position pos = Position::MF;
    int number = 0;
    int rating = 70;       // 40..99 overall ability
    bool generated = false;  // auto-added by squad validation

    PlayerStats stats;

    // Discipline / fitness status
    int yellowAccum = 0;   // yellows counting toward an accumulation ban
    int suspended = 0;     // matches still to serve
    int injured = 0;       // matches still to miss
    std::string injuryNote;

    bool available() const { return suspended == 0 && injured == 0; }

    std::string statusText() const {
        if (suspended > 0) return "SUSPENDED (" + std::to_string(suspended) + ")";
        if (injured > 0) return "INJURED (" + std::to_string(injured) + ")";
        if (yellowAccum == 2 || yellowAccum == 4 || yellowAccum == 6) return "1 yellow from ban";
        return "Available";
    }
};
