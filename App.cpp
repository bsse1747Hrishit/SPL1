#include "App.h"
#include <ctime>
#include <fstream>
#include <iostream>
#include <sstream>
#include "Features.h"
#include "SaveLoad.h"
#include "Elo.h"
#include "MonteCarlo.h"
#include "Reports.h"
#include "Season.h"
#include "Console.h"
#include "LiveConsole.h"

#ifdef _WIN32
#include <direct.h>
#define MAKE_DIR(p) _mkdir(p)
#else
#include <sys/stat.h>
#define MAKE_DIR(p) mkdir(p, 0755)
#endif

using std::cout;
using namespace ui;

App::~App() { delete s_; }

int App::run() {
    cout << banner("UEFA CHAMPIONS LEAGUE  -  SIMULATOR & ANALYTICS");
    cout << "  36 clubs | Swiss-model league phase | knockout bracket | Elo & Monte-Carlo\n"
            "  Simulate matches, record them live event-by-event, or enter quick scores.\n\n";
    try {
        if (!startMenu()) return 0;
        mainMenu();
    } catch (const InputClosed&) {
        cout << "\n  Input closed - goodbye!\n";
    }
    return 0;
}

bool App::newSeason(unsigned long long seed) {
    Season* fresh = new Season();
    std::string log;
    cout << "  Validating squads and drawing fixtures (seed " << seed << ")...\n";
    if (!fresh->create(seed, log)) {
        cout << log << "  Could not create the season.\n";
        delete fresh;
        return false;
    }
    cout << log;
    delete s_;
    s_ = fresh;
    return true;
}

bool App::startMenu() {
    for (;;) {
        cout << "  1) Start a new season\n  2) Load a saved season\n  0) Quit\n";
        int c = askInt("  Choice: ", 0, 2, -1);
        if (c == 0) return false;
        if (c == 2) {
            loadMenu();
            if (s_) return true;
            continue;
        }
        std::string in = ask("  Random seed (number, blank = random): ");
        int seedVal;
        unsigned long long seed = parseInt(in, seedVal) && seedVal > 0 ? static_cast<unsigned long long>(seedVal)
                                                                         : static_cast<unsigned long long>(std::time(nullptr)) % 1000000;
        if (!newSeason(seed)) continue;
        if (askYesNo("  Show the league-phase draw now?", false)) reports::drawSummary(cout, *s_);
        return true;
    }
}

void App::header() {
    cout << "\n" << rule('=') << "\n";
    cout << "  " << phaseName(s_->phase);
    Match* next = s_->nextMatch();
    if (next) {
        if (next->stage == Stage::LEAGUE) cout << "  -  Matchday " << next->matchday << " of 8";
        else if (next->leg) cout << "  -  Leg " << next->leg;
        cout << "   |   Next: " << s_->team(next->home).name << " v " << s_->team(next->away).name;
    } else if (s_->champion() >= 0) {
        cout << "   |   Champions: " << s_->team(s_->champion()).name;
    }
    cout << "\n  Played " << s_->playedCount() << " matches   |   seed " << s_->seed << "\n" << rule('=') << "\n";
}

void App::flushNews(bool playerNews) {
    if (playerNews) for (const std::string& n : s_->playerNews) cout << "  - " << n << "\n";
    else if (!s_->playerNews.empty())
        cout << "  - " << s_->playerNews.size() << " new suspensions/injuries (see Teams > Suspensions & injuries)\n";
    s_->playerNews.clear();
    for (const std::string& n : s_->news) cout << "  * " << n << "\n";
    s_->news.clear();
}

