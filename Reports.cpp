#include "Reports.h"
#include "Features.h"
#include "MergeSort.h"
#include "Elo.h"
#include "Season.h"
#include "Console.h"

using ui::padL;
using ui::padR;

namespace reports {

// ---------------------------------------------------------------------------
//  Small helpers
// ---------------------------------------------------------------------------
std::string scoreLine(const Season& s, const Match& m) {
    std::string h = s.team(m.home).name, a = s.team(m.away).name;
    if (!m.played) return h + " v " + a;
    std::string r = h + " " + std::to_string(m.hg) + "-" + std::to_string(m.ag) + " " + a;
    if (m.abandoned) r += " (abandoned, awarded)";
    else if (m.shootout) r += " (aet, " + std::to_string(m.penH) + "-" + std::to_string(m.penA) + " pens)";
    else if (m.extraTime) r += " (aet)";
    return r;
}

namespace {
std::string pname(const Season& s, const Match& m, int side, int idx) {
    if (idx < 0) return "-";
    const Player& p = s.team(m.teamOf(side)).squad[idx];
    return p.name + " (" + std::to_string(p.number) + ")";
}
std::string minuteText(int minute) {
    if (minute == 46) return "HT";
    return std::to_string(minute) + "'";
}
std::string roundLabel(const Match& m) {
    if (m.stage == Stage::LEAGUE) return "MD" + std::to_string(m.matchday);
    std::string r = stageShort(m.stage);
    if (m.leg == 1) r += " L1";
    else if (m.leg == 2) r += " L2";
    return r;
}
}  // namespace

std::string eventText(const Season& s, const Match& m, const MatchEvent& e) {
    std::string who = pname(s, m, e.side, e.player);
    std::string code = s.team(m.teamOf(e.side)).code;
    std::string t = padL(minuteText(e.minute), 4) + "  " + padR(std::string("[") + eventTag(e.type) + "]", 7) + code + "  ";
    switch (e.type) {
        case EventType::GOAL:
            t += "GOAL! " + who;
            if (e.player2 >= 0) t += "  (assist: " + pname(s, m, e.side, e.player2) + ")";
            break;
        case EventType::PEN_GOAL: t += "GOAL! " + who + " from the penalty spot"; break;
        case EventType::OWN_GOAL: t += "Own goal by " + who; break;
        case EventType::PEN_MISS: t += "Penalty missed/saved: " + who; break;
        case EventType::YELLOW: t += "Yellow card: " + who; break;
        case EventType::SECOND_YELLOW: t += "Second yellow -> RED: " + who + " is sent off"; break;
        case EventType::RED: t += "RED CARD: " + who + " (ban " + std::to_string(e.extra) + ")"; break;
        case EventType::SUB: t += "OFF " + who + "  ON " + pname(s, m, e.side, e.player2); break;
        case EventType::INJURY: t += "Injury: " + who + " (out " + std::to_string(e.extra) + " match(es))"; break;
    }
    return t;
}

void matchLine(std::ostream& os, const Season& s, const Match& m) {
    os << "  #" << padL(std::to_string(m.id), 3) << "  " << padR(roundLabel(m), 7)
       << padL(s.team(m.home).name, 26) << "  ";
    if (m.played) os << padL(std::to_string(m.hg), 2) << " - " << padR(std::to_string(m.ag), 2);
    else os << "  v    ";
    os << "  " << padR(s.team(m.away).name, 26);
    if (m.shootout) os << " p" << m.penH << "-" << m.penA;
    else if (m.extraTime) os << " aet";
    else if (m.abandoned) os << " awd";
    os << "\n";
}

// ---------------------------------------------------------------------------
//  League table
// ---------------------------------------------------------------------------
void table(std::ostream& os, const Season& s, int hA, int hB) {
    ds::DynArray<int> r = s.currentRanking();
    os << ui::banner("UEFA CHAMPIONS LEAGUE - LEAGUE PHASE TABLE");
    os << "  Pos  Club                        P   W  D  L   GF  GA   GD  Pts  Form   Zone\n";
    os << "  " << ui::rule('-', 76) << "\n";
    for (int i = 0; i < r.size(); ++i) {
        const TableRow& row = s.table.row(r[i]);
        const Team& t = s.team(r[i]);
        std::string mark = (r[i] == hA || r[i] == hB) ? ">" : " ";
        os << mark << padL(std::to_string(i + 1), 4) << "  " << padR(t.name, 26)
           << padL(std::to_string(row.played), 3) << padL(std::to_string(row.won), 4)
           << padL(std::to_string(row.drawn), 3) << padL(std::to_string(row.lost), 3)
           << padL(std::to_string(row.gf), 5) << padL(std::to_string(row.ga), 4)
           << padL(ui::signedNum(row.gd()), 5) << padL(std::to_string(row.pts), 5)
           << "  " << padR(row.form, 6) << " " << Standings::zone(i) << "\n";
        if (i == 7 || i == 23) os << "  " << ui::rule('.', 76) << "\n";
    }
    os << "  R16 = straight to Round of 16 | PO = knockout play-off | OUT = eliminated\n"
          "  Tiebreakers: points, goal difference, goals scored, head-to-head, alphabetical\n";
}

// ---------------------------------------------------------------------------
//  Fixtures
// ---------------------------------------------------------------------------
void fixtures(std::ostream& os, const Season& s, int matchday) {
    if (matchday == -1) {
        os << ui::banner("KNOCKOUT FIXTURES & RESULTS");
        Stage last = Stage::LEAGUE;
        bool any = false;
        for (const Match* m : s.matches) {
            if (m->stage == Stage::LEAGUE) continue;
            if (m->stage != last) { os << "\n  -- " << stageName(m->stage) << " --\n"; last = m->stage; }
            matchLine(os, s, *m);
            any = true;
        }
        if (!any) os << "  The knockout rounds have not been drawn yet (after matchday 8).\n";
        return;
    }
    for (int md = 1; md <= 8; ++md) {
        if (matchday != 0 && md != matchday) continue;
        os << "\n  -- League phase, Matchday " << md << " --\n";
        for (const Match* m : s.matches)
            if (m->stage == Stage::LEAGUE && m->matchday == md) matchLine(os, s, *m);
    }
}

void teamFixtures(std::ostream& os, const Season& s, int team) {
    os << "\n  Fixtures & results - " << s.team(team).name << "\n";
    for (const Match* m : s.matches)
        if (m->home == team || m->away == team) matchLine(os, s, *m);
}

void drawSummary(std::ostream& os, const Season& s) {
    os << ui::banner("LEAGUE PHASE DRAW - 8 OPPONENTS PER TEAM (H = home, A = away)");
    for (const Team& t : s.teams) {
        os << "  " << padR(t.code + " (P" + std::to_string(t.pot) + ", " + t.country + ")", 15);
        for (int pot = 1; pot <= 4; ++pot) {
            for (const Match* m : s.matches) {
                if (m->stage != Stage::LEAGUE) continue;
                int opp = m->home == t.id ? m->away : (m->away == t.id ? m->home : -1);
                if (opp < 0 || s.team(opp).pot != pot) continue;
                os << (m->home == t.id ? "H:" : "A:") << s.team(opp).code << " ";
            }
            if (pot < 4) os << "| ";
        }
        os << "\n";
    }
    os << "  Rules: 2 opponents per pot (1 home, 1 away), no team from the same country,\n"
          "         max 2 opponents from any other single country.\n";
}

// ---------------------------------------------------------------------------
//  Match centre
// ---------------------------------------------------------------------------
void matchReport(std::ostream& os, const Season& s, const Match& m) {
    const Team& h = s.team(m.home);
    const Team& a = s.team(m.away);
    os << ui::banner(std::string(stageName(m.stage)) + (m.stage == Stage::LEAGUE ? " - Matchday " + std::to_string(m.matchday) : (m.leg ? " - Leg " + std::to_string(m.leg) : "")));
    os << "  " << scoreLine(s, m) << "\n";
    os << "  Venue: " << (m.neutral ? std::string("Puskas Arena, Budapest (neutral)") : h.stadium)
       << "   |   Match #" << m.id << "   |   " << modeName(m.mode) << "\n";
    if (m.stage != Stage::LEAGUE && m.leg == 2 && s.bracket.built()) os << "  Tie: " << s.tieSummary(m.tieNode) << "\n";
    if (!m.played) {
        elo::Outcome o = elo::predict(h.elo, a.elo, m.neutral);
        os << "\n  Not played yet. Elo forecast: " << h.code << ui::pct(o.home) << "  Draw" << ui::pct(o.draw)
           << "  " << a.code << ui::pct(o.away) << "   most likely " << o.likelyH << "-" << o.likelyA << "\n";
        return;
    }
    os << "\n  LINE-UPS\n";
    for (int i = 0; i < XI; ++i) {
        const Player& ph = h.squad[m.lineup[0].starters[i]];
        const Player& pa = a.squad[m.lineup[1].starters[i]];
        os << "  " << padL(std::to_string(ph.number), 3) << " " << positionName(ph.pos) << " " << padR(ph.name, 28)
           << padL(std::to_string(pa.number), 3) << " " << positionName(pa.pos) << " " << pa.name << "\n";
    }
    os << "  Bench: ";
    for (int i = 0; i < m.lineup[0].benchCount; ++i) os << h.squad[m.lineup[0].bench[i]].number << " ";
    os << " |  ";
    for (int i = 0; i < m.lineup[1].benchCount; ++i) os << a.squad[m.lineup[1].bench[i]].number << " ";
    os << "\n\n  TIMELINE\n";
    if (m.events.empty()) os << "  (no events)\n";
    int hs = 0, as = 0;
    for (const MatchEvent& e : m.events) {
        if (e.type == EventType::GOAL || e.type == EventType::PEN_GOAL) (e.side == 0 ? hs : as)++;
        if (e.type == EventType::OWN_GOAL) (e.side == 0 ? as : hs)++;
        os << "  " << eventText(s, m, e);
        if (e.type == EventType::GOAL || e.type == EventType::PEN_GOAL || e.type == EventType::OWN_GOAL)
            os << "   [" << hs << "-" << as << "]";
        os << "\n";
    }
    if (m.shootout) {
        os << "\n  PENALTY SHOOT-OUT\n";
        int ph = 0, pa = 0;
        for (const PenaltyKick& k : m.kicks) {
            if (k.scored) (k.side == 0 ? ph : pa)++;
            os << "    " << s.team(m.teamOf(k.side)).code << "  " << padR(pname(s, m, k.side, k.player), 32)
               << (k.scored ? "SCORED " : "missed ") << " [" << ph << "-" << pa << "]\n";
        }
    }
    if (m.abandoned)
        os << "\n  Match abandoned: " << s.team(m.teamOf(m.forfeitSide)).name
           << " were reduced to fewer than 7 players. Result awarded.\n";
}

// ---------------------------------------------------------------------------
//  Teams & players
// ---------------------------------------------------------------------------
void squad(std::ostream& os, const Season& s, int team) {
    const Team& t = s.team(team);
    os << ui::banner(t.name + "  (" + t.country + ", Pot " + std::to_string(t.pot) + ")");
    os << "  Stadium: " << t.stadium << "   Elo: " << ui::fixed(t.elo, 0)
       << " (start " << ui::fixed(t.startElo, 0) << ")   Best-XI rating: " << ui::fixed(t.bestXIRating(), 1) << "\n\n";
    os << "    #  Pos Name                          Rtg  Apps  Min  G  A  YC RC  CS  Status\n";
    os << "  " << ui::rule('-', 86) << "\n";
    for (int p = 0; p < 4; ++p) {
        for (const Player& pl : t.squad) {
            if (static_cast<int>(pl.pos) != p) continue;
            const PlayerStats& st = pl.stats;
            os << "  " << padL(std::to_string(pl.number), 3) << "  " << positionName(pl.pos) << "  "
               << padR(pl.name + (pl.generated ? "*" : ""), 29) << padL(std::to_string(pl.rating), 4)
               << padL(std::to_string(st.apps), 6) << padL(std::to_string(st.minutes), 5)
               << padL(std::to_string(st.goals), 3) << padL(std::to_string(st.assists), 3)
               << padL(std::to_string(st.yellows), 4) << padL(std::to_string(st.reds), 3)
               << padL(pl.pos == Position::GK ? std::to_string(st.cleanSheets) : "-", 4)
               << "  " << pl.statusText() << "\n";
        }
    }
    os << "  (* = academy player added by squad validation)\n";
}

void teamReport(std::ostream& os, const Season& s, int team) {
    squad(os, s, team);
    stats::TeamStat ts = stats::teamStat(s, team);
    os << "\n  Season record (all competitions): P" << ts.played << " W" << ts.won << " D" << ts.drawn
       << " L" << ts.lost << "  Goals " << ts.gf << "-" << ts.ga << "  Clean sheets " << ts.cleanSheets
       << "  Cards " << ts.yellows << "Y/" << ts.reds << "R  Elo change " << ui::signedNum(static_cast<int>(ts.eloChange)) << "\n";
    const Team& t = s.team(team);
    if (t.eloHistory.size() > 1) {
        os << "  Elo trend: ";
        for (int i = 0; i < t.eloHistory.size(); ++i) os << ui::fixed(t.eloHistory[i], 0) << (i + 1 < t.eloHistory.size() ? " > " : "\n");
    }
    teamFixtures(os, s, team);
}

void availability(std::ostream& os, const Season& s) {
    os << ui::banner("SUSPENSIONS & INJURIES");
    int count = 0;
    for (const Team& t : s.teams) {
        for (const Player& p : t.squad) {
            if (p.available() && !(p.yellowAccum == 2 || p.yellowAccum == 4 || p.yellowAccum == 6)) continue;
            os << "  " << padR(t.code, 5) << padR(p.name, 30) << padR(p.statusText(), 22) << p.injuryNote << "\n";
            ++count;
        }
    }
    if (!count) os << "  Everyone is available.\n";
    os << "  Rules: 3rd, 5th, 7th yellow = 1-match ban; second yellow = 1 match; straight red = 1-5.\n"
          "         Yellow cards are wiped after the quarter-finals. Unavailable players are\n"
          "         automatically blocked from every line-up.\n";
}

void playerRanking(std::ostream& os, const Season& s, stats::RankBy r, int n) {
    ds::DynArray<const Player*> top;
    stats::topN(s, r, n, top);       // heap-based leaderboard
    os << ui::banner(std::string(stats::rankByName(r)) + " - top " + std::to_string(n));
    if (top.empty()) { os << "  No data yet - play some matches first.\n"; return; }
    os << "  Rank  Player                        Club  Pos  Apps   Min   G   A  YC RC  Value\n";
    int rank = 0;
    for (int i = 0; i < top.size(); ++i) {
        const Player& p = *top[i];
        if (i == 0 || stats::metric(r, p) != stats::metric(r, *top[i - 1])) rank = i + 1;
        double v = stats::metric(r, p);
        bool decimal = r == stats::RankBy::IMPACT || r == stats::RankBy::GOALS_PER90;
        os << "  " << padL(std::to_string(rank), 4) << "  " << padR(p.name, 30) << padR(s.team(p.teamId).code, 6)
           << padR(positionName(p.pos), 4) << padL(std::to_string(p.stats.apps), 5)
           << padL(std::to_string(p.stats.minutes), 6) << padL(std::to_string(p.stats.goals), 4)
           << padL(std::to_string(p.stats.assists), 4) << padL(std::to_string(p.stats.yellows), 4)
           << padL(std::to_string(p.stats.reds), 3) << padL(decimal ? ui::fixed(v, 2) : ui::fixed(v, 0), 7) << "\n";
    }
}

void playerProfile(std::ostream& os, const Season& s, const Player& p) {
    const Team& t = s.team(p.teamId);
    const PlayerStats& st = p.stats;
    os << ui::banner(p.name);
    os << "  Club: " << t.name << "   #" << p.number << "   " << positionName(p.pos) << "   Rating " << p.rating << "\n";
    os << "  Status: " << p.statusText() << (p.injuryNote.empty() ? "" : " - " + p.injuryNote)
       << "   Yellow cards toward ban: " << p.yellowAccum << "\n\n";
    os << "  Appearances " << st.apps << " (" << st.starts << " starts, " << st.subApps << " as sub)   Minutes " << st.minutes << "\n";
    os << "  Goals " << st.goals << " (" << st.penGoals << " pens)   Assists " << st.assists
       << "   Own goals " << st.ownGoals << "   Penalties missed " << st.penMissed << "\n";
    os << "  Yellow " << st.yellows << "   Red " << st.reds << "   Injuries " << st.injuries
       << "   Matches missed " << st.matchesMissed;
    if (p.pos == Position::GK) os << "   Clean sheets " << st.cleanSheets;
    os << "\n";
    if (st.minutes > 0)
        os << "  Per 90: goals " << ui::fixed(st.goals * 90.0 / st.minutes, 2) << ", assists "
           << ui::fixed(st.assists * 90.0 / st.minutes, 2) << "   Impact index " << ui::fixed(stats::impactScore(p), 2) << "\n";
    // Rank among all players by goals (merge-sorted full ranking)
    ds::DynArray<const Player*> all;
    stats::rankAll(s, stats::RankBy::IMPACT, all);
    for (int i = 0; i < all.size(); ++i)
        if (all[i] == &p) { os << "  Impact rank: " << i + 1 << " of " << all.size() << " players who have played\n"; break; }
}

// ---------------------------------------------------------------------------
//  Season statistics
// ---------------------------------------------------------------------------
void seasonStats(std::ostream& os, const Season& s) {
    stats::Summary sm = stats::summarize(s);
    os << ui::banner("SEASON STATISTICS");
    if (sm.matches == 0) { os << "  No matches played yet.\n"; return; }
    os << "  Matches played: " << sm.matches << "   Goals: " << sm.goals << "\n";
    os << "  Goals per match: mean " << ui::fixed(sm.meanGoals, 2) << ", std-dev " << ui::fixed(sm.stdGoals, 2)
       << ", median " << ui::fixed(sm.medianGoals, 1) << "\n";
    os << "  Results: home wins " << sm.homeWins << " (" << ui::trim(ui::pct(double(sm.homeWins) / sm.matches)) << "), draws "
       << sm.draws << " (" << ui::trim(ui::pct(double(sm.draws) / sm.matches)) << "), away wins " << sm.awayWins
       << " (" << ui::trim(ui::pct(double(sm.awayWins) / sm.matches)) << ")\n";
    os << "  Yellow cards " << sm.yellows << ", red cards " << sm.reds << ", penalties scored " << sm.penalties
       << ", own goals " << sm.ownGoals << ", injuries " << sm.injuries << ", substitutions " << sm.subs << "\n";
    os << "  Extra time " << sm.extraTimes << " match(es), penalty shoot-outs " << sm.shootouts << "\n";
    if (sm.biggestWin >= 0) os << "  Biggest win:     " << scoreLine(s, s.match(sm.biggestWin)) << "\n";
    if (sm.highestScoring >= 0) os << "  Highest scoring: " << scoreLine(s, s.match(sm.highestScoring)) << "\n";

    int maxH = 1;
    for (int c : sm.histogram) if (c > maxH) maxH = c;
    os << "\n  Goals-per-match distribution\n";
    for (int g = 0; g <= 10; ++g) {
        if (sm.histogram[g] == 0 && g > 6) continue;
        os << "   " << padL(g == 10 ? "10+" : std::to_string(g), 3) << " | " << ui::bar(double(sm.histogram[g]) / maxH, 40)
           << " " << sm.histogram[g] << "\n";
    }
    int maxP = 1;
    for (int c : sm.goalsByPeriod) if (c > maxP) maxP = c;
    const char* periods[8] = {"1-15", "16-30", "31-45", "46-60", "61-75", "76-90", "91-105", "106-120"};
    os << "\n  When are goals scored?\n";
    for (int i = 0; i < 8; ++i) {
        if (i >= 6 && sm.goalsByPeriod[i] == 0) continue;
        os << "   " << padL(periods[i], 7) << " | " << ui::bar(double(sm.goalsByPeriod[i]) / maxP, 40) << " " << sm.goalsByPeriod[i] << "\n";
    }
}

void countryTable(std::ostream& os, const Season& s) {
    ds::DynArray<stats::CountryStat> cs;
    stats::countryStats(s, cs);
    os << ui::banner("PERFORMANCE BY COUNTRY (hash-table aggregation)");
    os << "  Country  Clubs    P    W    D    L    GF   GA   Pts/game\n";
    for (const stats::CountryStat& c : cs)
        os << "  " << padR(c.country, 8) << padL(std::to_string(c.teams), 5) << padL(std::to_string(c.played), 5)
           << padL(std::to_string(c.won), 5) << padL(std::to_string(c.drawn), 5) << padL(std::to_string(c.lost), 5)
           << padL(std::to_string(c.gf), 6) << padL(std::to_string(c.ga), 5) << padL(ui::fixed(c.ppg(), 2), 10) << "\n";
}

void eloTable(std::ostream& os, const Season& s) {
    ds::DynArray<int> ids;
    for (const Team& t : s.teams) ids.push(t.id);
    ds::mergeSort(ids, [&s](int a, int b) { return s.team(a).elo > s.team(b).elo; });
    os << ui::banner("ELO POWER RANKING");
    os << "  Rank  Club                         Elo    Start   Change\n";
    for (int i = 0; i < ids.size(); ++i) {
        const Team& t = s.team(ids[i]);
        int change = static_cast<int>(t.elo - t.startElo + (t.elo >= t.startElo ? 0.5 : -0.5));
        os << "  " << padL(std::to_string(i + 1), 4) << "  " << padR(t.name, 27) << padL(ui::fixed(t.elo, 0), 6)
           << padL(ui::fixed(t.startElo, 0), 8) << padL(ui::signedNum(change), 9) << "\n";
    }
}

// ---------------------------------------------------------------------------
//  Knockout bracket (tree)
// ---------------------------------------------------------------------------
namespace {
std::string nodeLabel(const Season& s, int n) {
    const BracketNode& node = s.bracket.node(n);
    if (node.leaf) {
        const Team& t = s.team(node.team);
        return t.name + " (#" + std::to_string(s.bracket.rankOf(node.team) + 1) + ")";
    }
    return std::string("[") + stageShort(node.stage) + "] " + s.tieSummary(n);
}
void printTree(std::ostream& os, const Season& s, int n, const std::string& prefix, bool last, bool root) {
    os << "  " << prefix << (root ? "" : (last ? "`-- " : "|-- ")) << nodeLabel(s, n) << "\n";
    const BracketNode& node = s.bracket.node(n);
    if (node.leaf) return;
    std::string childPrefix = root ? "" : prefix + (last ? "    " : "|   ");
    printTree(os, s, node.left, childPrefix, false, false);
    printTree(os, s, node.right, childPrefix, true, false);
}
}  // namespace

void bracketTree(std::ostream& os, const Season& s) {
    os << ui::banner("KNOCKOUT BRACKET (tree view)");
    if (!s.bracket.built()) {
        os << "  The bracket is drawn when the league phase ends.\n"
              "  Current projection if the league ended today:\n\n";
        ds::DynArray<int> r = s.currentRanking();
        os << "  Byes to R16 (1-8):  ";
        for (int i = 0; i < 8; ++i) os << s.team(r[i]).code << " ";
        os << "\n  Play-off pairs:     ";
        for (int k = 0; k < 4; ++k)
            os << s.team(r[8 + 2 * k]).code << "/" << s.team(r[9 + 2 * k]).code << " v "
               << s.team(r[22 - 2 * k]).code << "/" << s.team(r[23 - 2 * k]).code << "  ";
        os << "\n";
        return;
    }
    printTree(os, s, s.bracket.root(), "", true, true);
    os << "\n  Tree depth: " << s.bracket.depth() << " levels. Top-8 teams enter at R16 (bye).\n";
}

void bracketRounds(std::ostream& os, const Season& s) {
    os << ui::banner("KNOCKOUT ROUNDS");
    if (!s.bracket.built()) { os << "  Not drawn yet.\n"; return; }
    const Stage stages[5] = {Stage::PLAYOFF, Stage::R16, Stage::QF, Stage::SF, Stage::FINAL};
    for (Stage st : stages) {
        ds::DynArray<int> ties;
        s.bracket.tiesAtStage(st, ties);
        os << "\n  " << stageName(st) << "\n";
        for (int t : ties) os << "    " << s.tieSummary(t) << "\n";
    }
    int c = s.champion();
    if (c >= 0) os << "\n  CHAMPIONS: " << s.team(c).name << "\n";
}

// ---------------------------------------------------------------------------
//  Predictions
// ---------------------------------------------------------------------------
void prediction(std::ostream& os, const Season& s, int home, int away, bool neutral) {
    const Team& h = s.team(home);
    const Team& a = s.team(away);
    elo::Outcome o = elo::predict(h.elo, a.elo, neutral);
    os << ui::banner("ELO MATCH PREDICTION");
    os << "  " << h.name << " (Elo " << ui::fixed(h.elo, 0) << ")  vs  " << a.name << " (Elo " << ui::fixed(a.elo, 0) << ")"
       << (neutral ? "  [neutral venue]" : "  [at " + h.stadium + "]") << "\n\n";
    os << "  " << padR(h.code + " win", 10) << ui::pct(o.home) << "  " << ui::bar(o.home, 40) << "\n";
    os << "  " << padR("Draw", 10) << ui::pct(o.draw) << "  " << ui::bar(o.draw, 40) << "\n";
    os << "  " << padR(a.code + " win", 10) << ui::pct(o.away) << "  " << ui::bar(o.away, 40) << "\n\n";
    os << "  Expected goals: " << ui::fixed(o.xgHome, 2) << " - " << ui::fixed(o.xgAway, 2)
       << "   Most likely score: " << o.likelyH << "-" << o.likelyA << " (" << ui::trim(ui::pct(o.likelyP)) << ")\n";
    os << "  Over 2.5 goals:" << ui::pct(o.over25) << "   Both teams score:" << ui::pct(o.bothScore) << "\n";
    os << "  Elo win expectancy (incl. draws as half): " << ui::trim(ui::pct(elo::expectedScore(h.elo, a.elo, neutral))) << "\n";
}

void monteCarlo(std::ostream& os, const Season& s, const MCResult& r, const MCResult* before) {
    ds::DynArray<int> ids;
    for (const Team& t : s.teams) ids.push(t.id);
    ds::mergeSort(ids, [&r](int a, int b) {
        if (r.win[a] != r.win[b]) return r.win[a] > r.win[b];
        return r.avgRank[a] < r.avgRank[b];
    });
    os << ui::banner("MONTE-CARLO PROJECTIONS (" + std::to_string(r.iterations) + " simulated tournaments)");
    os << "  Club                     AvgPts AvgPos  Top8   PO    Out    R16    QF     SF   Final   WIN";
    if (before) os << "   (chg)";
    os << "\n";
    for (int t : ids) {
        os << "  " << padR(s.team(t).name, 24) << padL(ui::fixed(r.avgPts[t], 1), 6) << padL(ui::fixed(r.avgRank[t], 1), 7)
           << ui::pct(r.top8[t]) << ui::pct(r.playoff[t]) << ui::pct(r.out[t]) << ui::pct(r.r16[t]) << ui::pct(r.qf[t])
           << ui::pct(r.sf[t]) << ui::pct(r.final_[t]) << ui::pct(r.win[t]);
        if (before) {
            double d = (r.win[t] - before->win[t]) * 100.0;
            os << "  " << (d >= 0 ? "+" : "") << ui::fixed(d, 1);
        }
        os << "\n";
    }
    os << "  Top8 = direct to R16, PO = finish 9th-24th, Out = finish 25th-36th (eliminated).\n";
}

void dataStructures(std::ostream& os, const Season& s) {
    os << ui::banner("UNDER THE HOOD - CUSTOM DATA STRUCTURES");
    os << "  HashMap  teamIndex   : " << s.teamIndex.size() << " keys, " << s.teamIndex.bucketCount()
       << " buckets, load " << ui::fixed(s.teamIndex.loadFactor(), 2) << ", longest chain " << s.teamIndex.longestChain() << "\n";
    os << "  HashMap  playerIndex : " << s.playerIndex.size() << " keys, " << s.playerIndex.bucketCount()
       << " buckets, load " << ui::fixed(s.playerIndex.loadFactor(), 2) << ", longest chain " << s.playerIndex.longestChain() << "\n";
    os << "  Queue    fixtures    : " << s.pending.size() << " match(es) waiting to be played\n";
    os << "  DynArray matches     : " << s.matches.size() << " created, " << s.playOrder.size() << " played\n";
    int events = 0;
    for (const Match* m : s.matches) events += m->events.size();
    os << "  LinkedList events    : " << events << " match events stored across all matches\n";
    os << "  Tree     bracket     : " << (s.bracket.built() ? std::to_string(s.bracket.depth()) + " levels" : std::string("not built yet")) << "\n";
    os << "  Heap     leaderboards: Top-N players selected with a bounded min-heap\n";
    os << "  MergeSort            : league table, rankings, schedule minutes (stable sort)\n";
    os << "  Graph                : league draw = bipartite matchings; matchdays = perfect matchings\n";
}

void fullSeasonReport(std::ostream& os, const Season& s) {
    os << ui::banner("UEFA CHAMPIONS LEAGUE SIMULATOR - SEASON REPORT");
    os << "  Seed: " << s.seed << "   Stage: " << phaseName(s.phase) << "   Matches played: " << s.playedCount()
       << " / " << s.matches.size() << " created\n";
    int c = s.champion();
    if (c >= 0) os << "  CHAMPIONS: " << s.team(c).name << "\n";
    os << "\n";
    table(os, s);
    os << "\n";
    if (features::KNOCKOUT_BRACKET) {
        bracketRounds(os, s);
        os << "\n";
    } else {
        os << ui::banner("KNOCKOUT RESULTS");
        fixtures(os, s, -1);
        os << "\n";
    }
    if (features::PLAYER_STATS) {
        playerRanking(os, s, stats::RankBy::GOALS, 15);
        os << "\n";
        playerRanking(os, s, stats::RankBy::ASSISTS, 10);
        os << "\n";
        playerRanking(os, s, stats::RankBy::IMPACT, 10);
        os << "\n";
        playerRanking(os, s, stats::RankBy::CLEAN_SHEETS, 5);
        os << "\n";
    }
    seasonStats(os, s);
    os << "\n";
    countryTable(os, s);
    os << "\n";
    eloTable(os, s);
}

}  // namespace reports
