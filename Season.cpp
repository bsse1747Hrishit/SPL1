#include "Season.h"
#include "LeagueDraw.h"
#include "Elo.h"

namespace {
std::string lower(const std::string& s) {
    std::string r = s;
    for (char& c : r) if (c >= 'A' && c <= 'Z') c = static_cast<char>(c - 'A' + 'a');
    return r;
}
Stage stageOfPhase(Phase p) {
    switch (p) {
        case Phase::PLAYOFF: return Stage::PLAYOFF;
        case Phase::R16: return Stage::R16;
        case Phase::QF: return Stage::QF;
        case Phase::SF: return Stage::SF;
        case Phase::FINAL: return Stage::FINAL;
        default: return Stage::LEAGUE;
    }
}
const unsigned long long KO_SEED_MULT = 0x9E3779B97F4A7C15ULL;
}  // namespace

const char* phaseName(Phase p) {
    switch (p) {
        case Phase::LEAGUE: return "League Phase";
        case Phase::PLAYOFF: return "Knockout Play-offs";
        case Phase::R16: return "Round of 16";
        case Phase::QF: return "Quarter-finals";
        case Phase::SF: return "Semi-finals";
        case Phase::FINAL: return "Final";
        case Phase::COMPLETE: return "Season complete";
    }
    return "?";
}

Season::~Season() {
    for (Match* m : matches) delete m;
}

// ---------------------------------------------------------------------------
//  Creation: teams, draw, schedule, fixture queue
// ---------------------------------------------------------------------------
bool Season::create(unsigned long long s, std::string& log) {
    seed = s;
    if (!loadTeamDatabase(teams, log)) return false;
    rebuildIndexes();
    table.reset(teams.size());
    bracket.clear();

    ds::Random drawRng(seed);
    ds::DynArray<FixturePair> pairs;
    if (!LeagueDraw::draw(teams, drawRng, pairs, log)) return false;
    ds::DynArray<int> md;
    if (!LeagueDraw::schedule(teams.size(), pairs, drawRng, md, log)) return false;

    // Create matches ordered by matchday so ids follow the calendar
    for (int day = 0; day < LeagueDraw::MATCHDAYS; ++day)
        for (int i = 0; i < pairs.size(); ++i)
            if (md[i] == day) createMatch(Stage::LEAGUE, 0, day + 1, pairs[i].home, pairs[i].away, false, -1);

    rng.setSeed(seed * 7919ULL + 17ULL);
    phase = Phase::LEAGUE;
    return true;
}

void Season::rebuildIndexes() {
    teamIndex.clear();
    playerIndex.clear();
    for (Team& t : teams) {
        teamIndex.put(t.code, t.id);
        teamIndex.put(lower(t.code), t.id);
        teamIndex.put(lower(t.name), t.id);
        t.eloHistory.clear();
        t.eloHistory.push(t.elo);
        for (Player& p : t.squad) playerIndex.put(lower(p.name), p.id);
    }
}

int Season::createMatch(Stage s, int leg, int md, int home, int away, bool neutral, int tieNode) {
    Match* m = new Match();
    m->id = matches.size();
    m->stage = s;
    m->leg = leg;
    m->matchday = md;
    m->home = home;
    m->away = away;
    m->neutral = neutral;
    m->tieNode = tieNode;
    matches.push(m);
    pending.enqueue(m->id);
    return m->id;
}

int Season::findTeam(const std::string& text) const {
    const int* id = teamIndex.find(lower(text));
    if (!id) id = teamIndex.find(text);
    if (id) return *id;
    std::string q = lower(text);
    if (q.empty()) return -1;
    for (const Team& t : teams)
        if (lower(t.name).find(q) != std::string::npos) return t.id;
    return -1;
}

void Season::findPlayers(const std::string& text, ds::DynArray<int>& out) const {
    out.clear();
    std::string q = lower(text);
    if (q.empty()) return;
    const int* exact = playerIndex.find(q);       // O(1) hash lookup first
    if (exact) { out.push(*exact); return; }
    for (const Team& t : teams)
        for (const Player& p : t.squad)
            if (lower(p.name).find(q) != std::string::npos) out.push(p.id);
}

