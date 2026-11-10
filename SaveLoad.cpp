// Save format (one token-separated record per line):
//   UCLSIM-SAVE 1
//   SEED <seed>
//   RNG <state>
//   SETTINGS <tableMode> <mcIterations>
//   MATCH <id> <mode> <extraTime 0/1> <shootout 0/1>
//   L <side> <11 starters> <benchCount> <bench...>
//   E <type> <minute> <side> <player> <player2> <extra>
//   K <side> <player> <scored>
//   END
#include "SaveLoad.h"
#include <fstream>
#include <sstream>
#include "Season.h"

namespace saveload {

bool save(const Season& s, const std::string& path, std::string& err) {
    std::ofstream out(path);
    if (!out) { err = "cannot open " + path + " for writing"; return false; }
    out << "UCLSIM-SAVE 1\n";
    out << "SEED " << s.seed << "\n";
    out << "RNG " << s.rng.state() << "\n";
    out << "SETTINGS " << s.settings.tableMode << " " << s.settings.mcIterations << "\n";
    for (int id : s.playOrder) {
        const Match& m = s.match(id);
        out << "MATCH " << m.id << " " << static_cast<int>(m.mode) << " " << (m.extraTime ? 1 : 0) << " "
            << (m.shootout ? 1 : 0) << "\n";
        for (int side = 0; side < 2; ++side) {
            const Lineup& l = m.lineup[side];
            out << "L " << side;
            for (int i = 0; i < XI; ++i) out << " " << l.starters[i];
            out << " " << l.benchCount;
            for (int i = 0; i < l.benchCount; ++i) out << " " << l.bench[i];
            out << "\n";
        }
        for (const MatchEvent& e : m.events)
            out << "E " << static_cast<int>(e.type) << " " << e.minute << " " << e.side << " " << e.player << " "
                << e.player2 << " " << e.extra << "\n";
        for (const PenaltyKick& k : m.kicks) out << "K " << k.side << " " << k.player << " " << (k.scored ? 1 : 0) << "\n";
        out << "END\n";
    }
    if (!out) { err = "write error"; return false; }
    return true;
}

namespace {

struct SavedMatch {
    int id = -1, mode = 0, et = 0, so = 0;
    Lineup lineup[2];
    ds::LinkedList<MatchEvent> events;
    ds::LinkedList<PenaltyKick> kicks;
};

bool replay(Season& s, SavedMatch& sm, std::string& err) {
    Match* next = s.nextMatch();
    if (!next || next->id != sm.id) {
        err = "match #" + std::to_string(sm.id) + " is out of order";
        return false;
    }
    Match& m = *next;
    s.prepareMatch(m);
    MatchRecorder rec(m, s.team(m.home), s.team(m.away));
    if (!rec.start(sm.lineup[0], sm.lineup[1], err)) return false;
    for (const MatchEvent& e : sm.events) {
        if (e.minute > 90 && !rec.inExtraTime() && !rec.startExtraTime(err)) return false;
        if (!rec.apply(e, err)) {
            err = "match #" + std::to_string(sm.id) + ": " + err;
            return false;
        }
    }
    if (sm.et && !rec.inExtraTime() && !rec.startExtraTime(err)) return false;
    if (sm.so) {
        if (!rec.beginShootout(err)) return false;
        for (const PenaltyKick& k : sm.kicks)
            if (!rec.penaltyKick(k.side, k.player, k.scored, err)) return false;
    }
    rec.finish();
    m.mode = static_cast<PlayMode>(sm.mode);
    s.commitMatch(m, rec);
    return true;
}

}  // namespace

Season* load(const std::string& path, std::string& err) {
    std::ifstream in(path);
    if (!in) { err = "cannot open " + path; return nullptr; }
    std::string line, tag;
    std::getline(in, line);
    if (line.rfind("UCLSIM-SAVE", 0) != 0) { err = "not a simulator save file"; return nullptr; }

    unsigned long long seed = 0, rngState = 0;
    int tableMode = 1, mcIter = 2000;
    Season* s = nullptr;
    SavedMatch* cur = nullptr;
    int lineNo = 1;
    auto fail = [&](const std::string& msg) {
        err = "line " + std::to_string(lineNo) + ": " + msg;
        delete cur;
        delete s;
        return static_cast<Season*>(nullptr);
    };

    while (std::getline(in, line)) {
        ++lineNo;
        if (line.empty()) continue;
        std::istringstream ls(line);
        ls >> tag;
        if (tag == "SEED") ls >> seed;
        else if (tag == "RNG") ls >> rngState;
        else if (tag == "SETTINGS") ls >> tableMode >> mcIter;
        else if (tag == "MATCH") {
            if (!s) {
                s = new Season();
                std::string log;
                if (!s->create(seed, log)) return fail("could not rebuild season: " + log);
            }
            delete cur;
            cur = new SavedMatch();
            ls >> cur->id >> cur->mode >> cur->et >> cur->so;
        } else if (tag == "L" && cur) {
            int side;
            ls >> side;
            if (side < 0 || side > 1) return fail("bad line-up side");
            Lineup& l = cur->lineup[side];
            for (int i = 0; i < XI; ++i) ls >> l.starters[i];
            ls >> l.benchCount;
            if (l.benchCount < 0 || l.benchCount > MAX_BENCH) return fail("bad bench size");
            for (int i = 0; i < l.benchCount; ++i) ls >> l.bench[i];
        } else if (tag == "E" && cur) {
            MatchEvent e;
            int type;
            ls >> type >> e.minute >> e.side >> e.player >> e.player2 >> e.extra;
            if (type < 0 || type > static_cast<int>(EventType::INJURY) || e.side < 0 || e.side > 1)
                return fail("bad event");
            e.type = static_cast<EventType>(type);
            cur->events.pushBack(e);
        } else if (tag == "K" && cur) {
            PenaltyKick k;
            int scored;
            ls >> k.side >> k.player >> scored;
            k.scored = scored != 0;
            cur->kicks.pushBack(k);
        } else if (tag == "END" && cur) {
            if (!replay(*s, *cur, err)) return fail(err);
            delete cur;
            cur = nullptr;
        } else {
            return fail("unexpected record '" + tag + "'");
        }
        if (ls.fail() && tag != "END") return fail("malformed record");
    }
    delete cur;
    if (!s) {
        s = new Season();
        std::string log;
        if (!s->create(seed, log)) { err = log; delete s; return nullptr; }
    }
    s->rng.setState(rngState);
    s->settings.tableMode = tableMode;
    s->settings.mcIterations = mcIter;
    s->news.clear();
    s->playerNews.clear();
    return s;
}

}  // namespace saveload
