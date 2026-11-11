#pragma once
// ============================================================================
//  Feature switches
//  Set a value to true and rebuild to show that feature again. Menus are
//  numbered automatically, so no other code needs to change.
// ============================================================================

namespace features {

// Main menu "Match centre": full match report with line-ups and timeline
constexpr bool MATCH_CENTRE = false;

// Main menu "Player statistics & rankings": leaderboards and player profiles
constexpr bool PLAYER_STATS = false;

// Main menu "Knockout bracket": tree view and round-by-round bracket
constexpr bool KNOCKOUT_BRACKET = false;

// Main menu "Predictions": Elo odds, Monte-Carlo, what-if, prediction game
// (also hides the Elo forecast shown before each match)
constexpr bool PREDICTIONS = false;

}  // namespace features
