#include "MatchSimulator.h"
#include <cmath>
#include "MergeSort.h"
#include "Elo.h"

namespace {
const double YELLOW_PER_MIN = 1.7 / 90.0;    // ~1.7 bookings per team per match
const double RED_PER_MIN = 0.025 / 90.0;
const double INJURY_PER_MIN = 0.10 / 90.0;
const double OWN_GOAL_SHARE = 0.03;
const double PENALTY_SHARE = 0.09;
const double PENALTY_CONVERSION = 0.78;
const double ASSIST_SHARE = 0.72;
}  // namespace

// ---------------------------------------------------------------------------
//  Line-up selection (enforces suspensions & injuries automatically)
// ---------------------------------------------------------------------------
Lineup MatchSimulator::autoLineup(const Team& t) {
    Lineup l;
    const int need[4] = {1, 4, 3, 3};            // 4-3-3
    bool used[MatchRecorder::MAXP] = {false};
    ds::DynArray<int> order;
    for (int i = 0; i < t.squad.size(); ++i) order.push(i);
    ds::mergeSort(order, [&t](int a, int b) { return t.squad[a].rating > t.squad[b].rating; });

    int n = 0;
    for (int p = 0; p < 4; ++p) {
        int got = 0;
        for (int i : order) {
            const Player& pl = t.squad[i];
            if (got >= need[p]) break;
            if (!used[i] && pl.available() && static_cast<int>(pl.pos) == p) {
                l.starters[n++] = i; used[i] = true; ++got;
            }
        }
    }
    // Fill holes (many absentees in one line) with the best outfielders
    for (int i : order) {
        if (n >= XI) break;
        const Player& pl = t.squad[i];
        if (!used[i] && pl.available() && pl.pos != Position::GK) { l.starters[n++] = i; used[i] = true; }
    }
    // Bench: one GK first, then best available
    for (int i : order) {
        if (l.benchCount >= 1) break;
        if (!used[i] && t.squad[i].available() && t.squad[i].pos == Position::GK) { l.bench[l.benchCount++] = i; used[i] = true; }
    }
    for (int i : order) {
        if (l.benchCount >= 9) break;
        if (!used[i] && t.squad[i].available()) { l.bench[l.benchCount++] = i; used[i] = true; }
    }
    return l;
}

// ---------------------------------------------------------------------------
//  Weighted player selection
// ---------------------------------------------------------------------------
namespace {
template <typename W>
int weightedPick(const MatchRecorder& rec, int side, ds::Random& rng, W weight) {
    ds::DynArray<int> on;
    rec.pitchPlayers(side, on);
    double total = 0;
    for (int p : on) total += weight(rec.team(side).squad[p], p);
    if (total <= 0 || on.empty()) return on.empty() ? -1 : on[0];
    double r = rng.uniform() * total;
    for (int p : on) {
        r -= weight(rec.team(side).squad[p], p);
        if (r <= 0) return p;
    }
    return on.back();
}
double posWeight(Position pos, double gk, double df, double mf, double fw) {
    switch (pos) {
        case Position::GK: return gk;
        case Position::DF: return df;
        case Position::MF: return mf;
        case Position::FW: return fw;
    }
    return 1;
}
double ratingFactor(int rating) { double f = rating / 75.0; return f * f * f; }
}  // namespace

int MatchSimulator::pickScorer(const MatchRecorder& rec, int side, int exclude) {
    return weightedPick(rec, side, rng_, [exclude](const Player& pl, int idx) {
        if (idx == exclude) return 0.0;
        return posWeight(pl.pos, 0.02, 1.2, 4.0, 10.0) * ratingFactor(pl.rating);
    });
}

int MatchSimulator::pickAssister(const MatchRecorder& rec, int side, int scorer) {
    return weightedPick(rec, side, rng_, [scorer](const Player& pl, int idx) {
        if (idx == scorer) return 0.0;
        return posWeight(pl.pos, 0.2, 2.5, 6.0, 5.0) * ratingFactor(pl.rating);
    });
}

int MatchSimulator::pickCardPlayer(const MatchRecorder& rec, int side) {
    return weightedPick(rec, side, rng_, [](const Player& pl, int) {
        return posWeight(pl.pos, 0.2, 3.0, 3.0, 1.5);
    });
}

// ---------------------------------------------------------------------------
//  Strength model: Elo + quality of the players actually on the pitch +
//  numerical (dis)advantage after red cards.
// ---------------------------------------------------------------------------
void MatchSimulator::goalRates(const MatchRecorder& rec, double& ph, double& pa) const {
    double adj[2];
    int men[2];
    for (int s = 0; s < 2; ++s) {
        ds::DynArray<int> on;
        rec.pitchPlayers(s, on);
        double sum = 0;
        for (int p : on) sum += rec.team(s).squad[p].rating;
        double avg = on.empty() ? 60 : sum / on.size();
        adj[s] = (avg - rec.team(s).bestXIRating()) * 12.0;   // missing stars hurt
        men[s] = on.size();
    }
    double lh, la;
    const Match& m = rec.match();
    elo::expectedGoals(rec.team(0).elo + adj[0], rec.team(1).elo + adj[1], m.neutral, lh, la);
    lh *= std::pow(0.80, XI - men[0]) * std::pow(1.18, XI - men[1]);
    la *= std::pow(0.80, XI - men[1]) * std::pow(1.18, XI - men[0]);
    double fatigue = rec.inExtraTime() ? 0.85 : 1.0;
    ph = lh / 90.0 * fatigue;
    pa = la / 90.0 * fatigue;
}

