#pragma once
// ============================================================================
//  Team - a club with its squad, seeding pot and Elo strength rating.
// ============================================================================
#include <string>
#include "DynArray.h"
#include "Player.h"

const int MAX_SQUAD = 25;      // UEFA "List A" size
const int MIN_SQUAD = 16;

struct Team {
    int id = -1;
    std::string code;          // 3-letter short code, e.g. "BAY"
    std::string name;
    std::string country;       // 3-letter country code, e.g. "GER"
    std::string stadium;
    int pot = 0;               // 1..4 seeding pot
    double elo = 1500.0;
    double startElo = 1500.0;
    ds::DynArray<Player> squad;
    ds::DynArray<double> eloHistory;   // Elo after each match

    int indexByNumber(int number) const {
        for (int i = 0; i < squad.size(); ++i)
            if (squad[i].number == number) return i;
        return -1;
    }
    int countPosition(Position p) const {
        int c = 0;
        for (const Player& pl : squad) if (pl.pos == p) ++c;
        return c;
    }
    int availableCount() const {
        int c = 0;
        for (const Player& pl : squad) if (pl.available()) ++c;
        return c;
    }
    // Average rating of the best possible 4-3-3 XI (ignoring availability)
    double bestXIRating() const;
};

// Validates a squad against the rules (unique names/numbers, rating range,
// size limits, positional minimums) and repairs/pads it. Returns false only
// on unrecoverable problems. Messages are appended to `log`.
bool validateAndCompleteSquad(Team& t, std::string& log);

// Loads the 36-team database (embedded, or "teams.txt" if present) and
// validates it. Implemented in TeamData.cpp.
bool loadTeamDatabase(ds::DynArray<Team>& teams, std::string& log);

// Writes the embedded database to a file so users can edit it.
bool exportTeamDatabase(const std::string& path);
