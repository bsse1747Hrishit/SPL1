// ============================================================================
//  Squad validation rules
//   * 16..25 players (UEFA List A max 25)
//   * shirt numbers 1..99, unique within the squad
//   * player names unique within the squad
//   * rating within 40..99
//   * at least 2 GK, 5 DF, 5 MF, 3 FW (so a 4-3-3 plus bench is always
//     possible). Missing positions are filled with academy players.
// ============================================================================
#include "Team.h"
#include "HashMap.h"
#include "MergeSort.h"

namespace {

std::string lower(const std::string& s) {
    std::string r = s;
    for (char& c : r) if (c >= 'A' && c <= 'Z') c = static_cast<char>(c - 'A' + 'a');
    return r;
}

int freeNumber(const ds::HashMap<int, int>& used) {
    for (int n = 1; n <= 99; ++n)
        if (!used.contains(n)) return n;
    return -1;
}

}  // namespace

double Team::bestXIRating() const {
    // Take the top 1 GK, 4 DF, 3 MF, 3 FW by rating.
    const int need[4] = {1, 4, 3, 3};
    double sum = 0;
    int count = 0;
    for (int p = 0; p < 4; ++p) {
        ds::DynArray<int> ratings;
        for (const Player& pl : squad)
            if (static_cast<int>(pl.pos) == p) ratings.push(pl.rating);
        ds::mergeSort(ratings, [](int a, int b) { return a > b; });
        for (int i = 0; i < need[p] && i < ratings.size(); ++i) { sum += ratings[i]; ++count; }
    }
    return count ? sum / count : 70.0;
}

bool validateAndCompleteSquad(Team& t, std::string& log) {
    if (t.name.empty() || t.code.empty()) {
        log += "  [ERROR] team with empty name/code\n";
        return false;
    }
    if (t.pot < 1 || t.pot > 4) {
        log += "  [ERROR] " + t.name + ": pot must be 1..4\n";
        return false;
    }
    if (t.elo < 1000 || t.elo > 2500) {
        log += "  [FIX] " + t.name + ": Elo out of range, reset to 1600\n";
        t.elo = 1600;
    }
    if (t.squad.size() > MAX_SQUAD) {
        log += "  [FIX] " + t.name + ": squad trimmed to " + std::to_string(MAX_SQUAD) + " (UEFA List A)\n";
        while (t.squad.size() > MAX_SQUAD) t.squad.pop();
    }

    ds::HashMap<int, int> numbers;
    ds::HashMap<std::string, int> names;
    for (int i = 0; i < t.squad.size(); ++i) {
        Player& p = t.squad[i];
        if (p.rating < 40 || p.rating > 99) {
            int fixed = p.rating < 40 ? 40 : 99;
            log += "  [FIX] " + p.name + ": rating " + std::to_string(p.rating) + " -> " + std::to_string(fixed) + "\n";
            p.rating = fixed;
        }
        if (p.number < 1 || p.number > 99 || numbers.contains(p.number)) {
            int n = freeNumber(numbers);
            log += "  [FIX] " + t.code + " " + p.name + ": shirt #" + std::to_string(p.number) +
                   " invalid/duplicate -> #" + std::to_string(n) + "\n";
            p.number = n;
        }
        numbers.put(p.number, i);
        std::string key = lower(p.name);
        if (names.contains(key)) {
            p.name += " II";
            log += "  [FIX] duplicate name in " + t.code + " renamed to " + p.name + "\n";
            key = lower(p.name);
        }
        names.put(key, i);
    }

    // Positional minimums - pad with academy players
    const int minimum[4] = {2, 5, 5, 3};
    int academy = 1;
    for (int p = 0; p < 4; ++p) {
        Position pos = static_cast<Position>(p);
        while (t.countPosition(pos) < minimum[p] && t.squad.size() < MAX_SQUAD) {
            Player np;
            np.name = t.code + " Academy " + positionName(pos) + " " + std::to_string(academy++);
            np.pos = pos;
            np.rating = 62;
            np.number = freeNumber(numbers);
            np.generated = true;
            numbers.put(np.number, t.squad.size());
            t.squad.push(np);
        }
    }
    while (t.squad.size() < MIN_SQUAD) {
        Player np;
        np.name = t.code + " Academy MF " + std::to_string(academy++);
        np.pos = Position::MF;
        np.rating = 62;
        np.number = freeNumber(numbers);
        np.generated = true;
        numbers.put(np.number, t.squad.size());
        t.squad.push(np);
    }
    if (academy > 1)
        log += "  [PAD] " + t.name + ": added " + std::to_string(academy - 1) + " academy player(s) to meet squad rules\n";

    // Assign identities
    for (int i = 0; i < t.squad.size(); ++i) {
        t.squad[i].teamId = t.id;
        t.squad[i].squadIndex = i;
        t.squad[i].id = t.id * 100 + i;
    }
    t.startElo = t.elo;
    return true;
}