Match* Season::nextMatch() {
    if (pending.empty()) return nullptr;
    return matches[pending.front()];
}

int Season::currentMatchday() const {
    if (pending.empty()) return 0;
    return matches[pending.front()]->matchday;
}

int Season::champion() const {
    if (!bracket.built()) return -1;
    return bracket.node(bracket.root()).winner;
}

// ---------------------------------------------------------------------------
//  Knockout helpers
// ---------------------------------------------------------------------------
int Season::tieGoals(int tieNode, int teamId) const {
    const BracketNode& n = bracket.node(tieNode);
    int g = 0;
    if (n.leg1 >= 0 && matches[n.leg1]->played) g += matches[n.leg1]->goalsOf(teamId);
    if (n.leg2 >= 0 && matches[n.leg2]->played) g += matches[n.leg2]->goalsOf(teamId);
    return g;
}

std::string Season::tieSummary(int tieNode) const {
    const BracketNode& n = bracket.node(tieNode);
    int a = bracket.participant(tieNode, 0), b = bracket.participant(tieNode, 1);
    std::string an = a >= 0 ? teams[a].name : "TBD";
    std::string bn = b >= 0 ? teams[b].name : "TBD";
    if (a < 0 || b < 0 || n.leg1 < 0) return an + " v " + bn;
    std::string s = an + " " + std::to_string(tieGoals(tieNode, a)) + "-" +
                    std::to_string(tieGoals(tieNode, b)) + " " + bn;
    if (n.stage != Stage::FINAL) s += " (agg)";
    if (n.winner >= 0) s += "  -> " + teams[n.winner].name;
    return s;
}

bool Season::needsDecider(const MatchRecorder& rec) const {
    const Match& m = rec.match();
    if (m.stage == Stage::LEAGUE || m.leg == 1 || rec.isAbandoned()) return false;
    if (m.leg == 0) return rec.score(0) == rec.score(1);        // final
    // second leg: compare aggregate
    const Match& first = *matches[bracket.node(m.tieNode).leg1];
    int aggHome = first.goalsOf(m.home) + rec.score(0);
    int aggAway = first.goalsOf(m.away) + rec.score(1);
    return aggHome == aggAway;
}

void Season::autoComplete(MatchRecorder& rec, MatchSimulator& sim) {
    std::string err;
    if (!rec.inExtraTime()) sim.simulateTo(rec, 90);
    if (rec.isAbandoned()) return;
    if (!rec.inExtraTime() && needsDecider(rec)) rec.startExtraTime(err);
    if (rec.inExtraTime() && !rec.inShootout()) sim.simulateTo(rec, 120);
    if (rec.isAbandoned()) return;
    if (rec.inExtraTime() && (rec.inShootout() || needsDecider(rec))) sim.simulateShootout(rec);
}

// UEFA allows emergency registrations: a replacement goalkeeper when no
// keeper is available, and extra players when too few are fit. Deterministic,
// so save-file replays reproduce the same registrations.
void Season::prepareMatch(Match& m) {
    const int MIN_AVAILABLE = 13;
    for (int side = 0; side < 2; ++side) {
        Team& t = team(m.teamOf(side));
        auto registerPlayer = [&](Position pos) {
            if (t.squad.size() >= MAX_SQUAD) return false;
            Player p;
            p.pos = pos;
            p.rating = 60;
            p.generated = true;
            p.name = t.code + " Emergency " + positionName(pos) + " " + std::to_string(t.squad.size() + 1);
            for (int n = 99; n >= 1; --n) if (t.indexByNumber(n) < 0) { p.number = n; break; }
            p.teamId = t.id;
            p.squadIndex = t.squad.size();
            p.id = t.id * 100 + p.squadIndex;
            t.squad.push(p);
            playerIndex.put(lower(p.name), p.id);
            playerNews.push(t.name + " register " + p.name + " (emergency registration)");
            return true;
        };
        int keepers = 0;
        for (const Player& p : t.squad) if (p.available() && p.pos == Position::GK) ++keepers;
        if (keepers == 0) registerPlayer(Position::GK);
        while (t.availableCount() < MIN_AVAILABLE && registerPlayer(Position::MF)) {}
    }
}

