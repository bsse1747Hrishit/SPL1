#pragma once
// ============================================================================
//  MatchRecorder - the live rule engine of a match.
//
//  EVERY event (from the simulator, from a user typing live, or replayed from
//  a save file) goes through this class, which enforces the laws:
//   * a starting XI has 11 different, available players incl. exactly 1 GK
//     (suspended / injured players are rejected)
//   * the bench has at most 12 players
//   * minutes never go backwards; 1-90 in normal time, 91-120 in extra time
//   * only players on the pitch can score, assist, be booked or get injured
//   * a second yellow becomes a red automatically; sent-off players leave
//     and cannot be replaced
//   * max 5 substitutes in 3 windows (half-time does not count); one extra
//     substitute and window in extra time; a player taken off cannot return
//   * a team reduced below 7 players => match abandoned, awarded 3-0
//   * shoot-out: only players on the pitch at the end, alternate kicks,
//     nobody kicks twice before all team-mates kicked once, best of 5 then
//     sudden death, stops as soon as the result is decided
// ============================================================================
#include <string>
#include "DynArray.h"
#include "Match.h"
#include "Team.h"

class MatchRecorder {
public:
    enum Status { NONE = 0, PITCH = 1, BENCH = 2, SUBBED_OFF = 3, SENT_OFF = 4 };
    static const int MAXP = 32;

    MatchRecorder(Match& m, Team& home, Team& away);

    // Validates and applies both line-ups. Must be called first.
    bool start(const Lineup& home, const Lineup& away, std::string& err);
    static bool validateLineup(const Team& t, const Lineup& l, std::string& err);

    // ---- events (return false + reason when a rule is violated) ----------
    bool goal(int side, int minute, int scorer, int assist, bool penalty, std::string& err);
    bool ownGoal(int playerSide, int minute, int player, std::string& err);
    bool penaltyMiss(int side, int minute, int player, std::string& err);
    bool yellow(int side, int minute, int player, std::string& err);
    bool red(int side, int minute, int player, int ban, std::string& err);
    bool sub(int side, int minute, int out, int in, std::string& err);
    bool injury(int side, int minute, int player, int matchesOut, std::string& err);
    bool apply(const MatchEvent& e, std::string& err);   // dispatcher (replay)

    // Lets the clock run without events (used by the simulator).
    void setClock(int minute);

    bool startExtraTime(std::string& err);
    bool beginShootout(std::string& err);
    bool penaltyKick(int side, int player, bool scored, std::string& err);
    bool shootoutDecided() const;
    int shootoutWinner() const;          // side
    int nextKickSide() const;
    // Eligible kicker check without recording
    bool canKick(int side, int player) const;

    void finish();                       // final whistle

    // ---- queries ----------------------------------------------------------
    Match& match() { return m_; }
    const Match& match() const { return m_; }
    Team& team(int side) { return *t_[side]; }
    const Team& team(int side) const { return *t_[side]; }
    int score(int side) const { return score_[side]; }
    int minute() const { return minute_; }
    bool started() const { return started_; }
    bool finished() const { return finished_; }
    bool inExtraTime() const { return et_; }
    bool inShootout() const { return shootout_; }
    bool isAbandoned() const { return abandoned_; }
    int status(int side, int p) const { return status_[side][p]; }
    bool onPitch(int side, int p) const { return valid(side, p) && status_[side][p] == PITCH; }
    bool wasStarter(int side, int p) const { return starter_[side][p]; }
    int entered(int side, int p) const { return enter_[side][p]; }   // -1 never played
    int exited(int side, int p) const { return exit_[side][p]; }
    int yellowCount(int side, int p) const { return yc_[side][p]; }
    bool injuredInMatch(int side, int p) const { return injuredNow_[side][p]; }
    int playersOnPitch(int side) const;
    void pitchPlayers(int side, ds::DynArray<int>& out) const;
    void benchPlayers(int side, ds::DynArray<int>& out) const;  // unused subs
    int subsUsed(int side) const { return subs_[side]; }
    int maxSubs() const { return et_ ? 6 : 5; }
    int maxWindows() const { return et_ ? 4 : 3; }
    int windowsUsed(int side) const { return windows_[side]; }
    bool canSubAt(int side, int minute) const;
    int penScore(int side) const { return pen_[side]; }
    int penTaken(int side) const { return taken_[side]; }

private:
    bool valid(int side, int p) const { return side >= 0 && side < 2 && p >= 0 && p < t_[side]->squad.size(); }
    bool live(std::string& err) const;
    bool checkMinute(int minute, std::string& err) const;
    bool requirePitch(int side, int p, std::string& err) const;
    void sendOff(int side, int p, int minute);
    void push(EventType t, int minute, int side, int p, int p2, int extra);
    const std::string& nameOf(int side, int p) const { return t_[side]->squad[p].name; }

    Match& m_;
    Team* t_[2];
    int status_[2][MAXP];
    bool starter_[2][MAXP];
    int enter_[2][MAXP];
    int exit_[2][MAXP];
    int yc_[2][MAXP];
    bool injuredNow_[2][MAXP];
    int kicksBy_[2][MAXP];
    int subs_[2], windows_[2], lastWindow_[2], score_[2], pen_[2], taken_[2];
    int minute_ = 0;
    bool started_ = false, finished_ = false, et_ = false, shootout_ = false, abandoned_ = false;
};
