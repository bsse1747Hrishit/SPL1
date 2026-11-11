#include "MatchRecorder.h"

MatchRecorder::MatchRecorder(Match& m, Team& home, Team& away) : m_(m) {
    t_[0] = &home;
    t_[1] = &away;
    for (int s = 0; s < 2; ++s) {
        for (int p = 0; p < MAXP; ++p) {
            status_[s][p] = NONE;
            starter_[s][p] = false;
            enter_[s][p] = -1;
            exit_[s][p] = -1;
            yc_[s][p] = 0;
            injuredNow_[s][p] = false;
            kicksBy_[s][p] = 0;
        }
        subs_[s] = windows_[s] = score_[s] = pen_[s] = taken_[s] = 0;
        lastWindow_[s] = -1;
    }
}

// ---------------------------------------------------------------------------
//  Line-ups
// ---------------------------------------------------------------------------
bool MatchRecorder::validateLineup(const Team& t, const Lineup& l, std::string& err) {
    bool used[MAXP] = {false};
    int keepers = 0;
    for (int i = 0; i < XI; ++i) {
        int p = l.starters[i];
        if (p < 0 || p >= t.squad.size()) { err = t.name + ": starting XI is incomplete (need 11 players)"; return false; }
        const Player& pl = t.squad[p];
        if (used[p]) { err = t.name + ": " + pl.name + " is listed twice"; return false; }
        if (pl.suspended > 0) { err = t.name + ": " + pl.name + " is SUSPENDED and cannot play"; return false; }
        if (pl.injured > 0) { err = t.name + ": " + pl.name + " is INJURED and cannot play"; return false; }
        used[p] = true;
        if (pl.pos == Position::GK) ++keepers;
    }
    int availableKeepers = 0;
    for (const Player& pl : t.squad) if (pl.pos == Position::GK && pl.available()) ++availableKeepers;
    if (keepers > 1 || (keepers == 0 && availableKeepers > 0)) {
        err = t.name + ": the starting XI must contain exactly one goalkeeper";
        return false;
    }
    if (l.benchCount < 0 || l.benchCount > MAX_BENCH) { err = t.name + ": bench may hold at most 12 players"; return false; }
    for (int i = 0; i < l.benchCount; ++i) {
        int p = l.bench[i];
        if (p < 0 || p >= t.squad.size()) { err = t.name + ": invalid bench player"; return false; }
        const Player& pl = t.squad[p];
        if (used[p]) { err = t.name + ": " + pl.name + " cannot be both starter and substitute"; return false; }
        if (!pl.available()) { err = t.name + ": " + pl.name + " is unavailable (" + pl.statusText() + ")"; return false; }
        used[p] = true;
    }
    return true;
}

bool MatchRecorder::start(const Lineup& home, const Lineup& away, std::string& err) {
    if (started_) { err = "match already started"; return false; }
    if (!validateLineup(*t_[0], home, err) || !validateLineup(*t_[1], away, err)) return false;
    const Lineup* l[2] = {&home, &away};
    for (int s = 0; s < 2; ++s) {
        for (int i = 0; i < XI; ++i) {
            int p = l[s]->starters[i];
            status_[s][p] = PITCH;
            starter_[s][p] = true;
            enter_[s][p] = 0;
        }
        for (int i = 0; i < l[s]->benchCount; ++i) status_[s][l[s]->bench[i]] = BENCH;
        m_.lineup[s] = *l[s];
    }
    m_.events.clear();
    m_.kicks.clear();
    started_ = true;
    return true;
}

// ---------------------------------------------------------------------------
//  Rule helpers
// ---------------------------------------------------------------------------
bool MatchRecorder::live(std::string& err) const {
    if (!started_) { err = "the match has not kicked off"; return false; }
    if (finished_) { err = "the match is over"; return false; }
    if (abandoned_) { err = "the match was abandoned"; return false; }
    if (shootout_) { err = "the penalty shoot-out is in progress"; return false; }
    return true;
}

