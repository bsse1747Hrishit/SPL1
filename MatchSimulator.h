#pragma once
// ============================================================================
//  MatchSimulator - minute-by-minute event simulation.
//  It never edits a match directly: it only *proposes* events to the
//  MatchRecorder, so the simulator obeys exactly the same rules as a human
//  recording a match live. It can take over at any minute ("auto" command).
// ============================================================================
#include "Random.h"
#include "MatchRecorder.h"

class MatchSimulator {
public:
    explicit MatchSimulator(ds::Random& rng) : rng_(rng) {}

    // Best available 4-3-3 + bench (skips suspended / injured players).
    static Lineup autoLineup(const Team& t);

    // Simulates from the recorder's current minute up to `toMinute`.
    void simulateTo(MatchRecorder& rec, int toMinute);
    // Plays a complete penalty shoot-out.
    void simulateShootout(MatchRecorder& rec);
    // Generates a plausible timeline for a known 90-minute score.
    void quickScore(MatchRecorder& rec, int homeGoals, int awayGoals, int fromMinute, int toMinute);

    // Weighted random choices among players on the pitch (public for reuse)
    int pickScorer(const MatchRecorder& rec, int side, int exclude = -1);
    int pickAssister(const MatchRecorder& rec, int side, int scorer);
    int pickCardPlayer(const MatchRecorder& rec, int side);

private:
    void goalEvent(MatchRecorder& rec, int side, int minute);
    void aiSubstitutions(MatchRecorder& rec, int side, int minute);
    void goalRates(const MatchRecorder& rec, double& perMinHome, double& perMinAway) const;
    int bestReplacement(const MatchRecorder& rec, int side, int outPlayer) const;

    ds::Random& rng_;
};
