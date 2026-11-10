# UEFA Champions League Simulator & Analytics

A C++17 console application that simulates and manages a complete UEFA
Champions League season for the 36 clubs of the 2025/26 league phase. It covers the
"Swiss model" league phase draw, every match event, the league table, the
knockout bracket, the final, Elo predictions, Monte-Carlo projections, reports
and save/load.

It uses no STL containers. Every data structure comes from a small custom
library (`include/ds/`): dynamic array, linked list, hash table, queue, heap
and merge sort.

---

## 1. Build & run

You need any C++17 compiler (g++ 7+, clang 5+, MSVC 2017+).

```bash
make
```

```bash
./ucl_sim
```

With CMake instead:

```bash
cmake -S . -B build && cmake --build build
```

```bash
./build/ucl_sim
```

To run the self-test suite (65 checks: data structures, draw rules, the rule engine,
tiebreakers, 12 full seasons and save/load fidelity):

```bash
make test
```

---

## 2. A 2-minute tour

1. Start a new season. Enter any number as the **seed** (the same seed always
   gives the same draw), or leave it blank for a random one.
2. **Main menu → 1: Play next match.** You first see the Elo prediction, then you choose:
   * **Simulate**: the minute-by-minute engine plays the match.
   * **Live record**: you type the match events yourself (see section 3).
   * **Quick score**: you type only the result. Scorers, assists, cards and
     subs are generated to fit it.
3. The **league table** appears after every match (you can change this in Settings).
4. **2 / 3 / 4** simulate the rest of the matchday, the league phase, or the
   whole season.
5. After matchday 8 the **knockout bracket** is drawn automatically
   (menu 10 shows it as a tree).
6. **11: Predictions** has match odds, Monte-Carlo title odds, *what-if*
   scenarios and a prediction game against the Elo model.
7. **13** exports text reports to `reports/`. **14 / 15** save and load.

---

## 3. Live (dynamic) match recording

Players are referenced by **shirt number**. `h` means home and `a` means away.

| Command | Meaning | Example |
|---|---|---|
| `g <side> <min> <scorer> [assist]` | goal | `g h 23 9 17` |
| `p <side> <min> <scorer>` | penalty scored | `p a 55 10` |
| `pm <side> <min> <taker>` | penalty missed | `pm h 70 7` |
| `og <side> <min> <player>` | own goal **by** a player of `<side>` | `og a 80 4` |
| `y <side> <min> <player>` | yellow card (a 2nd yellow becomes a red automatically) | `y a 30 5` |
| `r <side> <min> <player> [ban]` | straight red card (1–5 match ban) | `r h 44 3 2` |
| `s <side> <min> <off> <on>` | substitution (46 = half-time) | `s a 60 15 28` |
| `i <side> <min> <player> <matches>` | injury | `i h 80 4 3` |
| `auto [min]` | let the simulator take over (to a minute, or to the end) | `auto 75` |
| `ft` | full-time whistle. Leads to extra time and penalties if a knockout tie is level | |
| `l` / `e` / `?` | show the pitch & bench / the timeline / help | |

Each command is checked **as you type it**, and an illegal one is rejected with
the reason, for example: *"REJECTED: all 3 substitution windows used"*.

---

## 4. Rules that are enforced

**Squads (validated at start-up)**
* Each squad has 16–25 players (UEFA List A), with unique shirt numbers and names.
* Ratings must be between 40 and 99.
* Each squad has at least 2 GK, 5 DF, 5 MF and 3 FW. Missing places are filled with academy players.
* Emergency registration is automatic: a replacement goalkeeper when none is
  fit, and extra players when fewer than 13 are available.

**League-phase draw**
* 4 pots of 9 clubs. Each club plays 8 different opponents: 2 from every pot, one at home and one away.
* No club meets a club from its own country.
* No club meets more than 2 clubs from any one country.
* Each matchday every club plays exactly once. No club plays 3 home or 3 away matches in a row.

**Matches**
* The XI has 11 different players and exactly one goalkeeper.
  **Suspended and injured players are blocked.**
* Minutes never go backwards: 1–90 in normal time and 91–120 in extra time.
* Only players on the pitch can score, assist, be booked or get injured.
* A second yellow is an automatic red. A sent-off player cannot be replaced.
* Each team may use 5 substitutes in 3 windows (half-time does not count as a window).
  Extra time adds one more substitute and one more window. A player who is taken off cannot return.
