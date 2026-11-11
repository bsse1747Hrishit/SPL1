#include "Match.h"

const char* stageName(Stage s) {
    switch (s) {
        case Stage::LEAGUE:  return "League Phase";
        case Stage::PLAYOFF: return "Knockout Play-off";
        case Stage::R16:     return "Round of 16";
        case Stage::QF:      return "Quarter-final";
        case Stage::SF:      return "Semi-final";
        case Stage::FINAL:   return "Final";
    }
    return "?";
}

const char* stageShort(Stage s) {
    switch (s) {
        case Stage::LEAGUE:  return "LP";
        case Stage::PLAYOFF: return "PO";
        case Stage::R16:     return "R16";
        case Stage::QF:      return "QF";
        case Stage::SF:      return "SF";
        case Stage::FINAL:   return "F";
    }
    return "?";
}

const char* eventTag(EventType t) {
    switch (t) {
        case EventType::GOAL:          return "GOAL";
        case EventType::PEN_GOAL:      return "PEN";
        case EventType::OWN_GOAL:      return "OG";
        case EventType::PEN_MISS:      return "MISS";
        case EventType::YELLOW:        return "YC";
        case EventType::SECOND_YELLOW: return "2YC";
        case EventType::RED:           return "RC";
        case EventType::SUB:           return "SUB";
        case EventType::INJURY:        return "INJ";
    }
    return "?";
}

const char* modeName(PlayMode m) {
    switch (m) {
        case PlayMode::NONE:      return "-";
        case PlayMode::SIMULATED: return "Simulated";
        case PlayMode::LIVE:      return "Live-recorded";
        case PlayMode::QUICK:     return "Quick score";
    }
    return "?";
}
