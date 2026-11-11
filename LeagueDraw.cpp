#include "LeagueDraw.h"
#include "HashMap.h"

namespace {

const int N = 36;
const int PER_POT = 9;
const int MAXC = 36;

// ---------------------------------------------------------------------------
//  Draw state
// ---------------------------------------------------------------------------
struct DrawState {
    int country[N];
    int potTeams[4][PER_POT];
    bool met[N][N];
    int countryCount[N][MAXC];   // opponents per country for each team

    // A task = "every team of hostPot hosts exactly one team of visitorPot"
    int taskHost[16], taskVisitor[16];
    int taskCount = 0;
    int hostOrder[16][PER_POT];   // shuffled host order per task
    bool visitorUsed[16][PER_POT];

    ds::DynArray<FixturePair> pairs;
    long long budget = 0;
};

bool allowed(const DrawState& st, int h, int v) {
    if (h == v || st.met[h][v]) return false;
    int ch = st.country[h], cv = st.country[v];
    if (ch == cv) return false;                         // same association
    if (st.countryCount[h][cv] >= 2) return false;      // max 2 per country
    if (st.countryCount[v][ch] >= 2) return false;
    return true;
}

void assign(DrawState& st, int h, int v, int delta) {
    bool on = delta > 0;
    st.met[h][v] = st.met[v][h] = on;
    st.countryCount[h][st.country[v]] += delta;
    st.countryCount[v][st.country[h]] += delta;
}

// Forward check: every remaining host of the current task still has an option.
bool forwardOk(const DrawState& st, int task, int fromK) {
    int vp = st.taskVisitor[task];
    for (int k = fromK; k < PER_POT; ++k) {
        int h = st.hostOrder[task][k];
        bool any = false;
        for (int j = 0; j < PER_POT && !any; ++j)
            if (!st.visitorUsed[task][j] && allowed(st, h, st.potTeams[vp][j])) any = true;
        if (!any) return false;
    }
    return true;
}

bool solve(DrawState& st, ds::Random& rng, int task, int k) {
    if (--st.budget < 0) return false;
    if (task == st.taskCount) return true;
    if (k == PER_POT) return solve(st, rng, task + 1, 0);

    int h = st.hostOrder[task][k];
    int vp = st.taskVisitor[task];
    int cand[PER_POT];
    for (int j = 0; j < PER_POT; ++j) cand[j] = j;
    rng.shuffle(cand, PER_POT);

    for (int c = 0; c < PER_POT; ++c) {
        int j = cand[c];
        int v = st.potTeams[vp][j];
        if (st.visitorUsed[task][j] || !allowed(st, h, v)) continue;
        st.visitorUsed[task][j] = true;
        assign(st, h, v, +1);
        FixturePair fp;
        fp.home = h; fp.away = v;
        st.pairs.push(fp);
        if (forwardOk(st, task, k + 1) && solve(st, rng, task, k + 1)) return true;
        st.pairs.pop();
        assign(st, h, v, -1);
        st.visitorUsed[task][j] = false;
        if (st.budget < 0) return false;
    }
    return false;
}

}  // namespace

bool LeagueDraw::draw(const ds::DynArray<Team>& teams, ds::Random& rng,
                      ds::DynArray<FixturePair>& out, std::string& log) {
    DrawState* st = new DrawState();     // ~10 KB, keep it off the stack
    ds::HashMap<std::string, int> countryIds;
    int potFill[4] = {0, 0, 0, 0};
    for (const Team& t : teams) {
        if (!countryIds.contains(t.country)) countryIds.put(t.country, countryIds.size());
        st->country[t.id] = *countryIds.find(t.country);
        st->potTeams[t.pot - 1][potFill[t.pot - 1]++] = t.id;
    }
    // 4 intra-pot tasks + 12 cross-pot tasks (both directions) = 16
    for (int i = 0; i < 4; ++i) { st->taskHost[st->taskCount] = i; st->taskVisitor[st->taskCount++] = i; }
    for (int i = 0; i < 4; ++i)
        for (int j = 0; j < 4; ++j)
            if (i != j) { st->taskHost[st->taskCount] = i; st->taskVisitor[st->taskCount++] = j; }

    bool ok = false;
    int attempt = 0;
    for (attempt = 1; attempt <= 400 && !ok; ++attempt) {
        for (int a = 0; a < N; ++a) {
            for (int b = 0; b < N; ++b) st->met[a][b] = false;
            for (int c = 0; c < MAXC; ++c) st->countryCount[a][c] = 0;
        }
        for (int t = 0; t < st->taskCount; ++t) {
            for (int k = 0; k < PER_POT; ++k) {
                st->hostOrder[t][k] = st->potTeams[st->taskHost[t]][k];
                st->visitorUsed[t][k] = false;
            }
            rng.shuffle(st->hostOrder[t], PER_POT);
        }
        st->pairs.clear();
        st->budget = 60000;
        ok = solve(*st, rng, 0, 0);
    }
    if (ok) {
        out = st->pairs;
        log += "League-phase draw completed in " + std::to_string(attempt - 1) + " attempt(s): " +
               std::to_string(out.size()) + " fixtures.\n";
    } else {
        log += "[ERROR] could not find a valid draw.\n";
    }
    delete st;
    return ok;
}

