#include "Statistics.h"
#include <cmath>
#include "HashMap.h"
#include "Heap.h"
#include "MergeSort.h"
#include "Season.h"

namespace stats {

const char* rankByName(RankBy r) {
    switch (r) {
        case RankBy::GOALS: return "Top scorers";
        case RankBy::ASSISTS: return "Top assists";
        case RankBy::CONTRIBUTIONS: return "Goal contributions (G+A)";
        case RankBy::CLEAN_SHEETS: return "Clean sheets (goalkeepers)";
        case RankBy::MINUTES: return "Minutes played";
        case RankBy::DISCIPLINE: return "Discipline (cards)";
        case RankBy::IMPACT: return "Impact index (player of the season)";
        case RankBy::GOALS_PER90: return "Goals per 90 (min. 180')";
    }
    return "?";
}

double impactScore(const Player& p) {
    const PlayerStats& s = p.stats;
    double score = s.goals * 4.0 + s.assists * 3.0 - s.ownGoals * 2.0 - s.penMissed * 1.5 -
                   s.yellows * 0.5 - s.reds * 3.0 + s.minutes / 90.0 * 0.6;
    if (p.pos == Position::GK) score += s.cleanSheets * 3.0;
    score += (p.rating - 75) * 0.05 * (s.apps > 0 ? 1 : 0);
    return score;
}

double metric(RankBy r, const Player& p) {
    const PlayerStats& s = p.stats;
    switch (r) {
        case RankBy::GOALS: return s.goals;
        case RankBy::ASSISTS: return s.assists;
        case RankBy::CONTRIBUTIONS: return s.goals + s.assists;
        case RankBy::CLEAN_SHEETS: return p.pos == Position::GK ? s.cleanSheets : 0;
        case RankBy::MINUTES: return s.minutes;
        case RankBy::DISCIPLINE: return s.yellows + 3.0 * s.reds;
        case RankBy::IMPACT: return impactScore(p);
        case RankBy::GOALS_PER90: return s.minutes >= 180 ? s.goals * 90.0 / s.minutes : 0;
    }
    return 0;
}

bool ranksAbove(RankBy r, const Player& a, const Player& b) {
    double ma = metric(r, a), mb = metric(r, b);
    if (ma != mb) return ma > mb;
    // secondary keys
    if (r == RankBy::GOALS && a.stats.assists != b.stats.assists) return a.stats.assists > b.stats.assists;
    if (r == RankBy::ASSISTS && a.stats.goals != b.stats.goals) return a.stats.goals > b.stats.goals;
    if (r != RankBy::MINUTES && a.stats.minutes != b.stats.minutes) return a.stats.minutes < b.stats.minutes;
    return a.name < b.name;
}

namespace {
bool qualifies(RankBy r, const Player& p) {
    if (p.stats.apps == 0) return false;
    if (r == RankBy::CLEAN_SHEETS && p.pos != Position::GK) return false;
    if (r == RankBy::GOALS_PER90 && p.stats.minutes < 180) return false;
    return metric(r, p) > 0 || r == RankBy::IMPACT;
}
}  // namespace

void rankAll(const Season& s, RankBy r, ds::DynArray<const Player*>& out) {
    out.clear();
    for (const Team& t : s.teams)
        for (const Player& p : t.squad)
            if (qualifies(r, p)) out.push(&p);
    ds::mergeSort(out, [r](const Player* a, const Player* b) { return ranksAbove(r, *a, *b); });
}

void topN(const Season& s, RankBy r, int n, ds::DynArray<const Player*>& out) {
    // Bounded heap whose ROOT is the weakest of the current top N.
    struct Weaker {
        RankBy r;
        bool operator()(const Player* a, const Player* b) const { return ranksAbove(r, *b, *a); }
    };
    ds::Heap<const Player*, Weaker> heap(Weaker{r});
    for (const Team& t : s.teams) {
        for (const Player& p : t.squad) {
            if (!qualifies(r, p)) continue;
            heap.push(&p);
            if (heap.size() > n) heap.pop();      // drop the weakest
        }
    }
    out.clear();
    out.resize(heap.size());
    for (int i = heap.size() - 1; i >= 0; --i) out[i] = heap.pop();   // best first
}

// ---------------------------------------------------------------------------
double mean(const ds::DynArray<double>& v) {
    if (v.empty()) return 0;
    double s = 0;
    for (double x : v) s += x;
    return s / v.size();
}
double stddev(const ds::DynArray<double>& v) {
    if (v.size() < 2) return 0;
    double m = mean(v), s = 0;
    for (double x : v) s += (x - m) * (x - m);
    return std::sqrt(s / (v.size() - 1));
}
double median(ds::DynArray<double> v) {
    if (v.empty()) return 0;
    ds::mergeSort(v, [](double a, double b) { return a < b; });
    int n = v.size();
    return n % 2 ? v[n / 2] : (v[n / 2 - 1] + v[n / 2]) / 2.0;
}

Summary summarize(const Season& s) {
    Summary out;
    ds::DynArray<double> goals;
    int bestMargin = -1, bestTotal = -1;
    for (int id : s.playOrder) {
        const Match& m = s.match(id);
        int total = m.hg + m.ag;
        out.matches++;
        out.goals += total;
        goals.push(total);
        out.histogram[total > 10 ? 10 : total]++;
        if (m.hg > m.ag) out.homeWins++; else if (m.hg == m.ag) out.draws++; else out.awayWins++;
        if (m.extraTime) out.extraTimes++;
        if (m.shootout) out.shootouts++;
        int margin = m.hg > m.ag ? m.hg - m.ag : m.ag - m.hg;
        if (margin > bestMargin) { bestMargin = margin; out.biggestWin = id; }
        if (total > bestTotal) { bestTotal = total; out.highestScoring = id; }
        for (const MatchEvent& e : m.events) {
            switch (e.type) {
                case EventType::GOAL: case EventType::PEN_GOAL: case EventType::OWN_GOAL: {
                    int bucket = (e.minute - 1) / 15;
                    if (bucket > 7) bucket = 7;
                    if (bucket < 0) bucket = 0;
                    out.goalsByPeriod[bucket]++;
                    if (e.type == EventType::PEN_GOAL) out.penalties++;
                    if (e.type == EventType::OWN_GOAL) out.ownGoals++;
                    break;
                }
                case EventType::YELLOW: out.yellows++; break;
                case EventType::SECOND_YELLOW: out.yellows++; out.reds++; break;
                case EventType::RED: out.reds++; break;
                case EventType::INJURY: out.injuries++; break;
                case EventType::SUB: out.subs++; break;
                default: break;
            }
        }
    }
    out.meanGoals = mean(goals);
    out.stdGoals = stddev(goals);
    out.medianGoals = median(goals);
    return out;
}

void countryStats(const Season& s, ds::DynArray<CountryStat>& out) {
    ds::HashMap<std::string, CountryStat> map;
    for (const Team& t : s.teams) {
        CountryStat& c = map[t.country];
        c.country = t.country;
        c.teams++;
    }
    for (int id : s.playOrder) {
        const Match& m = s.match(id);
        const std::string& ch = s.team(m.home).country;
        const std::string& ca = s.team(m.away).country;
        CountryStat& h = map[ch];
        CountryStat& a = map[ca];
        h.played++; a.played++;
        h.gf += m.hg; h.ga += m.ag;
        a.gf += m.ag; a.ga += m.hg;
        if (m.hg > m.ag) { h.won++; a.lost++; }
        else if (m.hg < m.ag) { a.won++; h.lost++; }
        else { h.drawn++; a.drawn++; }
    }
    out.clear();
    map.forEach([&out](const std::string&, const CountryStat& c) { out.push(c); });
    ds::mergeSort(out, [](const CountryStat& a, const CountryStat& b) {
        if (a.ppg() != b.ppg()) return a.ppg() > b.ppg();
        return a.country < b.country;
    });
}

TeamStat teamStat(const Season& s, int team) {
    TeamStat ts;
    ts.team = team;
    for (int id : s.playOrder) {
        const Match& m = s.match(id);
        int side = m.sideOf(team);
        if (side < 0) continue;
        int f = side == 0 ? m.hg : m.ag, a = side == 0 ? m.ag : m.hg;
        ts.played++;
        ts.gf += f; ts.ga += a;
        if (f > a) ts.won++; else if (f == a) ts.drawn++; else ts.lost++;
        if (a == 0) ts.cleanSheets++;
        for (const MatchEvent& e : m.events) {
            if (e.side != side) continue;
            if (e.type == EventType::YELLOW) ts.yellows++;
            if (e.type == EventType::SECOND_YELLOW) { ts.yellows++; ts.reds++; }
            if (e.type == EventType::RED) ts.reds++;
        }
    }
    ts.eloChange = s.team(team).elo - s.team(team).startElo;
    return ts;
}

}  // namespace stats
