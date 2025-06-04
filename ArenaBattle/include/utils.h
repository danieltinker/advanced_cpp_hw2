#pragma once

#include <string>
#include <sstream>
#include <iostream>
#include <cctype>
#include "common/ActionRequest.h"
/*
  Utility: parse lines like "Key = Value" and convert Value → size_t.
*/
namespace arena {
inline bool parseKeyValue(const std::string& line,
                          const std::string& key,
                          std::size_t& outVar)
{
    auto pos = line.find('=');
    if (pos == std::string::npos) return false;
    std::string lhs = line.substr(0, pos);
    std::string rhs = line.substr(pos + 1);

    auto trim = [](std::string& s) {
        std::size_t a = 0;
        while (a < s.size() && std::isspace(static_cast<unsigned char>(s[a]))) ++a;
        std::size_t b = s.size();
        while (b > a && std::isspace(static_cast<unsigned char>(s[b - 1]))) --b;
        s = s.substr(a, b - a);
    };
    trim(lhs);
    trim(rhs);
    if (lhs != key) return false;
    try {
        outVar = static_cast<std::size_t>(std::stoul(rhs));
        return true;
    } catch (...) {
        return false;
    }
}

/*
  Convert an ActionRequest enum to exactly the string the output log expects.
*/
inline std::string actionToString(common::ActionRequest a) {
    switch (a) {
        case common::ActionRequest::MoveForward:   return "MoveForward";
        case common::ActionRequest::MoveBackward:  return "MoveBackward";
        case common::ActionRequest::RotateLeft90:  return "RotateLeft90";
        case common::ActionRequest::RotateRight90: return "RotateRight90";
        case common::ActionRequest::RotateLeft45:  return "RotateLeft45";
        case common::ActionRequest::RotateRight45: return "RotateRight45";
        case common::ActionRequest::Shoot:         return "Shoot";
        case common::ActionRequest::GetBattleInfo: return "GetBattleInfo";
        case common::ActionRequest::DoNothing:     return "DoNothing";
        default:                                   return "DoNothing";
    }
}
}