void MatchSimulator::goalEvent(MatchRecorder& rec, int side, int minute) {
    std::string err;
    double r = rng_.uniform();
    if (r < OWN_GOAL_SHARE) {
        int victim = weightedPick(rec, 1 - side, rng_, [](const Player& pl, int) {
            return posWeight(pl.pos, 0.5, 4.0, 1.0, 0.3);
        });
        rec.ownGoal(1 - side, minute, victim, err);
        return;
    }
    if (r < OWN_GOAL_SHARE + PENALTY_SHARE) {
        // Penalty: the best-rated outfielder on the pitch steps up
        ds::DynArray<int> on;
        rec.pitchPlayers(side, on);
        int taker = -1;
        for (int p : on) {
            const Player& pl = rec.team(side).squad[p];
            if (pl.pos == Position::GK) continue;
            if (taker < 0 || pl.rating > rec.team(side).squad[taker].rating) taker = p;
        }
        if (taker < 0) return;
        if (rng_.chance(PENALTY_CONVERSION)) rec.goal(side, minute, taker, -1, true, err);
        else rec.penaltyMiss(side, minute, taker, err);
        return;
    }
    int scorer = pickScorer(rec, side);
    if (scorer < 0) return;
    int assist = rng_.chance(ASSIST_SHARE) ? pickAssister(rec, side, scorer) : -1;
    if (!rec.goal(side, minute, scorer, assist, false, err))
        rec.goal(side, minute, scorer, -1, false, err);
}

// ---------------------------------------------------------------------------
//  Substitutions (AI manager)
// ---------------------------------------------------------------------------
int MatchSimulator::bestReplacement(const MatchRecorder& rec, int side, int outPlayer) const {
    ds::DynArray<int> bench;
    rec.benchPlayers(side, bench);
    const Team& t = rec.team(side);
    Position want = t.squad[outPlayer].pos;
    int best = -1;
    for (int p : bench) {
        const Player& pl = t.squad[p];
        if (want == Position::GK ? pl.pos != Position::GK : pl.pos == Position::GK) continue;
        bool samePos = pl.pos == want;
        if (best < 0) { best = p; continue; }
        const Player& b = t.squad[best];
        bool bestSame = b.pos == want;
        if (samePos != bestSame) { if (samePos) best = p; continue; }
        if (pl.rating > b.rating) best = p;
    }
    return best;
}

void MatchSimulator::aiSubstitutions(MatchRecorder& rec, int side, int minute) {
    std::string err;
    const Team& t = rec.team(side);
    // 1) injured players are replaced immediately if possible
    ds::DynArray<int> on;
    rec.pitchPlayers(side, on);
    for (int p : on) {
        if (rec.injuredInMatch(side, p) && rec.canSubAt(side, minute)) {
            int in = bestReplacement(rec, side, p);
            if (in >= 0) rec.sub(side, minute, p, in, err);
        }
    }
    // 2) planned changes
    int planned = 0;
    if (minute == 60) planned = 2;
    else if (minute == 70) planned = 1 + (rec.score(side) < rec.score(1 - side) ? 1 : 0);
    else if (minute == 80) planned = 1;
    else if (minute == 46 && rec.score(side) + 1 < rec.score(1 - side)) planned = 1;
    else if (minute == 91 || minute == 106) planned = 1;
    for (int k = 0; k < planned; ++k) {
        if (!rec.canSubAt(side, minute)) break;
        rec.pitchPlayers(side, on);
        // Prefer booked players, then the lowest-rated outfield player
        int out = -1;
        for (int p : on) {
            if (t.squad[p].pos == Position::GK) continue;
            if (rec.entered(side, p) > 0) continue;   // do not remove someone who came on
            if (out < 0) { out = p; continue; }
            bool booked = rec.yellowCount(side, p) > 0, outBooked = rec.yellowCount(side, out) > 0;
            if (booked != outBooked) { if (booked) out = p; continue; }
            if (t.squad[p].rating < t.squad[out].rating) out = p;
        }
        if (out < 0) break;
        int in = bestReplacement(rec, side, out);
        if (in < 0) break;
        if (!rec.sub(side, minute, out, in, err)) break;
    }
}

