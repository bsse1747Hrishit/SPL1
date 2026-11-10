#include "Standings.h"
#include "MergeSort.h"

void TableRow::pushForm(char c) {
    int len = 0;
    while (len < 5 && form[len]) ++len;
    if (len == 5) {
        for (int i = 0; i < 4; ++i) form[i] = form[i + 1];
        form[4] = c;
    } else {
        form[len] = c;
    }
}

void Standings::reset(int teams) {
    rows_.clear();
    for (int i = 0; i < teams; ++i) {
        TableRow r;
        r.team = i;
        rows_.push(r);
    }
    h2h_.clear();
}

void Standings::record(int home, int away, int hg, int ag) {
    TableRow& h = rows_[home];
    TableRow& a = rows_[away];
    h.played++; a.played++;
    h.gf += hg; h.ga += ag;
    a.gf += ag; a.ga += hg;
    if (hg > ag)      { h.won++; a.lost++; h.pts += 3; h.pushForm('W'); a.pushForm('L'); }
    else if (hg < ag) { a.won++; h.lost++; a.pts += 3; a.pushForm('W'); h.pushForm('L'); }
    else              { h.drawn++; a.drawn++; h.pts++; a.pts++; h.pushForm('D'); a.pushForm('D'); }

    HeadToHead& rec = h2h_[pairKey(home, away)];
    if (home < away) { rec.goalsLow += hg; rec.goalsHigh += ag; }
    else             { rec.goalsLow += ag; rec.goalsHigh += hg; }
    rec.meetings++;
}

bool Standings::headToHead(int a, int b, int& goalsA, int& goalsB) const {
    const HeadToHead* rec = h2h_.find(pairKey(a, b));
    goalsA = goalsB = 0;
    if (!rec || rec->meetings == 0) return false;
    goalsA = a < b ? rec->goalsLow : rec->goalsHigh;
    goalsB = a < b ? rec->goalsHigh : rec->goalsLow;
    return true;
}

const char* Standings::zone(int position) {
    if (position < 8) return "R16";      // direct to Round of 16
    if (position < 24) return "PO";      // knockout play-off
    return "OUT";                       // eliminated
}

void Standings::resolveTie(ds::DynArray<int>& ids, int from, int to, const ds::DynArray<Team>& teams) const {
    // Mini-table among the tied teams only
    struct Mini { int team; int pts; int gd; int gf; };
    int n = to - from;
    ds::DynArray<Mini> mini;
    for (int i = from; i < to; ++i) {
        Mini m{ids[i], 0, 0, 0};
        for (int j = from; j < to; ++j) {
            if (i == j) continue;
            int ga, gb;
            if (headToHead(ids[i], ids[j], ga, gb)) {
                m.gf += ga;
                m.gd += ga - gb;
                if (ga > gb) m.pts += 3; else if (ga == gb) m.pts += 1;
            }
        }
        mini.push(m);
    }
    ds::mergeSort(mini, [&teams](const Mini& a, const Mini& b) {
        if (a.pts != b.pts) return a.pts > b.pts;
        if (a.gd != b.gd) return a.gd > b.gd;
        if (a.gf != b.gf) return a.gf > b.gf;
        return teams[a.team].name < teams[b.team].name;     // alphabetical
    });
    for (int k = 0; k < n; ++k) ids[from + k] = mini[k].team;
}

ds::DynArray<int> Standings::ranking(const ds::DynArray<Team>& teams) const {
    ds::DynArray<int> ids;
    for (int i = 0; i < rows_.size(); ++i) ids.push(i);
    auto primary = [this](int a, int b) {
        const TableRow& x = rows_[a];
        const TableRow& y = rows_[b];
        if (x.pts != y.pts) return x.pts > y.pts;
        if (x.gd() != y.gd()) return x.gd() > y.gd();
        return x.gf > y.gf;
    };
    ds::mergeSort(ids, primary);
    int i = 0, n = ids.size();
    while (i < n) {
        int j = i + 1;
        while (j < n && !primary(ids[i], ids[j]) && !primary(ids[j], ids[i])) ++j;
        if (j - i > 1) resolveTie(ids, i, j, teams);
        i = j;
    }
    return ids;
}
