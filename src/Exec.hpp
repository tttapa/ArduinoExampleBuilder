#pragma once

#include <string>

struct ExecResult {
    int status;
    std::string output;
};

ExecResult exec(const char *cmd);
inline ExecResult exec(const std::string &cmd) { return exec(cmd.c_str()); }