bool MatchRecorder::checkMinute(int minute, std::string& err) const {
    int lo = et_ ? 91 : 1;
    int hi = et_ ? 120 : 90;
    if (minute < lo || minute > hi) {
        err = "minute must be between " + std::to_string(lo) + " and " + std::to_string(hi) +
              (et_ ? " (extra time)" : " (stoppage time counts as 90)");
        return false;
    }
    if (minute < minute_) {
        err = "minute " + std::to_string(minute) + " is earlier than the last event (" + std::to_string(minute_) + "')";
        return false;
    }
    return true;
}

bool MatchRecorder::requirePitch(int side, int p, std::string& err) const {
    if (side < 0 || side > 1) { err = "invalid side"; return false; }
    if (!valid(side, p)) { err = "no such player"; return false; }
    if (status_[side][p] != PITCH) {
        const char* why = "is not in the match-day squad";
        switch (status_[side][p]) {
            case BENCH: why = "is on the bench, not on the pitch"; break;
            case SUBBED_OFF: why = "has already been substituted"; break;
            case SENT_OFF: why = "has been sent off"; break;
            default: break;
        }
        err = nameOf(side, p) + " " + why;
        return false;
    }
    return true;
}

int MatchRecorder::playersOnPitch(int side) const {
    int c = 0;
    for (int p = 0; p < t_[side]->squad.size(); ++p) if (status_[side][p] == PITCH) ++c;
    return c;
}

void MatchRecorder::pitchPlayers(int side, ds::DynArray<int>& out) const {
    out.clear();
    for (int p = 0; p < t_[side]->squad.size(); ++p) if (status_[side][p] == PITCH) out.push(p);
}

void MatchRecorder::benchPlayers(int side, ds::DynArray<int>& out) const {
    out.clear();
    for (int p = 0; p < t_[side]->squad.size(); ++p) if (status_[side][p] == BENCH) out.push(p);
}

void MatchRecorder::push(EventType t, int minute, int side, int p, int p2, int extra) {
    MatchEvent e;
    e.type = t; e.minute = minute; e.side = side; e.player = p; e.player2 = p2; e.extra = extra;
    m_.events.pushBack(e);
    if (minute > minute_) minute_ = minute;
}

void MatchRecorder::setClock(int minute) {
    int hi = et_ ? 120 : 90;
    if (minute > hi) minute = hi;
    if (minute > minute_) minute_ = minute;
}

void MatchRecorder::sendOff(int side, int p, int minute) {
    status_[side][p] = SENT_OFF;
    exit_[side][p] = minute;
    if (playersOnPitch(side) < 7) {
        abandoned_ = true;
        m_.forfeitSide = side;
    }
}

// ---------------------------------------------------------------------------
//  Events
// ---------------------------------------------------------------------------
bool MatchRecorder::goal(int side, int minute, int scorer, int assist, bool penalty, std::string& err) {
    if (!live(err) || !checkMinute(minute, err) || !requirePitch(side, scorer, err)) return false;
    if (assist >= 0) {
        if (penalty) { err = "a penalty goal cannot have an assist"; return false; }
        if (assist == scorer) { err = "a player cannot assist his own goal"; return false; }
        if (!requirePitch(side, assist, err)) { err = "assist: " + err; return false; }
    }
    score_[side]++;
    push(penalty ? EventType::PEN_GOAL : EventType::GOAL, minute, side, scorer, assist, 0);
    return true;
}

bool MatchRecorder::ownGoal(int playerSide, int minute, int player, std::string& err) {
    if (!live(err) || !checkMinute(minute, err) || !requirePitch(playerSide, player, err)) return false;
    score_[1 - playerSide]++;
    push(EventType::OWN_GOAL, minute, playerSide, player, -1, 0);
    return true;
}

bool MatchRecorder::penaltyMiss(int side, int minute, int player, std::string& err) {
    if (!live(err) || !checkMinute(minute, err) || !requirePitch(side, player, err)) return false;
    push(EventType::PEN_MISS, minute, side, player, -1, 0);
    return true;
}

