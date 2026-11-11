#include "Elo.h"
#include <cmath>

namespace elo {

double expectedScore(double eloHome, double eloAway, bool neutral) {
    double dr = eloHome + (neutral ? 0.0 : HOME_ADVANTAGE) - eloAway;
    return 1.0 / (1.0 + std::pow(10.0, -dr / 400.0));
}

void expectedGoals(double eloHome, double eloAway, bool neutral, double& lh, double& la) {
    double dr = eloHome + (neutral ? 0.0 : HOME_ADVANTAGE) - eloAway;
    lh = BASE_GOALS * std::pow(10.0, dr / 1000.0);
    la = BASE_GOALS * std::pow(10.0, -dr / 1000.0);
    if (lh < 0.15) lh = 0.15;
    if (la < 0.15) la = 0.15;
    if (lh > 5.0) lh = 5.0;
    if (la > 5.0) la = 5.0;
}

double poissonPmf(int k, double lambda) {
    double p = std::exp(-lambda);
    for (int i = 1; i <= k; ++i) p *= lambda / i;
    return p;
}

Outcome predict(double eloHome, double eloAway, bool neutral) {
    Outcome o;
    expectedGoals(eloHome, eloAway, neutral, o.xgHome, o.xgAway);
    const int MAXG = 10;
    double ph[MAXG + 1], pa[MAXG + 1];
    for (int g = 0; g <= MAXG; ++g) {
        ph[g] = poissonPmf(g, o.xgHome);
        pa[g] = poissonPmf(g, o.xgAway);
    }
    for (int h = 0; h <= MAXG; ++h) {
        for (int a = 0; a <= MAXG; ++a) {
            double p = ph[h] * pa[a];
            if (h > a) o.home += p;
            else if (h == a) o.draw += p;
            else o.away += p;
            if (h + a >= 3) o.over25 += p;
            if (h > 0 && a > 0) o.bothScore += p;
            if (p > o.likelyP) { o.likelyP = p; o.likelyH = h; o.likelyA = a; }
        }
    }
    double total = o.home + o.draw + o.away;
    o.home /= total; o.draw /= total; o.away /= total;
    return o;
}

double update(double& eloHome, double& eloAway, int hg, int ag, bool neutral) {
    double e = expectedScore(eloHome, eloAway, neutral);
    double w = hg > ag ? 1.0 : (hg == ag ? 0.5 : 0.0);
    int gd = hg > ag ? hg - ag : ag - hg;
    double g = gd <= 1 ? 1.0 : (gd == 2 ? 1.5 : (11.0 + gd) / 8.0);
    double delta = K_FACTOR * g * (w - e);
    eloHome += delta;
    eloAway -= delta;
    return delta;
}

}  // namespace elo
