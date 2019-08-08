#pragma once

#include <Exec.hpp>
#include <filesystem>
#include <unordered_map>

namespace fs = std::filesystem;

struct Options {
    bool noDefaults = false;
    std::string options = "";
    fs::path directory;
    fs::path boardOptions;
    fs::path cacheDirectory;
    std::string defaultBoard;
    std::string defaultBoardOptions;
    unsigned int jobs = 0;
    unsigned int parallel = 0;
    bool verbose = false;
};

class ArduinoBuildJob {
  public:
    ArduinoBuildJob(fs::path sketch, std::string board);

    void run();

    const ExecResult &getResult() const { return result; }
    const fs::path &getSketch() const { return sketch; }

    static std::vector<std::string> getBoards(const fs::path &sketch);

    static void configure(const Options &options);
    static void loadBoardOptions(const Options &options);

    static std::string command;
    static fs::path arduinoFolder;
    static fs::path cachedir;
    static std::unordered_map<std::string, std::string> boardOptions;
    static bool verbose;

  private:
    fs::path sketch;
    std::string board;
    ExecResult result;
};
