#include "Bracket.h"
#include "Queue.h"

void Bracket::clear() {
    count_ = 0;
    root_ = -1;
    for (int i = 0; i < 64; ++i) rank_[i] = 99;
    for (int i = 0; i < MAXN; ++i) nodes_[i] = BracketNode();
}

int Bracket::newLeaf(int team) {
    BracketNode& n = nodes_[count_];
    n = BracketNode();
    n.leaf = true;
    n.team = team;
    n.winner = team;
    return count_++;
}

int Bracket::newTie(Stage s, int l, int r) {
    BracketNode& n = nodes_[count_];
    n = BracketNode();
    n.stage = s;
    n.left = l;
    n.right = r;
    nodes_[l].parent = count_;
    nodes_[r].parent = count_;
    return count_++;
}

void Bracket::build(const int* ranking, int count, ds::Random& rng) {
    clear();
    for (int i = 0; i < count && i < 64; ++i) rank_[ranking[i]] = i;

    // ---- Knockout play-offs: k=0: 9/10 v 23/24 ... k=3: 15/16 v 17/18
    int po[4][2];
    for (int k = 0; k < 4; ++k) {
        int seedA = ranking[8 + 2 * k], seedB = ranking[9 + 2 * k];
        int unA = ranking[22 - 2 * k], unB = ranking[23 - 2 * k];
        int coin = rng.range(0, 1);
        int opA = coin ? unA : unB;
        int opB = coin ? unB : unA;
        po[k][0] = newTie(Stage::PLAYOFF, newLeaf(seedA), newLeaf(opA));
        po[k][1] = newTie(Stage::PLAYOFF, newLeaf(seedB), newLeaf(opB));
    }

    // ---- Round of 16: seeds (1,2) meet PO group k=3, (3,4) k=2, ...
    int half[2][4];
    for (int g = 0; g < 4; ++g) {
        int k = 3 - g;
        int coin = rng.range(0, 1);
        int tieA = newTie(Stage::R16, newLeaf(ranking[2 * g]), po[k][coin]);
        int tieB = newTie(Stage::R16, newLeaf(ranking[2 * g + 1]), po[k][1 - coin]);
        int side = rng.range(0, 1);             // pair split across halves
        half[side][g] = tieA;
        half[1 - side][g] = tieB;
    }

    // ---- Quarter-finals, semi-finals, final
    int sf[2];
    for (int h = 0; h < 2; ++h) {
        int qfA = newTie(Stage::QF, half[h][0], half[h][3]);   // 1/2 path v 7/8 path
        int qfB = newTie(Stage::QF, half[h][1], half[h][2]);   // 3/4 path v 5/6 path
        sf[h] = newTie(Stage::SF, qfA, qfB);
    }
    root_ = newTie(Stage::FINAL, sf[0], sf[1]);
}

int Bracket::participant(int tie, int slot) const {
    const BracketNode& n = nodes_[tie];
    int c = slot == 0 ? n.left : n.right;
    if (c < 0) return -1;
    return nodes_[c].winner;
}

void Bracket::inorder(int n, Stage s, ds::DynArray<int>& out) const {
    if (n < 0) return;
    const BracketNode& node = nodes_[n];
    if (node.leaf) return;
    inorder(node.left, s, out);
    if (node.stage == s) out.push(n);
    inorder(node.right, s, out);
}

void Bracket::tiesAtStage(Stage s, ds::DynArray<int>& out) const {
    out.clear();
    inorder(root_, s, out);
}

void Bracket::levelOrder(ds::DynArray<int>& out) const {
    out.clear();
    if (root_ < 0) return;
    ds::Queue<int> q;
    q.enqueue(root_);
    while (!q.empty()) {
        int n = q.dequeue();
        out.push(n);
        if (!nodes_[n].leaf) {
            q.enqueue(nodes_[n].left);
            q.enqueue(nodes_[n].right);
        }
    }
}

int Bracket::depth() const {
    // Longest root-to-leaf path, via BFS levels
    if (root_ < 0) return 0;
    ds::Queue<int> q;
    q.enqueue(root_);
    int levels = 0;
    while (!q.empty()) {
        int width = q.size();
        ++levels;
        for (int i = 0; i < width; ++i) {
            int n = q.dequeue();
            if (!nodes_[n].leaf) { q.enqueue(nodes_[n].left); q.enqueue(nodes_[n].right); }
        }
    }
    return levels;
}
