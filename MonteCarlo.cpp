#include "MonteCarlo.h"
#include "HashMap.h"
#include "Elo.h"
#include "Season.h"

MCResult::MCResult() {
    for (int i = 0; i < MAXT; ++i) {
        top8[i] = playoff[i] = out[i] = 0;
        r16[i] = qf[i] = sf[i] = final_[i] = win[i] = 0;
        avgPts[i] = avgRank[i] = 0;
    }
}

namespace {

struct Ctx {
    const Season* s;
    ds::Random* rng;
    const ds::HashMap<int, ForcedResult>* forced;
    MCResult* out;
};

void simScore(Ctx& c, int home, int away, bool neutral, int& hg, int& ag) {
    double lh, la;
    elo::expectedGoals(c.s->team(home).elo, c.s->team(away).elo, neutral, lh, la);
    hg = c.rng->poisson(lh);
    ag = c.rng->poisson(la);
}

// Score of a match: actual if played, forced if requested, else simulated.
void legScore(Ctx& c, int matchId, int home, int away, bool neutral, int& hg, int& ag) {
    if (matchId >= 0) {
        const Match& m = c.s->match(matchId);
        if (m.played) { hg = m.hg; ag = m.ag; return; }
        const ForcedResult* f = c.forced->find(matchId);
        if (f) { hg = f->hg; ag = f->ag; return; }
    }
    simScore(c, home, away, neutral, hg, ag);
}

int penaltyWinner(Ctx& c, int a, int b) {
    double pa = 0.5 + (c.s->team(a).elo - c.s->team(b).elo) / 4000.0;   // slight skill tilt
    return c.rng->chance(pa) ? a : b;
}

// Recursively resolves a bracket node; returns the winning team.
int simNode(Ctx& c, Bracket& b, int node) {
    BracketNode& n = b.node(node);
    if (n.leaf) return n.team;
    int x = simNode(c, b, n.left);
    int y = simNode(c, b, n.right);
    double* counter = nullptr;
    switch (n.stage) {
        case Stage::R16: counter = c.out->r16; break;
        case Stage::QF: counter = c.out->qf; break;
        case Stage::SF: counter = c.out->sf; break;
        case Stage::FINAL: counter = c.out->final_; break;
        default: break;
    }
    if (counter) { counter[x] += 1; counter[y] += 1; }
    if (n.winner >= 0) return n.winner;

    int winner;
    if (n.stage == Stage::FINAL) {
        int hg, ag;
        legScore(c, n.leg1, x, y, true, hg, ag);
        if (hg == ag) {                       // extra time = ~1/3 of a match
            double lh, la;
            elo::expectedGoals(c.s->team(x).elo, c.s->team(y).elo, true, lh, la);
            hg += c.rng->poisson(lh / 3.0);
            ag += c.rng->poisson(la / 3.0);
        }
        winner = hg > ag ? x : (ag > hg ? y : penaltyWinner(c, x, y));
    } else {
        int better = b.betterRanked(x, y), worse = better == x ? y : x;
        int g1h, g1a, g2h, g2a;
        legScore(c, n.leg1, worse, better, false, g1h, g1a);     // worse at home
        legScore(c, n.leg2, better, worse, false, g2h, g2a);     // better at home
        int aggBetter = g1a + g2h, aggWorse = g1h + g2a;
        if (aggBetter == aggWorse) {
            double lh, la;
            elo::expectedGoals(c.s->team(better).elo, c.s->team(worse).elo, false, lh, la);
            aggBetter += c.rng->poisson(lh / 3.0);
            aggWorse += c.rng->poisson(la / 3.0);
        }
        winner = aggBetter > aggWorse ? better : (aggWorse > aggBetter ? worse : penaltyWinner(c, better, worse));
    }
    n.winner = winner;
    return winner;
}

}  // namespace

void runMonteCarlo(const Season& s, int iterations, ds::Random& rng,
                   const ds::DynArray<ForcedResult>& forced, MCResult& out) {
    out = MCResult();
    out.iterations = iterations;
    ds::HashMap<int, ForcedResult> forcedMap;
    for (const ForcedResult& f : forced) forcedMap.put(f.matchId, f);
    Ctx c{&s, &rng, &forcedMap, &out};
    int n = s.teams.size();

    for (int it = 0; it < iterations; ++it) {
        int ranking[36];
        Bracket b;
        if (s.phase == Phase::LEAGUE) {
            Standings st(n);
            for (const Match* m : s.matches) {
                if (m->stage != Stage::LEAGUE) continue;
                int hg, ag;
                legScore(c, m->id, m->home, m->away, false, hg, ag);
                st.record(m->home, m->away, hg, ag);
            }
            ds::DynArray<int> r = st.ranking(s.teams);
            for (int i = 0; i < n; ++i) {
                ranking[i] = r[i];
                out.avgPts[r[i]] += st.row(r[i]).pts;
            }
            b.build(ranking, n, rng);    // a fresh random knockout draw
        } else {
            for (int i = 0; i < n; ++i) {
                ranking[i] = s.leagueRanking[i];
                out.avgPts[ranking[i]] += s.table.row(ranking[i]).pts;
            }
            b = s.bracket;
        }
        for (int i = 0; i < n; ++i) {
            int t = ranking[i];
            out.avgRank[t] += i + 1;
            if (i < 8) out.top8[t] += 1;
            else if (i < 24) out.playoff[t] += 1;
            else out.out[t] += 1;
        }
        int champion = simNode(c, b, b.root());
        out.win[champion] += 1;
    }

    double inv = iterations > 0 ? 1.0 / iterations : 0.0;
    for (int t = 0; t < n; ++t) {
        out.top8[t] *= inv; out.playoff[t] *= inv; out.out[t] *= inv;
        out.r16[t] *= inv; out.qf[t] *= inv; out.sf[t] *= inv;
        out.final_[t] *= inv; out.win[t] *= inv;
        out.avgPts[t] *= inv; out.avgRank[t] *= inv;
    }
}