bool Season::playAuto(Match& m) {
    prepareMatch(m);
    MatchRecorder rec(m, team(m.home), team(m.away));
    std::string err;
    Lineup lh = MatchSimulator::autoLineup(team(m.home));
    Lineup la = MatchSimulator::autoLineup(team(m.away));
    if (!rec.start(lh, la, err)) {
        news.push("Line-up error: " + err);
        return false;
    }
    MatchSimulator sim(rng);
    autoComplete(rec, sim);
    rec.finish();
    m.mode = PlayMode::SIMULATED;
    commitMatch(m, rec);
    return true;
}

// ---------------------------------------------------------------------------
//  Commit: the single place where a finished match changes the season
// ---------------------------------------------------------------------------
void Season::commitMatch(Match& m, MatchRecorder& rec) {
    if (!pending.empty() && pending.front() == m.id) pending.dequeue();
    Team* T[2] = {&team(m.home), &team(m.away)};

    // 1) Serve bans / injuries of players who were unavailable at kick-off
    for (int s = 0; s < 2; ++s) {
        for (Player& p : T[s]->squad) {
            if (p.suspended > 0) { p.suspended--; p.stats.matchesMissed++; }
            else if (p.injured > 0) { p.stats.matchesMissed++; }
            if (p.injured > 0) {
                p.injured--;
                if (p.injured == 0) p.injuryNote.clear();
            }
        }
    }

    // 2) Appearances, minutes, clean sheets
    int conceded[2] = {m.ag, m.hg};
    for (int s = 0; s < 2; ++s) {
        for (int i = 0; i < T[s]->squad.size(); ++i) {
            int in = rec.entered(s, i);
            if (in < 0) continue;
            Player& p = T[s]->squad[i];
            int mins = rec.exited(s, i) - in;
            if (mins < 0) mins = 0;
            p.stats.apps++;
            p.stats.minutes += mins;
            if (rec.wasStarter(s, i)) p.stats.starts++; else p.stats.subApps++;
            if (p.pos == Position::GK && conceded[s] == 0 && mins >= 60) p.stats.cleanSheets++;
        }
    }

    // 3) Events: goals, assists, cards (accumulation bans), injuries
    bool banFromThisMatch[2][MatchRecorder::MAXP] = {{false}};
    for (const MatchEvent& e : m.events) {
        Player& p = T[e.side]->squad[e.player];
        switch (e.type) {
            case EventType::GOAL:
                p.stats.goals++;
                if (e.player2 >= 0) T[e.side]->squad[e.player2].stats.assists++;
                break;
            case EventType::PEN_GOAL:
                p.stats.goals++;
                p.stats.penGoals++;
                break;
            case EventType::OWN_GOAL: p.stats.ownGoals++; break;
            case EventType::PEN_MISS: p.stats.penMissed++; break;
            case EventType::YELLOW:
                p.stats.yellows++;
                p.yellowAccum++;
                // UEFA: ban after the 3rd, 5th, 7th ... caution
                if (p.yellowAccum >= 3 && p.yellowAccum % 2 == 1) {
                    p.suspended++;
                    banFromThisMatch[e.side][e.player] = true;
                    playerNews.push(p.name + " (" + T[e.side]->code + ") is suspended: yellow-card accumulation");
                }
                break;
            case EventType::SECOND_YELLOW:
                // the first caution of this match is cancelled; 1-match ban for the red
                p.stats.yellows++;
                p.stats.reds++;
                p.yellowAccum--;
                if (banFromThisMatch[e.side][e.player]) p.suspended--;
                p.suspended++;
                playerNews.push(p.name + " (" + T[e.side]->code + ") sent off - suspended for 1 match");
                break;
            case EventType::RED:
                p.stats.reds++;
                p.suspended += e.extra;
                playerNews.push(p.name + " (" + T[e.side]->code + ") red card - banned for " + std::to_string(e.extra) + " match(es)");
                break;
            case EventType::INJURY:
                p.stats.injuries++;
                if (e.extra > p.injured) p.injured = e.extra;
                p.injuryNote = "injured " + std::to_string(e.minute) + "' v " + T[1 - e.side]->code;
                playerNews.push(p.name + " (" + T[e.side]->code + ") injured - out for " + std::to_string(e.extra) + " match(es)");
                break;
            case EventType::SUB: break;
        }
    }

    // 4) League table
    if (m.stage == Stage::LEAGUE) table.record(m.home, m.away, m.hg, m.ag);

    // 5) Elo ratings
    elo::update(T[0]->elo, T[1]->elo, m.hg, m.ag, m.neutral);
    T[0]->eloHistory.push(T[0]->elo);
    T[1]->eloHistory.push(T[1]->elo);

    m.played = true;
    playOrder.push(m.id);

    // 6) Knockout tie resolution
    if (m.stage != Stage::LEAGUE && m.leg != 1) resolveTie(m);

    advance();
}