bool MatchRecorder::yellow(int side, int minute, int player, std::string& err) {
    if (!live(err) || !checkMinute(minute, err) || !requirePitch(side, player, err)) return false;
    if (yc_[side][player] >= 1) {
        // Second booking: automatic red card
        yc_[side][player] = 2;
        push(EventType::SECOND_YELLOW, minute, side, player, -1, 1);
        sendOff(side, player, minute);
    } else {
        yc_[side][player] = 1;
        push(EventType::YELLOW, minute, side, player, -1, 0);
    }
    return true;
}

bool MatchRecorder::red(int side, int minute, int player, int ban, std::string& err) {
    if (!live(err) || !checkMinute(minute, err) || !requirePitch(side, player, err)) return false;
    if (ban < 1 || ban > 5) { err = "a straight red card carries a ban of 1-5 matches"; return false; }
    push(EventType::RED, minute, side, player, -1, ban);
    sendOff(side, player, minute);
    return true;
}

bool MatchRecorder::canSubAt(int side, int minute) const {
    if (subs_[side] >= maxSubs()) return false;
    bool breakTime = (minute == 46 || minute == 91 || minute == 106);
    if (breakTime || minute == lastWindow_[side]) return true;
    return windows_[side] < maxWindows();
}

bool MatchRecorder::sub(int side, int minute, int out, int in, std::string& err) {
    if (!live(err) || !checkMinute(minute, err)) return false;
    if (!requirePitch(side, out, err)) { err = "player off: " + err; return false; }
    if (!valid(side, in)) { err = "no such substitute"; return false; }
    if (status_[side][in] != BENCH) {
        if (status_[side][in] == SUBBED_OFF) err = nameOf(side, in) + " was already substituted and cannot return";
        else if (status_[side][in] == PITCH) err = nameOf(side, in) + " is already on the pitch";
        else err = nameOf(side, in) + " is not on the bench";
        return false;
    }
    if (subs_[side] >= maxSubs()) {
        err = "all " + std::to_string(maxSubs()) + " substitutions have been used";
        return false;
    }
    bool breakTime = (minute == 46 || minute == 91 || minute == 106);
    if (!breakTime && minute != lastWindow_[side]) {
        if (windows_[side] >= maxWindows()) {
            err = "all " + std::to_string(maxWindows()) + " substitution windows used (subs at 46', 91', 106' are free)";
            return false;
        }
        windows_[side]++;
        lastWindow_[side] = minute;
    }
    status_[side][out] = SUBBED_OFF;
    exit_[side][out] = minute;
    status_[side][in] = PITCH;
    enter_[side][in] = minute;
    subs_[side]++;
    push(EventType::SUB, minute, side, out, in, 0);
    return true;
}

bool MatchRecorder::injury(int side, int minute, int player, int matchesOut, std::string& err) {
    if (!live(err) || !checkMinute(minute, err) || !requirePitch(side, player, err)) return false;
    if (matchesOut < 1 || matchesOut > 10) { err = "injury length must be 1-10 matches"; return false; }
    if (injuredNow_[side][player]) { err = nameOf(side, player) + " is already injured"; return false; }
    injuredNow_[side][player] = true;
    push(EventType::INJURY, minute, side, player, -1, matchesOut);
    return true;
}

bool MatchRecorder::apply(const MatchEvent& e, std::string& err) {
    switch (e.type) {
        case EventType::GOAL:          return goal(e.side, e.minute, e.player, e.player2, false, err);
        case EventType::PEN_GOAL:      return goal(e.side, e.minute, e.player, -1, true, err);
        case EventType::OWN_GOAL:      return ownGoal(e.side, e.minute, e.player, err);
        case EventType::PEN_MISS:      return penaltyMiss(e.side, e.minute, e.player, err);
        case EventType::YELLOW:
            if (yc_[e.side][e.player] != 0) { err = "replay mismatch (yellow)"; return false; }
            return yellow(e.side, e.minute, e.player, err);
        case EventType::SECOND_YELLOW:
            if (yc_[e.side][e.player] != 1) { err = "replay mismatch (second yellow)"; return false; }
            return yellow(e.side, e.minute, e.player, err);
        case EventType::RED:           return red(e.side, e.minute, e.player, e.extra, err);
        case EventType::SUB:           return sub(e.side, e.minute, e.player, e.player2, err);
        case EventType::INJURY:        return injury(e.side, e.minute, e.player, e.extra, err);
    }
    err = "unknown event";
    return false;
}