// ---------------------------------------------------------------------------
//  Matchday scheduling: decompose the fixture graph into perfect matchings
// ---------------------------------------------------------------------------
namespace {

struct SchedState {
    int n = 0;
    const ds::DynArray<FixturePair>* pairs = nullptr;
    ds::DynArray<int> adj[N];        // pair indices per team
    int* md = nullptr;               // matchday per pair (-1 = none)
    bool matched[N];
    int venue[N][LeagueDraw::MATCHDAYS];   // +1 home, -1 away, 0 none
    long long budget = 0;
};

bool venueOk(const SchedState& s, int team, int day, int v) {
    // reject three consecutive home (or away) matches
    if (day >= 2 && s.venue[team][day - 1] == v && s.venue[team][day - 2] == v) return false;
    return true;
}

bool matchDay(SchedState& s, ds::Random& rng, int day, int remaining) {
    if (--s.budget < 0) return false;
    if (remaining == 0) return true;
    // Most-constrained team first
    int best = -1, bestOpts = 1000;
    for (int t = 0; t < s.n; ++t) {
        if (s.matched[t]) continue;
        int opts = 0;
        for (int e : s.adj[t]) {
            const FixturePair& p = (*s.pairs)[e];
            int o = p.home == t ? p.away : p.home;
            if (s.md[e] < 0 && !s.matched[o]) ++opts;
        }
        if (opts < bestOpts) { bestOpts = opts; best = t; }
    }
    if (bestOpts == 0) return false;

    ds::DynArray<int> opts;
    for (int e : s.adj[best]) {
        const FixturePair& p = (*s.pairs)[e];
        int o = p.home == best ? p.away : p.home;
        if (s.md[e] < 0 && !s.matched[o]) opts.push(e);
    }
    rng.shuffle(opts.raw(), opts.size());
    for (int e : opts) {
        const FixturePair& p = (*s.pairs)[e];
        if (!venueOk(s, p.home, day, +1) || !venueOk(s, p.away, day, -1)) continue;
        s.md[e] = day;
        s.matched[p.home] = s.matched[p.away] = true;
        s.venue[p.home][day] = +1;
        s.venue[p.away][day] = -1;
        if (matchDay(s, rng, day, remaining - 2)) return true;
        s.venue[p.home][day] = s.venue[p.away][day] = 0;
        s.matched[p.home] = s.matched[p.away] = false;
        s.md[e] = -1;
        if (s.budget < 0) return false;
    }
    return false;
}

}  // namespace

bool LeagueDraw::schedule(int teamCount, const ds::DynArray<FixturePair>& pairs,
                          ds::Random& rng, ds::DynArray<int>& mdOut, std::string& log) {
    SchedState* s = new SchedState();
    s->n = teamCount;
    s->pairs = &pairs;
    for (int i = 0; i < pairs.size(); ++i) {
        s->adj[pairs[i].home].push(i);
        s->adj[pairs[i].away].push(i);
    }
    mdOut = ds::DynArray<int>(pairs.size(), -1);
    s->md = mdOut.raw();

    bool ok = false;
    int attempt = 0;
    for (attempt = 1; attempt <= 3000 && !ok; ++attempt) {
        for (int i = 0; i < pairs.size(); ++i) s->md[i] = -1;
        for (int t = 0; t < teamCount; ++t)
            for (int d = 0; d < MATCHDAYS; ++d) s->venue[t][d] = 0;
        ok = true;
        for (int day = 0; day < MATCHDAYS && ok; ++day) {
            for (int t = 0; t < teamCount; ++t) s->matched[t] = false;
            s->budget = 4000;
            ok = matchDay(*s, rng, day, teamCount);
        }
    }
    if (ok) log += "Schedule built in " + std::to_string(attempt - 1) + " attempt(s): 8 matchdays x 18 matches.\n";
    else log += "[ERROR] could not build a matchday schedule.\n";
    delete s;
    return ok;
}
