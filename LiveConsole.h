#pragma once
// ============================================================================
//  Interactive match modes
//   * Live recording: the user types events as they happen (goals, cards,
//     subs, injuries, shoot-out kicks). The MatchRecorder enforces every
//     rule immediately and rejects illegal input with an explanation. The
//     simulator can take over at any moment ("auto").
//   * Quick score: the user only enters the score; scorers, assists,
//     cards and substitutions are generated consistently.
// ============================================================================
class Season;
struct Match;
struct Lineup;
struct Team;

namespace ui {

bool chooseLineup(const Team& t, Lineup& out);   // interactive or auto
bool runLiveMatch(Season& s, Match& m);          // returns true when committed
bool runQuickScore(Season& s, Match& m);

}  // namespace ui