// ---------------------------------------------------------------------------
//  Extra time & shoot-out
// ---------------------------------------------------------------------------
bool MatchRecorder::startExtraTime(std::string& err) {
    if (!live(err)) return false;
    if (et_) { err = "extra time already started"; return false; }
    et_ = true;
    if (minute_ < 90) minute_ = 90;
    return true;
}

bool MatchRecorder::beginShootout(std::string& err) {
    if (!started_ || finished_ || abandoned_) { err = "no shoot-out possible now"; return false; }
    if (!et_) { err = "a shoot-out only follows extra time"; return false; }
    if (score_[0] != score_[1]) { err = "the match is not level"; return false; }
    shootout_ = true;
    minute_ = 120;
    return true;
}

int MatchRecorder::nextKickSide() const { return taken_[0] == taken_[1] ? 0 : 1; }

bool MatchRecorder::canKick(int side, int player) const {
    if (!valid(side, player) || status_[side][player] != PITCH) return false;
    int minKicks = 1000;
    for (int p = 0; p < t_[side]->squad.size(); ++p)
        if (status_[side][p] == PITCH && kicksBy_[side][p] < minKicks) minKicks = kicksBy_[side][p];
    return kicksBy_[side][player] == minKicks;
}

bool MatchRecorder::penaltyKick(int side, int player, bool scored, std::string& err) {
    if (!shootout_) { err = "no shoot-out in progress"; return false; }
    if (shootoutDecided()) { err = "the shoot-out is already decided"; return false; }
    if (side != nextKickSide()) { err = std::string("it is ") + (nextKickSide() == 0 ? "the home" : "the away") + " team's kick"; return false; }
    if (!valid(side, player) || status_[side][player] != PITCH) {
        err = "only players on the pitch at the end of extra time may take a kick";
        return false;
    }
    if (!canKick(side, player)) { err = nameOf(side, player) + " cannot kick again until all team-mates have taken one"; return false; }
    kicksBy_[side][player]++;
    taken_[side]++;
    if (scored) pen_[side]++;
    PenaltyKick k;
    k.side = side; k.player = player; k.scored = scored;
    m_.kicks.pushBack(k);
    return true;
}

bool MatchRecorder::shootoutDecided() const {
    if (!shootout_) return false;
    if (taken_[0] <= 5 && taken_[1] <= 5 && !(taken_[0] == 5 && taken_[1] == 5)) {
        int rem0 = 5 - taken_[0], rem1 = 5 - taken_[1];
        if (pen_[0] > pen_[1] + rem1) return true;
        if (pen_[1] > pen_[0] + rem0) return true;
        return false;
    }
    return taken_[0] == taken_[1] && pen_[0] != pen_[1];
}

int MatchRecorder::shootoutWinner() const { return pen_[0] > pen_[1] ? 0 : 1; }

// ---------------------------------------------------------------------------
//  Final whistle
// ---------------------------------------------------------------------------
void MatchRecorder::finish() {
    if (finished_) return;
    int end = abandoned_ ? minute_ : (et_ ? 120 : 90);
    for (int s = 0; s < 2; ++s)
        for (int p = 0; p < t_[s]->squad.size(); ++p)
            if (status_[s][p] == PITCH) exit_[s][p] = end;

    m_.hg = score_[0];
    m_.ag = score_[1];
    if (abandoned_) {
        // Awarded 3-0 to the non-offending side unless it was already ahead by more.
        int winner = 1 - m_.forfeitSide;
        int wg = score_[winner], lg = score_[m_.forfeitSide];
        if (wg - lg < 3) { wg = 3; lg = 0; }
        m_.hg = winner == 0 ? wg : lg;
        m_.ag = winner == 0 ? lg : wg;
    }
    m_.abandoned = abandoned_;
    m_.extraTime = et_;
    m_.shootout = shootout_;
    m_.penH = pen_[0];
    m_.penA = pen_[1];
    finished_ = true;
}