void App::mainMenu() {
    // Each entry: menu text + action code. Hidden features are left out and
    // the remaining entries are numbered 1, 2, 3 ... automatically.
    enum Action { PLAY_NEXT, PLAY_ROUND, PLAY_LEAGUE, PLAY_SEASON, TABLE, FIXTURES, MATCH_CENTRE, TEAMS,
                  PLAYERS, BRACKET, PREDICT, STATS, REPORTS, SAVE, LOAD, SETTINGS };
    struct Item { const char* label; Action action; bool enabled; };
    const Item all[] = {
        {"Play next match (simulate / live record / quick score)", PLAY_NEXT, true},
        {"Simulate the rest of this matchday / round", PLAY_ROUND, true},
        {"Simulate to the end of the league phase", PLAY_LEAGUE, true},
        {"Simulate the whole remaining season", PLAY_SEASON, true},
        {"League table", TABLE, true},
        {"Fixtures & results", FIXTURES, true},
        {"Match centre (full match report)", MATCH_CENTRE, features::MATCH_CENTRE},
        {"Teams & squads", TEAMS, true},
        {"Player statistics & rankings", PLAYERS, features::PLAYER_STATS},
        {"Knockout bracket", BRACKET, features::KNOCKOUT_BRACKET},
        {"Predictions (Elo, Monte-Carlo, what-if, prediction game)", PREDICT, features::PREDICTIONS},
        {"Season statistics & analytics", STATS, true},
        {"Export reports to files", REPORTS, true},
        {"Save season", SAVE, true},
        {"Load season", LOAD, true},
        {"Settings", SETTINGS, true},
    };
    ds::DynArray<Item> menu;
    for (const Item& it : all) if (it.enabled) menu.push(it);

    for (;;) {
        header();
        for (int i = 0; i < menu.size(); ++i) cout << padL(std::to_string(i + 1), 4) << ") " << menu[i].label << "\n";
        cout << "   0) Exit\n";
        int c = askInt("  Choice: ", 0, menu.size(), -1);
        if (c == 0) {
            if (askYesNo("  Save before exiting?", false)) saveMenu();
            cout << "  Goodbye!\n";
            return;
        }
        switch (menu[c - 1].action) {
            case PLAY_NEXT: playNext(); break;
            case PLAY_ROUND: playMatchday(); break;
            case PLAY_LEAGUE: playUntil(static_cast<int>(Phase::PLAYOFF), true); break;
            case PLAY_SEASON: playUntil(static_cast<int>(Phase::COMPLETE), false); break;
            case TABLE: reports::table(cout, *s_); break;
            case FIXTURES: fixturesMenu(); break;
            case MATCH_CENTRE: matchCentre(); break;
            case TEAMS: teamsMenu(); break;
            case PLAYERS: playersMenu(); break;
            case BRACKET: bracketMenu(); break;
            case PREDICT: predictionsMenu(); break;
            case STATS: statsMenu(); break;
            case REPORTS: reportsMenu(); break;
            case SAVE: saveMenu(); break;
            case LOAD: loadMenu(); break;
            case SETTINGS: settingsMenu(); break;
        }
    }
}

// ---------------------------------------------------------------------------
//  Playing
// ---------------------------------------------------------------------------
void App::showAfterMatch(int matchId, bool showTable) {
    const Match& m = s_->match(matchId);
    cout << "\n  FULL TIME: " << reports::scoreLine(*s_, m) << "\n";
    for (const MatchEvent& e : m.events)
        if (e.type != EventType::SUB) cout << "   " << reports::eventText(*s_, m, e) << "\n";
    if (m.shootout) cout << "   Penalties: " << m.penH << "-" << m.penA << "\n";
    if (m.stage != Stage::LEAGUE && m.leg != 1) cout << "  Tie: " << s_->tieSummary(m.tieNode) << "\n";
    flushNews();
    if (showTable && m.stage == Stage::LEAGUE && s_->settings.tableMode == 1) {
        cout << "\n";
        reports::table(cout, *s_, m.home, m.away);
    }
}

