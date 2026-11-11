#pragma once
// ============================================================================
//  Elo rating model
//   * Expected result   E = 1 / (1 + 10^(-(Rh + H - Ra) / 400))
//   * Rating update     R' = R + K * G * (W - E)
//       K = 30, G = goal-difference multiplier (1, 1.5, (11+gd)/8)
//   * Goal expectation  lambda = 1.40 * 10^(+-dr / 1000) (Poisson scoring)
//     so the same model drives the predictions AND the match simulator.
// ============================================================================

namespace elo {

const double HOME_ADVANTAGE = 60.0;
const double K_FACTOR = 30.0;
const double BASE_GOALS = 1.40;

double expectedScore(double eloHome, double eloAway, bool neutral);
void expectedGoals(double eloHome, double eloAway, bool neutral, double& lambdaHome, double& lambdaAway);
double poissonPmf(int k, double lambda);

struct Outcome {
    double home = 0, draw = 0, away = 0;   // probabilities
    double xgHome = 0, xgAway = 0;        // expected goals
    int likelyH = 0, likelyA = 0;         // most likely score
    double likelyP = 0;
    double over25 = 0, bothScore = 0;
};
Outcome predict(double eloHome, double eloAway, bool neutral);

// Updates both ratings in place; returns the change for the home team.
double update(double& eloHome, double& eloAway, int hg, int ag, bool neutral);

}  // namespace elo
