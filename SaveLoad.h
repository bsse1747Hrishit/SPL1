#pragma once
// ============================================================================
//  Save / Load
//  A save file stores the seed, settings, RNG state and the complete event
//  log of every played match (line-ups, events, shoot-out kicks) in play
//  order. Loading rebuilds the season from the seed and REPLAYS each match
//  through the MatchRecorder, so all rules are re-validated and every
//  statistic, ban, injury, Elo rating and bracket slot is reconstructed
//  exactly. The file is plain text and human-readable.
// ============================================================================
#include <string>

class Season;

namespace saveload {

bool save(const Season& s, const std::string& path, std::string& err);
// Returns a newly allocated season (caller owns it) or nullptr on error.
Season* load(const std::string& path, std::string& err);

}  // namespace saveload