void Season::resolveTie(Match& m) {
    BracketNode& n = bracket.node(m.tieNode);
    int a = m.home, b = m.away;
    int winner;
    if (m.abandoned) {
        winner = m.forfeitSide == 0 ? b : a;
    } else {
        int ga = tieGoals(m.tieNode, a), gb = tieGoals(m.tieNode, b);
        if (ga != gb) winner = ga > gb ? a : b;
        else winner = m.penH > m.penA ? a : b;     // shoot-out (home side = a)
    }
    n.winner = winner;
}

void Season::resetYellowCards() {
    for (Team& t : teams)
        for (Player& p : t.squad) p.yellowAccum = 0;
    news.push("Yellow cards wiped clean after the quarter-finals (UEFA rule). Pending bans still apply.");
}

// ---------------------------------------------------------------------------
//  Phase transitions
// ---------------------------------------------------------------------------
void Season::startKnockoutRound(Phase p) {
    Stage s = stageOfPhase(p);
    ds::DynArray<int> ties;
    bracket.tiesAtStage(s, ties);
    int round = 8 + static_cast<int>(p);
    if (s == Stage::FINAL) {
        int t = ties[0];
        int a = bracket.participant(t, 0), b = bracket.participant(t, 1);
        bracket.node(t).leg1 = createMatch(s, 0, round, a, b, true, t);
        return;
    }
    // first legs (worse-ranked team at home), then second legs
    for (int t : ties) {
        int a = bracket.participant(t, 0), b = bracket.participant(t, 1);
        int better = bracket.betterRanked(a, b), worse = better == a ? b : a;
        bracket.node(t).leg1 = createMatch(s, 1, round, worse, better, false, t);
    }
    for (int t : ties) {
        int a = bracket.participant(t, 0), b = bracket.participant(t, 1);
        int better = bracket.betterRanked(a, b), worse = better == a ? b : a;
        bracket.node(t).leg2 = createMatch(s, 2, round, better, worse, false, t);
    }
}

void Season::advance() {
    if (!pending.empty() || phase == Phase::COMPLETE) return;
    if (phase == Phase::LEAGUE) {
        ds::DynArray<int> r = table.ranking(teams);
        for (int i = 0; i < r.size(); ++i) leagueRanking[i] = r[i];
        leagueDone = true;
        ds::Random koRng(seed * KO_SEED_MULT + 1);
        bracket.build(leagueRanking, teams.size(), koRng);
        phase = Phase::PLAYOFF;
        news.push("League phase complete! Top 8 go straight to the Round of 16; 9th-24th enter the play-offs; 25th-36th are eliminated.");
        news.push("Knockout bracket drawn.");
        startKnockoutRound(phase);
        return;
    }
    if (phase == Phase::FINAL) {
        phase = Phase::COMPLETE;
        int c = champion();
        if (c >= 0) news.push(teams[c].name + " are the CHAMPIONS OF EUROPE!");
        return;
    }
    if (phase == Phase::QF) resetYellowCards();
    phase = static_cast<Phase>(static_cast<int>(phase) + 1);
    news.push(std::string(phaseName(phase)) + " begin.");
    startKnockoutRound(phase);
}
