#pragma once

#include <Exec.hpp>
#include <filesystem>

namespace fs = std::filesystem;

class ArduinoBuildJob {
  public:
    ArduinoBuildJob(fs::path sketch) : sketch(sketch) {}

    void run();

    const ExecResult &getResult() const { return result; }
    const fs::path &getSketch() const { return sketch; }

  private:
    fs::path sketch;
    ExecResult result;
};