void App::playNext() {
    Match* m = s_->nextMatch();
    if (!m) { cout << "  The season is complete!\n"; return; }
    cout << "\n  NEXT MATCH: " << s_->team(m->home).name << " v " << s_->team(m->away).name << "  ("
         << stageName(m->stage) << (m->neutral ? ", neutral venue" : ", at " + s_->team(m->home).stadium) << ")\n";
    if (features::PREDICTIONS) reports::prediction(cout, *s_, m->home, m->away, m->neutral);
    if (m->stage != Stage::LEAGUE && m->leg == 2) cout << "  First leg: " << reports::scoreLine(*s_, s_->match(s_->bracket.node(m->tieNode).leg1)) << "\n";
    cout << "\n  How should this match be played?\n"
            "   1) Simulate it (minute-by-minute engine)\n"
            "   2) LIVE recording - enter goals, cards, subs yourself (rules enforced)\n"
            "   3) Quick score - type the result, details are generated\n"
            "   0) Back\n";
    int c = askInt("  Choice [1]: ", 0, 3, 1);
    if (c == 0) return;
    int id = m->id;
    if (c == 1) { if (!s_->playAuto(*m)) { flushNews(); return; } }
    else if (c == 2) { if (!runLiveMatch(*s_, *m)) return; }
    else { if (!runQuickScore(*s_, *m)) return; }
    showAfterMatch(id, true);
}

void App::playMatchday() {
    Match* m = s_->nextMatch();
    if (!m) { cout << "  The season is complete!\n"; return; }
    Stage stage = m->stage;
    int md = m->matchday, leg = m->leg;
    cout << "\n  Results:\n";
    while ((m = s_->nextMatch()) && m->stage == stage && m->matchday == md && m->leg == leg) {
        int id = m->id;
        if (!s_->playAuto(*m)) { flushNews(); return; }
        reports::matchLine(cout, *s_, s_->match(id));
        const Match& done = s_->match(id);
        if (done.stage != Stage::LEAGUE && done.leg != 1) cout << "        Tie: " << s_->tieSummary(done.tieNode) << "\n";
    }
    flushNews(false);
    if (stage == Stage::LEAGUE && s_->settings.tableMode != 0) reports::table(cout, *s_);
    else if (stage != Stage::LEAGUE && features::KNOCKOUT_BRACKET) reports::bracketRounds(cout, *s_);
}

void App::playUntil(int phaseLimit, bool showEachMatchday) {
    if (static_cast<int>(s_->phase) >= phaseLimit) { cout << "  Already there.\n"; return; }
    int lastMd = -1;
    while (static_cast<int>(s_->phase) < phaseLimit) {
        Match* m = s_->nextMatch();
        if (!m) break;
        if (showEachMatchday && m->stage == Stage::LEAGUE && m->matchday != lastMd) {
            lastMd = m->matchday;
            cout << "  Matchday " << lastMd << " ...\n";
        }
        if (!s_->playAuto(*m)) break;
    }
    flushNews(false);
    if (s_->phase == Phase::PLAYOFF || phaseLimit == static_cast<int>(Phase::PLAYOFF)) reports::table(cout, *s_);
    if (s_->bracket.built() && features::KNOCKOUT_BRACKET) reports::bracketTree(cout, *s_);
    int c = s_->champion();
    if (c >= 0) cout << "\n" << banner("CHAMPIONS OF EUROPE: " + s_->team(c).name);
}

// ---------------------------------------------------------------------------
//  Views
// ---------------------------------------------------------------------------
int App::pickTeam(const char* prompt) {
    for (;;) {
        std::string q = ask(prompt);
        if (q.empty()) return -1;
        if (q == "list") {
            for (int pot = 1; pot <= 4; ++pot) {
                cout << "  Pot " << pot << ": ";
                for (const Team& t : s_->teams) if (t.pot == pot) cout << t.code << "=" << t.name << "  ";
                cout << "\n";
            }
            continue;
        }
        int t = s_->findTeam(q);
        if (t >= 0) return t;
        cout << "  Team not found. Type a code (e.g. BAY), part of a name, 'list', or Enter to cancel.\n";
    }
}

void App::fixturesMenu() {
    cout << "  1) A league matchday   2) All league fixtures   3) Knockout fixtures   4) One team   5) The league-phase draw\n";
    int c = askInt("  Choice: ", 1, 5, 1);
    if (c == 1) reports::fixtures(cout, *s_, askInt("  Matchday (1-8): ", 1, 8, s_->currentMatchday() ? s_->currentMatchday() : 1));
    else if (c == 2) reports::fixtures(cout, *s_, 0);
    else if (c == 3) reports::fixtures(cout, *s_, -1);
    else if (c == 4) { int t = pickTeam("  Team (code/name/list): "); if (t >= 0) reports::teamFixtures(cout, *s_, t); }
    else reports::drawSummary(cout, *s_);
}

