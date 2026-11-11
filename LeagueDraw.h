#pragma once
// ============================================================================
//  League-phase draw (UEFA "Swiss model", 2024/25 onward)
//   * 36 teams in 4 pots of 9; each team plays 8 different opponents:
//     2 from every pot - one at home, one away (4 home + 4 away)
//   * no opponent from the same country (association)
//   * at most 2 opponents from any one other country
//
//  The draw is modelled as a GRAPH problem: teams are vertices and fixtures
//  are directed edges (home -> away). For every pot pair we must find a
//  perfect bipartite matching "host -> visitor" that respects all
//  constraints; solved with randomised backtracking + forward checking.
//
//  Scheduling: the resulting 8-regular graph is split into 8 matchdays,
//  each being a PERFECT MATCHING (every team plays exactly once), again by
//  backtracking with a "fewest options first" heuristic. Home/away runs are
//  balanced so no team has three home or three away games in a row.
// ============================================================================
#include <string>
#include "DynArray.h"
#include "Random.h"
#include "Team.h"

struct FixturePair {
    int home = -1;
    int away = -1;
};

class LeagueDraw {
public:
    static const int MATCHES_PER_TEAM = 8;
    static const int MATCHDAYS = 8;

    // Produces 144 pairings. Returns false if no valid draw was found.
    static bool draw(const ds::DynArray<Team>& teams, ds::Random& rng,
                     ds::DynArray<FixturePair>& out, std::string& log);

    // Assigns each pairing a matchday (0..7). mdOut[i] = matchday of pairs[i].
    static bool schedule(int teamCount, const ds::DynArray<FixturePair>& pairs,
                         ds::Random& rng, ds::DynArray<int>& mdOut, std::string& log);
};
