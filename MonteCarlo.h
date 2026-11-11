#pragma once
// ============================================================================
//  Monte-Carlo projections
//  Simulates the REST of the tournament thousands of times from the current
//  state (remaining league games with Elo/Poisson scores, then a freshly
//  drawn or the actual bracket) and counts how often each team finishes in
//  the top 8, in the play-off places, is eliminated, reaches each knockout
//  round and wins the title.
//
//  "What-if": any unplayed match can be forced to a chosen score to see the
//  aftermath of a result on everybody's odds.
// ============================================================================
#include "DynArray.h"
#include "Random.h"

class Season;

struct ForcedResult {
    int matchId = -1;
    int hg = 0, ag = 0;
};

struct MCResult {
    static const int MAXT = 36;
    int iterations = 0;
    double top8[MAXT], playoff[MAXT], out[MAXT];
    double r16[MAXT], qf[MAXT], sf[MAXT], final_[MAXT], win[MAXT];
    double avgPts[MAXT], avgRank[MAXT];
    MCResult();
};

void runMonteCarlo(const Season& s, int iterations, ds::Random& rng,
                   const ds::DynArray<ForcedResult>& forced, MCResult& out);
