#include "LiveConsole.h"
#include <iostream>
#include <sstream>
#include "MergeSort.h"
#include "Reports.h"
#include "Season.h"
#include "Console.h"

namespace ui {

namespace {

void printSquadForSelection(const Team& t) {
    std::cout << "\n  " << t.name << " squad (unavailable players are marked and cannot be picked)\n";
    for (int p = 0; p < 4; ++p) {
        std::cout << "   " << positionName(static_cast<Position>(p)) << ": ";
        int col = 0;
        for (const Player& pl : t.squad) {
            if (static_cast<int>(pl.pos) != p) continue;
            std::string item = "#" + std::to_string(pl.number) + " " + pl.name + " (" + std::to_string(pl.rating) + ")";
            if (!pl.available()) item += " [" + pl.statusText() + "]";
            if (col > 0 && col % 3 == 0) std::cout << "\n       ";
            std::cout << padR(item, 34);
            ++col;
        }
        std::cout << "\n";
    }
}

// Rest of the available squad, best first, becomes the bench (max 12).
void fillBench(const Team& t, Lineup& l) {
    bool used[MatchRecorder::MAXP] = {false};
    for (int i = 0; i < XI; ++i) if (l.starters[i] >= 0) used[l.starters[i]] = true;
    ds::DynArray<int> rest;
    for (int i = 0; i < t.squad.size(); ++i)
        if (!used[i] && t.squad[i].available()) rest.push(i);
    ds::mergeSort(rest, [&t](int a, int b) { return t.squad[a].rating > t.squad[b].rating; });
    l.benchCount = 0;
    for (int i : rest) {
        if (l.benchCount >= MAX_BENCH) break;
        l.bench[l.benchCount++] = i;
    }
}

void showPitch(const MatchRecorder& rec) {
    for (int side = 0; side < 2; ++side) {
        const Team& t = rec.team(side);
        ds::DynArray<int> on, bench;
        rec.pitchPlayers(side, on);
        rec.benchPlayers(side, bench);
        std::cout << "  " << t.code << " on pitch (" << on.size() << "): ";
        for (int p : on) {
            std::cout << "#" << t.squad[p].number << " " << t.squad[p].name;
            if (rec.yellowCount(side, p)) std::cout << "[Y]";
            if (rec.injuredInMatch(side, p)) std::cout << "[inj]";
            std::cout << ", ";
        }
        std::cout << "\n       bench: ";
        for (int p : bench) std::cout << "#" << t.squad[p].number << " " << t.squad[p].name << ", ";
        std::cout << "\n       subs used " << rec.subsUsed(side) << "/" << rec.maxSubs() << ", windows "
                  << rec.windowsUsed(side) << "/" << rec.maxWindows() << "\n";
    }
}

void liveHelp() {
    std::cout <<
        "  Commands (side = h / a, players are SHIRT NUMBERS, minute 1-90, ET 91-120):\n"
        "    g  <side> <min> <scorer> [assist]   goal               e.g.  g h 23 9 17\n"
        "    p  <side> <min> <scorer>            penalty scored     e.g.  p a 55 10\n"
        "    pm <side> <min> <taker>             penalty missed\n"
        "    og <side> <min> <player>            own goal BY a player of <side>\n"
        "    y  <side> <min> <player>            yellow card (2nd yellow = red, automatic)\n"
        "    r  <side> <min> <player> [ban]      straight red card (ban 1-5, default 1)\n"
        "    s  <side> <min> <off> <on>          substitution (46 = half-time, free window)\n"
        "    i  <side> <min> <player> <matches>  injury (player stays on until you sub him)\n"
        "    auto [min]                          let the simulator play on (to min, or to the end)\n"
        "    ft                                  full-time whistle (extra time / pens if required)\n"
        "    l   show players on pitch & bench   e  show timeline   ?  this help\n";
}

int sideOf(const std::string& tok, const MatchRecorder& rec) {
    std::string t = lower(tok);
    if (t == "h" || t == "home" || t == "0") return 0;
    if (t == "a" || t == "away" || t == "1") return 1;
    if (t == lower(rec.team(0).code)) return 0;
    if (t == lower(rec.team(1).code)) return 1;
    return -1;
}

int playerOf(const MatchRecorder& rec, int side, const std::string& tok) {
    int n;
    if (!parseInt(tok, n)) return -2;
    return rec.team(side).indexByNumber(n);
}

void printLastEvent(const Season& s, const Match& m) {
    if (m.events.empty()) return;
    std::cout << "   " << reports::eventText(s, m, m.events.back()) << "\n";
}

std::string scoreboard(const MatchRecorder& rec) {
    std::string clock = rec.inShootout() ? "PENS" : std::to_string(rec.minute()) + "'";
    return "[" + padL(clock, 4) + "] " + rec.team(0).code + " " + std::to_string(rec.score(0)) + "-" +
           std::to_string(rec.score(1)) + " " + rec.team(1).code;
}

// Interactive penalty shoot-out
void runShootout(Season& s, MatchRecorder& rec, MatchSimulator& sim) {
    std::string err;
    rec.beginShootout(err);
    std::cout << "\n  PENALTY SHOOT-OUT! Enter kicks as:  k <shirt> <1=scored|0=missed>   or 'auto'\n";
    while (!rec.shootoutDecided()) {
        int side = rec.nextKickSide();
        std::string line = ask("  Pens " + std::to_string(rec.penScore(0)) + "-" + std::to_string(rec.penScore(1)) +
                               "  " + rec.team(side).code + " to kick > ");
        std::istringstream in(line);
        std::string cmd, a, b;
        in >> cmd >> a >> b;
        if (cmd == "auto") { sim.simulateShootout(rec); break; }
        if (cmd != "k") { std::cout << "  Use: k <shirt> <1|0>  or  auto\n"; continue; }
        int p = playerOf(rec, side, a);
        int scored;
        if (p < 0 || !parseInt(b, scored)) { std::cout << "  Unknown player / result.\n"; continue; }
        if (!rec.penaltyKick(side, p, scored != 0, err)) std::cout << "  REJECTED: " << err << "\n";
    }
    for (const PenaltyKick& k : rec.match().kicks)
        std::cout << "    " << rec.team(k.side).code << " " << padR(rec.team(k.side).squad[k.player].name, 28)
                  << (k.scored ? "scored" : "MISSED") << "\n";
    std::cout << "  Shoot-out: " << rec.penScore(0) << "-" << rec.penScore(1) << " -> "
              << rec.team(rec.shootoutWinner()).name << " win!\n";
    (void)s;
}

}  // namespace

bool chooseLineup(const Team& t, Lineup& out) {
    std::cout << "\n  Line-up for " << t.name << ":  1) auto-pick best available XI   2) choose manually\n";
    int c = askInt("  Choice [1]: ", 1, 2, 1);
    if (c == 1) {
        out = MatchSimulator::autoLineup(t);
        return true;
    }
    printSquadForSelection(t);
    for (;;) {
        std::string line = ask("  Enter 11 shirt numbers (exactly 1 GK), or 'auto': ");
        if (lower(line) == "auto") { out = MatchSimulator::autoLineup(t); return true; }
        std::istringstream in(line);
        Lineup l;
        std::string tok;
        int n = 0;
        bool bad = false;
        while (in >> tok) {
            int num;
            if (!parseInt(tok, num) || t.indexByNumber(num) < 0) {
                std::cout << "  No player wears #" << tok << ".\n";
                bad = true;
                break;
            }
            if (n < XI) l.starters[n] = t.indexByNumber(num);
            ++n;
        }
        if (bad) continue;
        if (n != XI) { std::cout << "  You entered " << n << " players - a team needs exactly 11.\n"; continue; }
        fillBench(t, l);
        std::string err;
        if (!MatchRecorder::validateLineup(t, l, err)) { std::cout << "  INVALID: " << err << "\n"; continue; }
        out = l;
        return true;
    }
}

bool runLiveMatch(Season& s, Match& m) {
    Team& home = s.team(m.home);
    Team& away = s.team(m.away);
    std::cout << banner("LIVE MATCH RECORDING");
    std::cout << "  " << home.name << " v " << away.name << "  -  " << stageName(m.stage) << "\n";
    s.prepareMatch(m);
    Lineup lh, la;
    chooseLineup(home, lh);
    chooseLineup(away, la);

    MatchRecorder rec(m, home, away);
    std::string err;
    if (!rec.start(lh, la, err)) { std::cout << "  Cannot start: " << err << "\n"; return false; }
    MatchSimulator sim(s.rng);
    std::cout << "\n  Kick-off! Type '?' for the list of commands.\n";
    liveHelp();

    while (!rec.isAbandoned()) {
        std::string line = ask("  " + scoreboard(rec) + " > ");
        std::istringstream in(line);
        std::string cmd;
        in >> cmd;
        cmd = lower(cmd);
        if (cmd.empty()) continue;
        if (cmd == "?" || cmd == "help") { liveHelp(); continue; }
        if (cmd == "l") { showPitch(rec); continue; }
        if (cmd == "e") {
            for (const MatchEvent& e : m.events) std::cout << "   " << reports::eventText(s, m, e) << "\n";
            continue;
        }
        if (cmd == "auto") {
            std::string t;
            int to = rec.inExtraTime() ? 120 : 90;
            if (in >> t) {
                if (!parseInt(t, to)) { std::cout << "  auto [minute]\n"; continue; }
            }
            int before = m.events.size();
            sim.simulateTo(rec, to);
            int idx = 0;
            for (const MatchEvent& e : m.events)
                if (idx++ >= before) std::cout << "   " << reports::eventText(s, m, e) << "\n";
            std::cout << "   ... simulated to " << rec.minute() << "'\n";
            if (rec.minute() >= (rec.inExtraTime() ? 120 : 90) && !rec.isAbandoned()) {
                if (!askYesNo("  End of the period reached. Blow the whistle?", true)) continue;
                cmd = "ft";
            } else {
                continue;
            }
        }
        if (cmd == "ft") {
            if (s.needsDecider(rec)) {
                if (!rec.inExtraTime()) {
                    std::cout << "  Level " << (m.leg == 2 ? "on aggregate" : "") << " - EXTRA TIME (minutes 91-120)!\n";
                    rec.startExtraTime(err);
                    continue;
                }
                runShootout(s, rec, sim);
            }
            break;
        }

        // ---- event commands -------------------------------------------
        std::string sideTok, minTok, p1Tok, p2Tok;
        in >> sideTok >> minTok >> p1Tok >> p2Tok;
        int side = sideOf(sideTok, rec);
        int minute;
        if (side < 0 || !parseInt(minTok, minute)) {
            std::cout << "  Format: <cmd> <h|a> <minute> <shirt> ...  (type ? for help)\n";
            continue;
        }
        bool ok = false;
        if (cmd == "og") {
            int p = playerOf(rec, side, p1Tok);
            if (p < 0) { std::cout << "  Unknown shirt number.\n"; continue; }
            ok = rec.ownGoal(side, minute, p, err);
        } else if (cmd == "g" || cmd == "p" || cmd == "pm" || cmd == "y" || cmd == "r" || cmd == "s" || cmd == "i") {
            int p = playerOf(rec, side, p1Tok);
            if (p < 0) { std::cout << "  Unknown shirt number '" << p1Tok << "' for " << rec.team(side).name << ".\n"; continue; }
            int num2 = -1;
            bool has2 = !p2Tok.empty() && parseInt(p2Tok, num2);
            if (cmd == "g") {
                int assist = has2 ? rec.team(side).indexByNumber(num2) : -1;
                if (has2 && assist < 0) { std::cout << "  Unknown assist shirt number.\n"; continue; }
                ok = rec.goal(side, minute, p, assist, false, err);
            } else if (cmd == "p") ok = rec.goal(side, minute, p, -1, true, err);
            else if (cmd == "pm") ok = rec.penaltyMiss(side, minute, p, err);
            else if (cmd == "y") ok = rec.yellow(side, minute, p, err);
            else if (cmd == "r") ok = rec.red(side, minute, p, has2 ? num2 : 1, err);
            else if (cmd == "i") ok = rec.injury(side, minute, p, has2 ? num2 : 1, err);
            else if (cmd == "s") {
                int in2 = has2 ? rec.team(side).indexByNumber(num2) : -1;
                if (in2 < 0) { std::cout << "  Unknown substitute shirt number.\n"; continue; }
                ok = rec.sub(side, minute, p, in2, err);
            }
        } else {
            std::cout << "  Unknown command '" << cmd << "'. Type ? for help.\n";
            continue;
        }
        if (ok) printLastEvent(s, m);
        else std::cout << "  REJECTED: " << err << "\n";
    }
    if (rec.isAbandoned())
        std::cout << "  MATCH ABANDONED - " << rec.team(m.forfeitSide).name << " has fewer than 7 players. Awarded 3-0.\n";
    rec.finish();
    m.mode = PlayMode::LIVE;
    s.commitMatch(m, rec);
    return true;
}

bool runQuickScore(Season& s, Match& m) {
    s.prepareMatch(m);
    Team& home = s.team(m.home);
    Team& away = s.team(m.away);
    MatchRecorder rec(m, home, away);
    std::string err;
    if (!rec.start(MatchSimulator::autoLineup(home), MatchSimulator::autoLineup(away), err)) {
        std::cout << "  Cannot start: " << err << "\n";
        return false;
    }
    MatchSimulator sim(s.rng);
    auto askScore = [](const std::string& prompt, int& h, int& a) {
        for (;;) {
            std::istringstream in(ask(prompt));
            if (in >> h >> a && h >= 0 && a >= 0 && h <= 15 && a <= 15) return;
            std::cout << "  Enter two numbers 0-15, e.g.  2 1\n";
        }
    };
    int h, a;
    askScore("  Score after 90 minutes - " + home.code + " and " + away.code + " (e.g. 2 1): ", h, a);
    sim.quickScore(rec, h, a, 1, 90);
    if (s.needsDecider(rec)) {
        std::cout << "  The tie is level - extra time!\n";
        rec.startExtraTime(err);
        askScore("  Goals in extra time (home away, e.g. 0 0): ", h, a);
        sim.quickScore(rec, h, a, 91, 120);
        if (s.needsDecider(rec)) {
            std::cout << "  Still level - the penalty shoot-out is simulated.\n";
            sim.simulateShootout(rec);
        }
    }
    rec.finish();
    m.mode = PlayMode::QUICK;
    s.commitMatch(m, rec);
    return true;
}

}  // namespace ui
