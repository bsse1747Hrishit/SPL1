#pragma once
// ============================================================================
//  Console helpers: safe line-based input and column formatting.
// ============================================================================
#include <string>

namespace ui {

// Thrown when standard input is closed (e.g. end of a piped script).
struct InputClosed {};

bool readLine(std::string& out);                      // false on EOF
std::string ask(const std::string& prompt);           // throws InputClosed
int askInt(const std::string& prompt, int lo, int hi, int defaultValue);
bool askYesNo(const std::string& prompt, bool defaultValue);

std::string trim(const std::string& s);
std::string lower(const std::string& s);
bool parseInt(const std::string& s, int& out);

int displayWidth(const std::string& s);               // UTF-8 aware
std::string padR(const std::string& s, int width);    // left-aligned
std::string padL(const std::string& s, int width);    // right-aligned
std::string fixed(double v, int decimals);
std::string pct(double p);                            // 0.4567 -> " 45.7%"
std::string signedNum(int v);                         // +3 / -2 / 0
std::string bar(double fraction, int width);          // ######....

std::string rule(char c = '-', int width = 78);
std::string banner(const std::string& title);

}  // namespace ui