void App::matchCentre() {
    if (s_->playOrder.empty()) { cout << "  No matches played yet. (Match ids are shown in the fixtures list.)\n"; }
    int last = s_->playOrder.empty() ? 0 : s_->playOrder.back();
    int id = askInt("  Match id (see fixtures list) [last played]: ", 0, s_->matches.size() - 1, last);
    reports::matchReport(cout, *s_, s_->match(id));
}

void App::teamsMenu() {
    cout << "  1) Team profile & squad   2) All teams by pot   3) Suspensions & injuries   4) Export team database (teams.txt)\n";
    int c = askInt("  Choice: ", 1, 4, 1);
    if (c == 1) { int t = pickTeam("  Team (code/name/list): "); if (t >= 0) reports::teamReport(cout, *s_, t); }
    else if (c == 2) {
        for (int pot = 1; pot <= 4; ++pot) {
            cout << "\n  POT " << pot << "\n";
            for (const Team& t : s_->teams)
                if (t.pot == pot)
                    cout << "   " << padR(t.code, 5) << padR(t.name, 28) << padR(t.country, 5) << " Elo "
                         << fixed(t.elo, 0) << "   " << t.stadium << "\n";
        }
    } else if (c == 3) reports::availability(cout, *s_);
    else cout << (exportTeamDatabase("teams.txt") ? "  Written teams.txt - edit it and restart to use custom squads.\n"
                                                  : "  Could not write teams.txt\n");
}

void App::playersMenu() {
    cout << "  1) Leaderboards   2) Player profile (search)   3) Suspensions & injuries\n";
    int c = askInt("  Choice: ", 1, 3, 1);
    if (c == 1) {
        for (int k = 0; k < stats::RANK_KINDS; ++k)
            cout << "   " << k + 1 << ") " << stats::rankByName(static_cast<stats::RankBy>(k)) << "\n";
        int k = askInt("  Ranking [1]: ", 1, stats::RANK_KINDS, 1);
        int n = askInt("  How many players [15]: ", 1, 100, 15);
        reports::playerRanking(cout, *s_, static_cast<stats::RankBy>(k - 1), n);
    } else if (c == 2) {
        std::string q = ask("  Player name (or part of it): ");
        ds::DynArray<int> found;
        s_->findPlayers(q, found);
        if (found.empty()) { cout << "  No player found.\n"; return; }
        int pick = 0;
        if (found.size() > 1) {
            for (int i = 0; i < found.size() && i < 20; ++i)
                cout << "   " << i + 1 << ") " << s_->player(found[i]).name << " (" << s_->team(found[i] / 100).code << ")\n";
            pick = askInt("  Which one: ", 1, found.size() < 20 ? found.size() : 20, 1) - 1;
        }
        reports::playerProfile(cout, *s_, s_->player(found[pick]));
    } else {
        reports::availability(cout, *s_);
    }
}

void App::bracketMenu() {
    cout << "  1) Tree view   2) Round by round\n";
    if (askInt("  Choice: ", 1, 2, 1) == 1) reports::bracketTree(cout, *s_);
    else reports::bracketRounds(cout, *s_);
}

void App::statsMenu() {
    cout << "  1) Season statistics   2) Performance by country   3) Elo power ranking   4) Data-structure internals\n";
    int c = askInt("  Choice: ", 1, 4, 1);
    if (c == 1) reports::seasonStats(cout, *s_);
    else if (c == 2) reports::countryTable(cout, *s_);
    else if (c == 3) reports::eloTable(cout, *s_);
    else reports::dataStructures(cout, *s_);
}

