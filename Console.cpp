#include "Console.h"
#include <cstdio>
#include <iostream>

namespace ui {

bool readLine(std::string& out) {
    if (!std::getline(std::cin, out)) return false;
    if (!out.empty() && out.back() == '\r') out.pop_back();
    return true;
}

std::string ask(const std::string& prompt) {
    std::cout << prompt << std::flush;
    std::string line;
    if (!readLine(line)) throw InputClosed();
    return trim(line);
}

int askInt(const std::string& prompt, int lo, int hi, int defaultValue) {
    for (;;) {
        std::string s = ask(prompt);
        if (s.empty() && defaultValue >= lo && defaultValue <= hi) return defaultValue;
        int v;
        if (parseInt(s, v) && v >= lo && v <= hi) return v;
        std::cout << "  Please enter a number between " << lo << " and " << hi << ".\n";
    }
}

bool askYesNo(const std::string& prompt, bool defaultValue) {
    for (;;) {
        std::string s = lower(ask(prompt + (defaultValue ? " [Y/n]: " : " [y/N]: ")));
        if (s.empty()) return defaultValue;
        if (s == "y" || s == "yes") return true;
        if (s == "n" || s == "no") return false;
    }
}

std::string trim(const std::string& s) {
    size_t a = 0, b = s.size();
    while (a < b && (s[a] == ' ' || s[a] == '\t')) ++a;
    while (b > a && (s[b - 1] == ' ' || s[b - 1] == '\t')) --b;
    return s.substr(a, b - a);
}

std::string lower(const std::string& s) {
    std::string r = s;
    for (char& c : r) if (c >= 'A' && c <= 'Z') c = static_cast<char>(c - 'A' + 'a');
    return r;
}

bool parseInt(const std::string& s, int& out) {
    if (s.empty() || s.size() > 9) return false;
    int i = 0, v = 0;
    bool neg = false;
    if (s[0] == '-' || s[0] == '+') { neg = s[0] == '-'; i = 1; if (s.size() == 1) return false; }
    for (; i < static_cast<int>(s.size()); ++i) {
        if (s[i] < '0' || s[i] > '9') return false;
        v = v * 10 + (s[i] - '0');
    }
    out = neg ? -v : v;
    return true;
}

int displayWidth(const std::string& s) {
    int w = 0;
    for (unsigned char c : s) if ((c & 0xC0) != 0x80) ++w;   // count UTF-8 lead bytes
    return w;
}

std::string padR(const std::string& s, int width) {
    int w = displayWidth(s);
    if (w >= width) {
        // truncate on a character boundary
        std::string r;
        int count = 0;
        for (size_t i = 0; i < s.size(); ++i) {
            unsigned char c = static_cast<unsigned char>(s[i]);
            if ((c & 0xC0) != 0x80) { if (count == width) break; ++count; }
            r += s[i];
        }
        return r;
    }
    return s + std::string(width - w, ' ');
}

std::string padL(const std::string& s, int width) {
    int w = displayWidth(s);
    if (w >= width) return s;
    return std::string(width - w, ' ') + s;
}

std::string fixed(double v, int decimals) {
    char buf[64];
    std::snprintf(buf, sizeof buf, "%.*f", decimals, v);
    return buf;
}

std::string pct(double p) {
    if (p > 0 && p < 0.001) return "  <0.1%";
    char buf[32];
    std::snprintf(buf, sizeof buf, "%6.1f%%", p * 100.0);
    return buf;
}

std::string signedNum(int v) { return v > 0 ? "+" + std::to_string(v) : std::to_string(v); }

std::string bar(double f, int width) {
    if (f < 0) f = 0;
    if (f > 1) f = 1;
    int n = static_cast<int>(f * width + 0.5);
    return std::string(n, '#') + std::string(width - n, '.');
}

std::string rule(char c, int width) { return std::string(width, c); }

std::string banner(const std::string& title) {
    std::string line = rule('=');
    int pad = (78 - displayWidth(title)) / 2;
    if (pad < 0) pad = 0;
    return line + "\n" + std::string(pad, ' ') + title + "\n" + line + "\n";
}

}  // namespace ui