* A team with fewer than 7 players means the match is abandoned and awarded 3-0.

**Discipline**
* The 3rd, 5th, 7th… yellow card gives a 1-match ban.
* A second yellow gives a 1-match ban.
* A straight red gives a 1–5 match ban.
* Yellow cards are wiped after the quarter-finals.

**Table tiebreakers**, in order: points → goal difference → goals scored →
head-to-head (a mini-table between the tied clubs) → alphabetical.

**Knockouts**
* Places 1–8 get a bye to the Round of 16. Places 9–24 play the knockout play-offs. Places 25–36 are eliminated.
* Play-off seeding: 9/10 v 23/24, 11/12 v 21/22, 13/14 v 19/20 and 15/16 v 17/18.
  The winners meet 7/8, 5/6, 3/4 and 1/2 in the Round of 16.
* The clubs ranked 1st and 2nd go into opposite halves of the bracket.
* Ties are two legs, and the better-ranked club plays the second leg at home.
* If the aggregate score is level: extra time, then a penalty shoot-out.
* Shoot-out rules: kicks alternate, it is best of 5 and then sudden death, and nobody kicks twice
  before all team-mates have kicked once.
* The final is a single match at a neutral venue.

---

## 5. Prediction & analytics

* **Elo model.** Each team's chance of winning comes from the gap between the two Elo ratings
  (`E = 1/(1+10^(-dr/400))`, with home advantage 60).
  Expected goals are `1.4·10^(±dr/1000)`. Poisson score probabilities give
  the home / draw / away odds, the most likely score, over 2.5 goals and both teams scoring.
  Ratings update after every match (K = 30, scaled by the goal difference).
  The same model drives the match engine.
* **Monte-Carlo.** The rest of the tournament is simulated N times (2000 by default).
  You get each team's chances of finishing in the top 8, the play-offs (9–24) or
  eliminated (25–36), of reaching the R16, QF, SF and final, and of winning the title.
* **What-if.** Force scores for upcoming matches and see the aftermath:
  before/after odds for every team, compared using common random numbers.
* **Prediction game.** You predict the scores, the matches are played, and you
  get 3 points for an exact score and 1 for the right outcome. The Elo model plays the same game.
* **Statistics:**
  * player leaderboards (goals, assists, G+A, clean sheets, minutes, cards, impact index, goals/90)
  * player profiles
  * mean, standard deviation and median goals per match
  * a goals histogram and when goals are scored
  * home/draw/away split
  * performance by country
  * the Elo power ranking

---

## 6. Project structure

| Module | Files | Concepts |
|---|---|---|
| Data-structure library | `include/ds/*.h` | Array, list, **hash table**, queue, **heap**, **merge sort**, PRNG |
| Teams & players | `src/model/` | OOP, validation, embedded 36-club database |
| Fixture generator | `src/league/LeagueDraw.*` | **Graphs**: bipartite matchings for the draw and perfect matchings for the matchdays, backtracking |
| Match engine | `src/match/` | Rule-enforcing recorder, minute-by-minute simulator |
| Points table | `src/league/Standings.*` | Sorting on several keys, head-to-head kept in a hash table |
| Player statistics | `src/stats/` | Heap Top-N, merge-sort rankings, descriptive statistics |
| Elo & Monte-Carlo | `src/predict/` | Probability models, simulation |
| Knockout bracket | `src/knockout/` | **Binary tree**, breadth-first traversal with the queue |
| Season flow | `src/season/` | Stage machine, discipline and injury bookkeeping |
| Reports | `src/reports/` | Each report prints to the console and to files |
| Save / Load | `src/io/` | The event log is replayed through the rule engine |
| Console UI | `src/ui/` | Menus, live recorder |

**How save/load works.** A save file stores the seed and the event log of every
played match. Loading rebuilds the season from the seed and replays every event
through the same rule engine. Statistics, bans, injuries, Elo ratings and the
bracket come back exactly as they were (the test suite checks this byte-for-byte). The file
is plain text you can read.

---

## 7. About the data

Clubs, pots, countries and stadiums are those of the **2025/26 league phase**.
Squads, shirt numbers, player ratings and Elo values are an **approximate snapshot from early
in the season**, entered by hand. They are good enough for simulation, but they are not an official
record, especially for the smaller clubs. To use your own data:
**Teams & squads → Export team database** writes `teams.txt`. Edit it and
restart: if `teams.txt` exists, it is loaded instead of the built-in data, and it goes through the same
validation.