// ---------------------------------------------------------------------------
//  Predictions
// ---------------------------------------------------------------------------
void App::predictionsMenu() {
    cout << "  1) Predict any match (Elo model)\n"
            "  2) Forecast the next 10 fixtures\n"
            "  3) Monte-Carlo projections: top-8 / play-off / elimination / title odds\n"
            "  4) What-if: force results and see the aftermath on everyone's odds\n"
            "  5) Prediction game: predict the next matches, then play them and score\n";
    int c = askInt("  Choice: ", 1, 5, 1);
    if (c == 1) {
        int h = pickTeam("  Home team (code/name/list): ");
        if (h < 0) return;
        int a = pickTeam("  Away team: ");
        if (a < 0 || a == h) return;
        bool neutral = askYesNo("  Neutral venue?", false);
        reports::prediction(cout, *s_, h, a, neutral);
    } else if (c == 2) {
        cout << "\n  Upcoming fixtures - Elo forecast\n";
        for (int i = 0; i < s_->pending.size() && i < 10; ++i) {
            const Match& m = s_->match(s_->pending.peek(i));
            elo::Outcome o = elo::predict(s_->team(m.home).elo, s_->team(m.away).elo, m.neutral);
            cout << "  #" << padL(std::to_string(m.id), 3) << " " << padL(s_->team(m.home).name, 24) << " v "
                 << padR(s_->team(m.away).name, 24) << " H" << pct(o.home) << " D" << pct(o.draw) << " A" << pct(o.away)
                 << "  tip " << o.likelyH << "-" << o.likelyA << "\n";
        }
    } else if (c == 3) {
        cout << "  Running " << s_->settings.mcIterations << " simulations...\n";
        ds::Random rng(s_->seed * 31 + s_->playedCount() + 1);
        MCResult r;
        runMonteCarlo(*s_, s_->settings.mcIterations, rng, ds::DynArray<ForcedResult>(), r);
        reports::monteCarlo(cout, *s_, r, nullptr);
    } else if (c == 4) {
        whatIf();
    } else {
        predictionGame();
    }
}

void App::whatIf() {
    if (s_->pending.empty()) { cout << "  Nothing left to predict.\n"; return; }
    cout << "\n  Upcoming matches you can force a result for:\n";
    int shown = s_->pending.size() < 24 ? s_->pending.size() : 24;
    for (int i = 0; i < shown; ++i) reports::matchLine(cout, *s_, s_->match(s_->pending.peek(i)));
    ds::DynArray<ForcedResult> forced;
    for (;;) {
        std::string line = ask("  Enter '<match id> <home goals> <away goals>' (blank = run): ");
        if (line.empty()) break;
        std::istringstream in(line);
        ForcedResult f;
        if (!(in >> f.matchId >> f.hg >> f.ag) || f.matchId < 0 || f.matchId >= s_->matches.size() ||
            s_->match(f.matchId).played || f.hg < 0 || f.ag < 0) {
            cout << "  Invalid (match must exist and be unplayed).\n";
            continue;
        }
        forced.push(f);
        cout << "  Forced: " << s_->team(s_->match(f.matchId).home).name << " " << f.hg << "-" << f.ag << " "
             << s_->team(s_->match(f.matchId).away).name << "\n";
    }
    if (forced.empty()) return;
    int n = s_->settings.mcIterations;
    cout << "  Simulating " << n << " tournaments with and without your results...\n";
    MCResult base, after;
    unsigned long long sd = s_->seed * 131 + s_->playedCount();
    ds::Random r1(sd), r2(sd);    // common random numbers => fair comparison
    runMonteCarlo(*s_, n, r1, ds::DynArray<ForcedResult>(), base);
    runMonteCarlo(*s_, n, r2, forced, after);
    reports::monteCarlo(cout, *s_, after, &base);
    cout << "  (chg) = change in title chances (percentage points) caused by your forced results.\n";
    ds::DynArray<int> teamsInvolved;
    for (const ForcedResult& f : forced) { teamsInvolved.push(s_->match(f.matchId).home); teamsInvolved.push(s_->match(f.matchId).away); }
    cout << "\n  Aftermath for the teams involved:\n";
    for (int t : teamsInvolved)
        cout << "   " << padR(s_->team(t).name, 26) << " Top-8 " << pct(base.top8[t]) << " ->" << pct(after.top8[t])
             << "   Eliminated " << pct(base.out[t]) << " ->" << pct(after.out[t]) << "   Title " << pct(base.win[t])
             << " ->" << pct(after.win[t]) << "\n";
}

