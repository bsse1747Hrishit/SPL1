#pragma once
// ============================================================================
//  Knockout bracket as a BINARY TREE
//
//  Leaves are teams; every internal node is a tie whose two children feed
//  it. Teams ranked 1-8 receive a BYE: their leaf hangs directly under a
//  Round-of-16 node, while teams 9-24 enter one level lower through a
//  play-off tie. 25-36 are eliminated.
//
//                    FINAL
//            SF                SF
//        QF      QF        QF      QF
//      R16 R16 R16 R16  R16 R16 R16 R16
//     /  \   <- each R16 tie
//  seed  PO tie        (seed = bye leaf, PO = play-off node with 2 leaves)
//
//  UEFA seeding: 9/10 v 23/24, 11/12 v 21/22, 13/14 v 19/20, 15/16 v 17/18;
//  the winners face 7/8, 5/6, 3/4, 1/2 respectively. The two teams of every
//  seeded pair go to opposite halves, so 1 and 2 can only meet in the final.
//  The better-ranked team plays the second leg at home.
//
//  Nodes live in a fixed pool (no pointers) so the bracket is trivially
//  copyable - the Monte-Carlo engine copies it thousands of times.
// ============================================================================
#include <iostream>
#include "DynArray.h"
#include "Random.h"
#include "Match.h"

struct BracketNode {
    bool leaf = false;
    Stage stage = Stage::FINAL;
    int team = -1;          // leaf only
    int left = -1, right = -1, parent = -1;
    int winner = -1;        // decided tie
    int leg1 = -1, leg2 = -1;   // match ids (final: leg1 only)
};

class Bracket {
public:
    static const int MAXN = 64;

    void clear();
    // ranking: team ids, best first (at least 24 entries)
    void build(const int* ranking, int count, ds::Random& rng);
    bool built() const { return root_ >= 0; }

    int root() const { return root_; }
    BracketNode& node(int i) { return nodes_[i]; }
    const BracketNode& node(int i) const { return nodes_[i]; }

    // Team in slot 0 (left) or 1 (right) of a tie, -1 if not yet known.
    int participant(int tie, int slot) const;
    // Lower league position is better (0-based rank, 99 = unranked)
    int rankOf(int team) const { return team >= 0 && team < 64 ? rank_[team] : 99; }
    int betterRanked(int a, int b) const { return rankOf(a) <= rankOf(b) ? a : b; }

    // Ties of one stage in bracket order (left to right, in-order traversal)
    void tiesAtStage(Stage s, ds::DynArray<int>& out) const;
    // Breadth-first traversal (root first) using our Queue - for level views
    void levelOrder(ds::DynArray<int>& out) const;
    int depth() const;

private:
    int newLeaf(int team);
    int newTie(Stage s, int l, int r);
    void inorder(int n, Stage s, ds::DynArray<int>& out) const;

    BracketNode nodes_[MAXN];
    int count_ = 0;
    int root_ = -1;
    int rank_[64];
};
