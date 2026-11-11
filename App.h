#pragma once
// ============================================================================
//  App - menu-driven console front end.
// ============================================================================
class Season;

class App {
public:
    ~App();
    int run();

private:
    bool startMenu();
    void mainMenu();
    void header();
    void flushNews(bool playerNews = true);

    // play
    void playNext();
    void playMatchday();
    void playUntil(int phaseLimit, bool showEachMatchday);
    void showAfterMatch(int matchId, bool showTable);

    // views
    void fixturesMenu();
    void matchCentre();
    void teamsMenu();
    void playersMenu();
    void bracketMenu();
    void predictionsMenu();
    void statsMenu();
    void reportsMenu();
    void saveMenu();
    void loadMenu();
    void settingsMenu();

    // predictions
    void whatIf();
    void predictionGame();

    int pickTeam(const char* prompt);
    bool newSeason(unsigned long long seed);

    Season* s_ = nullptr;
};