void App::predictionGame() {
    int n = askInt("  How many upcoming matches to predict (1-18) [5]: ", 1, 18, 5);
    if (n > s_->pending.size()) n = s_->pending.size();
    if (n == 0) { cout << "  No matches left.\n"; return; }
    ds::DynArray<int> ids, ph, pa;
    for (int i = 0; i < n; ++i) {
        const Match& m = s_->match(s_->pending.peek(i));
        elo::Outcome o = elo::predict(s_->team(m.home).elo, s_->team(m.away).elo, m.neutral);
        int h, a;
        for (;;) {
            std::istringstream in(ask("  " + s_->team(m.home).name + " v " + s_->team(m.away).name + "  (Elo tip " +
                                      std::to_string(o.likelyH) + "-" + std::to_string(o.likelyA) + ")  your score: "));
            if (in >> h >> a && h >= 0 && a >= 0) break;
            cout << "  e.g. 2 1\n";
        }
        ids.push(m.id); ph.push(h); pa.push(a);
    }
    int you = 0, model = 0;
    cout << "\n  Results (3 pts exact score, 1 pt correct outcome):\n";
    for (int i = 0; i < ids.size(); ++i) {
        Match& m = s_->match(ids[i]);
        elo::Outcome o = elo::predict(s_->team(m.home).elo, s_->team(m.away).elo, m.neutral);
        if (!s_->playAuto(m)) break;
        auto points = [&m](int h, int a) {
            if (h == m.hg && a == m.ag) return 3;
            int r1 = h > a ? 1 : (h == a ? 0 : -1), r2 = m.hg > m.ag ? 1 : (m.hg == m.ag ? 0 : -1);
            return r1 == r2 ? 1 : 0;
        };
        int py = points(ph[i], pa[i]), pm = points(o.likelyH, o.likelyA);
        you += py; model += pm;
        cout << "   " << padR(reports::scoreLine(*s_, m), 52) << " you " << ph[i] << "-" << pa[i] << " (+" << py
             << ")  Elo " << o.likelyH << "-" << o.likelyA << " (+" << pm << ")\n";
    }
    flushNews();
    cout << "  FINAL SCORE:  You " << you << "  vs  Elo model " << model << (you > model ? "  - you beat the model!" : you == model ? "  - a draw!" : "  - the model wins this time.") << "\n";
}

