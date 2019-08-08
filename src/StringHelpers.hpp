#pragma once

#include <algorithm>
#include <cctype>
#include <locale>

// trim from start (in place)
static inline std::string &ltrim(std::string &s) {
    s.erase(s.begin(), std::find_if(s.begin(), s.end(),
                                    [](int ch) { return !std::isspace(ch); }));
    return s;
}

// trim from end (in place)
static inline std::string &rtrim(std::string &s) {
    s.erase(std::find_if(s.rbegin(), s.rend(),
                         [](int ch) { return !std::isspace(ch); })
                .base(),
            s.end());
    return s;
}

// trim from both ends (in place)
static inline std::string &trim(std::string &s) {
    ltrim(s);
    rtrim(s);
    return s;
}

// trim from start (copying)
static inline std::string ltrim_copy(std::string s) {
    ltrim(s);
    return s;
}

// trim from end (copying)
static inline std::string rtrim_copy(std::string s) {
    rtrim(s);
    return s;
}

// trim from both ends (copying)
static inline std::string trim_copy(std::string s) {
    trim(s);
    return s;
}

static inline std::string &tolower(std::string &s) {
    std::transform(s.begin(), s.end(), s.begin(),
                   [](unsigned char c) { return std::tolower(c); });
    return s;
}

static inline std::string tolower_copy(std::string s) {
    std::transform(s.begin(), s.end(), s.begin(),
                   [](unsigned char c) { return std::tolower(c); });
    return s;
}

std::string md5(const std::string &s);