// ---------------------------------------------------------------------------
//  Main loop
// ---------------------------------------------------------------------------
void MatchSimulator::simulateTo(MatchRecorder& rec, int toMinute) {
    int first = rec.minute() + 1;
    if (rec.inExtraTime() && first < 91) first = 91;
    int last = rec.inExtraTime() ? (toMinute > 120 ? 120 : toMinute) : (toMinute > 90 ? 90 : toMinute);
    std::string err;
    for (int minute = first; minute <= last; ++minute) {
        if (rec.isAbandoned()) return;
        double ph, pa;
        goalRates(rec, ph, pa);
        for (int side = 0; side < 2 && !rec.isAbandoned(); ++side) {
            aiSubstitutions(rec, side, minute);
            if (rng_.chance(side == 0 ? ph : pa)) goalEvent(rec, side, minute);
            if (rng_.chance(YELLOW_PER_MIN * (minute > 60 ? 1.3 : 0.8))) {
                int p = pickCardPlayer(rec, side);
                // booked players play carefully: a second caution is less likely
                if (p >= 0 && (rec.yellowCount(side, p) == 0 || rng_.chance(0.35))) rec.yellow(side, minute, p, err);
            }
            if (!rec.isAbandoned() && rng_.chance(RED_PER_MIN)) {
                int p = pickCardPlayer(rec, side);
                if (p >= 0) rec.red(side, minute, p, rng_.chance(0.8) ? 1 : 2, err);
            }
            if (!rec.isAbandoned() && rng_.chance(INJURY_PER_MIN)) {
                ds::DynArray<int> on;
                rec.pitchPlayers(side, on);
                if (!on.empty()) {
                    int p = on[rng_.range(0, on.size() - 1)];
                    int length = rng_.chance(0.6) ? 1 : rng_.range(2, 5);
                    if (rec.injury(side, minute, p, length, err)) aiSubstitutions(rec, side, minute);
                }
            }
        }
        rec.setClock(minute);
    }
}

void MatchSimulator::simulateShootout(MatchRecorder& rec) {
    std::string err;
    if (!rec.inShootout() && !rec.beginShootout(err)) return;
    // Kicker order: best outfielders first, goalkeeper last
    ds::DynArray<int> order[2];
    for (int s = 0; s < 2; ++s) {
        rec.pitchPlayers(s, order[s]);
        const Team& t = rec.team(s);
        ds::mergeSort(order[s], [&t](int a, int b) {
            bool ga = t.squad[a].pos == Position::GK, gb = t.squad[b].pos == Position::GK;
            if (ga != gb) return gb;
            return t.squad[a].rating > t.squad[b].rating;
        });
    }
    int gkRating[2] = {75, 75};
    for (int s = 0; s < 2; ++s)
        for (int p : order[s]) if (rec.team(s).squad[p].pos == Position::GK) gkRating[s] = rec.team(s).squad[p].rating;

    int nextIdx[2] = {0, 0};
    int guard = 0;
    while (!rec.shootoutDecided() && guard++ < 200) {
        int s = rec.nextKickSide();
        if (order[s].empty()) break;
        int p = order[s][nextIdx[s] % order[s].size()];
        nextIdx[s]++;
        double conv = 0.75 + (rec.team(s).squad[p].rating - 75) * 0.006 - (gkRating[1 - s] - 75) * 0.004;
        if (conv < 0.55) conv = 0.55;
        if (conv > 0.92) conv = 0.92;
        rec.penaltyKick(s, p, rng_.chance(conv), err);
    }
}

void MatchSimulator::quickScore(MatchRecorder& rec, int hg, int ag, int fromMinute, int toMinute) {
    // Random goal minutes, sorted with merge sort, then a quiet simulation
    // with no random goals; cards/subs still happen for realism.
    int total = hg + ag;
    ds::DynArray<int> minutes, sides;
    for (int i = 0; i < total; ++i) minutes.push(rng_.range(fromMinute, toMinute));
    ds::mergeSort(minutes, [](int a, int b) { return a < b; });
    for (int i = 0; i < total; ++i) sides.push(i < hg ? 0 : 1);
    rng_.shuffle(sides.raw(), sides.size());

    std::string err;
    int gi = 0;
    for (int minute = fromMinute; minute <= toMinute; ++minute) {
        if (rec.isAbandoned()) break;
        for (int side = 0; side < 2; ++side) {
            aiSubstitutions(rec, side, minute);
            if (rng_.chance(YELLOW_PER_MIN)) {
                int p = pickCardPlayer(rec, side);
                // Avoid second yellows here so the requested score stays valid
                if (p >= 0 && rec.yellowCount(side, p) == 0) rec.yellow(side, minute, p, err);
            }
        }
        while (gi < total && minutes[gi] == minute) {
            int side = sides[gi++];
            int scorer = pickScorer(rec, side);
            int assist = rng_.chance(ASSIST_SHARE) ? pickAssister(rec, side, scorer) : -1;
            if (!rec.goal(side, minute, scorer, assist, false, err)) rec.goal(side, minute, scorer, -1, false, err);
        }
        rec.setClock(minute);
    }
}