// ---------------------------------------------------------------------------
//  Reports / save / load / settings
// ---------------------------------------------------------------------------
void App::reportsMenu() {
    MAKE_DIR("reports");
    enum Kind { SEASON, TABLE, FIXTURES, TEAM, MATCH, PLAYERS, PROJECTIONS, BRACKET, EVERYTHING };
    struct Item { const char* label; Kind kind; bool enabled; };
    const Item all[] = {
        {"Full season report", SEASON, true},
        {"League table", TABLE, true},
        {"All fixtures & results", FIXTURES, true},
        {"Team report", TEAM, true},
        {"Match report", MATCH, features::MATCH_CENTRE},
        {"Player leaderboards", PLAYERS, features::PLAYER_STATS},
        {"Monte-Carlo projections", PROJECTIONS, features::PREDICTIONS},
        {"Knockout bracket", BRACKET, features::KNOCKOUT_BRACKET},
        {"Everything", EVERYTHING, true},
    };
    ds::DynArray<Item> menu;
    for (const Item& it : all) if (it.enabled) menu.push(it);
    cout << "  Export to the 'reports' folder:\n";
    for (int i = 0; i < menu.size(); ++i) cout << "   " << i + 1 << ") " << menu[i].label << "\n";
    Kind k = menu[askInt("  Choice: ", 1, menu.size(), 1) - 1].kind;
    bool all9 = k == EVERYTHING;

    auto write = [](const std::string& name, auto fn) {
        std::string path = "reports/" + name;
        std::ofstream out(path);
        if (!out) { cout << "  Could not write " << path << "\n"; return; }
        fn(out);
        cout << "  Written " << path << "\n";
    };
    Season& s = *s_;
    if (k == SEASON || all9) write("season_report.txt", [&s](std::ostream& o) { reports::fullSeasonReport(o, s); });
    if (k == TABLE || all9) write("league_table.txt", [&s](std::ostream& o) { reports::table(o, s); });
    if (k == FIXTURES || all9) write("fixtures.txt", [&s](std::ostream& o) {
        reports::drawSummary(o, s);
        reports::fixtures(o, s, 0);
        reports::fixtures(o, s, -1);
    });
    if (k == TEAM) {
        int t = pickTeam("  Team (code/name/list): ");
        if (t >= 0) write("team_" + s.team(t).code + ".txt", [&s, t](std::ostream& o) { reports::teamReport(o, s, t); });
    }
    if (all9) {
        for (const Team& t : s.teams) {
            std::ofstream out("reports/team_" + t.code + ".txt");
            reports::teamReport(out, s, t.id);
        }
        cout << "  Written reports/team_<CODE>.txt for all 36 clubs\n";
    }
    if (k == MATCH) {
        int id = askInt("  Match id: ", 0, s.matches.size() - 1, s.playOrder.empty() ? 0 : s.playOrder.back());
        write("match_" + std::to_string(id) + ".txt", [&s, id](std::ostream& o) { reports::matchReport(o, s, s.match(id)); });
    }
    if (all9 && features::MATCH_CENTRE) write("all_matches.txt", [&s](std::ostream& o) {
        for (int id : s.playOrder) { reports::matchReport(o, s, s.match(id)); o << "\n"; }
    });
    if (k == PLAYERS || (all9 && features::PLAYER_STATS)) write("player_rankings.txt", [&s](std::ostream& o) {
        for (int r = 0; r < stats::RANK_KINDS; ++r) { reports::playerRanking(o, s, static_cast<stats::RankBy>(r), 25); o << "\n"; }
        reports::availability(o, s);
    });
    if (k == PROJECTIONS || (all9 && features::PREDICTIONS)) {
        ds::Random rng(s.seed * 31 + s.playedCount() + 1);
        MCResult r;
        runMonteCarlo(s, s.settings.mcIterations, rng, ds::DynArray<ForcedResult>(), r);
        write("projections.txt", [&s, &r](std::ostream& o) { reports::monteCarlo(o, s, r, nullptr); });
    }
    if (k == BRACKET || (all9 && features::KNOCKOUT_BRACKET)) write("bracket.txt", [&s](std::ostream& o) {
        reports::bracketTree(o, s);
        reports::bracketRounds(o, s);
    });
}

void App::saveMenu() {
    std::string path = ask("  Save file name [season.sav]: ");
    if (path.empty()) path = "season.sav";
    std::string err;
    if (saveload::save(*s_, path, err)) cout << "  Saved " << s_->playedCount() << " played matches to " << path << "\n";
    else cout << "  Save failed: " << err << "\n";
}

void App::loadMenu() {
    std::string path = ask("  Load file name [season.sav]: ");
    if (path.empty()) path = "season.sav";
    std::string err;
    cout << "  Rebuilding the season and replaying every match through the rule engine...\n";
    Season* loaded = saveload::load(path, err);
    if (!loaded) { cout << "  Load failed: " << err << "\n"; return; }
    delete s_;
    s_ = loaded;
    cout << "  Loaded: seed " << s_->seed << ", " << s_->playedCount() << " matches replayed, stage: " << phaseName(s_->phase) << "\n";
}

void App::settingsMenu() {
    cout << "  League table display: 0 = never, 1 = after every match, 2 = after every matchday (now "
         << s_->settings.tableMode << ")\n";
    s_->settings.tableMode = askInt("  Table mode: ", 0, 2, s_->settings.tableMode);
    if (features::PREDICTIONS) s_->settings.mcIterations = askInt("  Monte-Carlo iterations (100-20000) [" + std::to_string(s_->settings.mcIterations) + "]: ",
                                       100, 20000, s_->settings.mcIterations);
}
