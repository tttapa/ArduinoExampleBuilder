#include <Printing.hpp>
#include <StringHelpers.hpp>
#include <fmt/format.h>
#include <fstream>
#include <unordered_map>

#include <ArduinoBuildJob.hpp>

/// Constructor
ArduinoBuildJob::ArduinoBuildJob(fs::path sketch, std::string board)
    : sketch(sketch), board(board) {
    if (boardOptions.count(tolower_copy(board)) == 0) {
        throw std::runtime_error("Error: unknown board `" + board +
                                 "` in file \"" + sketch.string() + "\"");
    }
}

/// Build the sketch for the given board
void ArduinoBuildJob::run() {
    if (boardOptions[tolower_copy(board)] == "skip") {
        LockedYellow(std::cout, cout_mutex)
            << "Skipped " << sketch.filename() << " for board " << board << "."
            << std::endl;
        skipped = true;
        return;
    }

    using namespace fmt::literals;

    LockedBlueB(std::cout, cout_mutex)
        << "Building Example " << sketch.filename() << " for board " << board
        << std::endl;

    fs::path boardCachedir = cachedir / board;
    fs::create_directories(boardCachedir);

    std::string hash = md5(sketch.string() + board);
    fs::path tmpsketchfolder = cachedir / hash;
    fs::path tmpsketch = tmpsketchfolder / sketch.filename();
    fs::path tmpsketchhash = tmpsketchfolder / (hash + ".ino");
    fs::create_directories(tmpsketchfolder);
    fs::copy(sketch.parent_path(), tmpsketchfolder,
             fs::copy_options::update_existing | fs::copy_options::recursive);
    fs::rename(tmpsketch, tmpsketchhash);

    std::string cmd = //
        fmt::format("cd \"{folder}\" && "
                    "unbuffer "
                    "{command} "
                    "-fqbn {fqbn} "
                    "-build-cache \"{cachedir}\" "
                    "-compile \"{file}\" 2>&1",

                    "folder"_a = arduinoFolder.string(),          //
                    "command"_a = command,                        //
                    "fqbn"_a = boardOptions[tolower_copy(board)], //
                    "cachedir"_a = boardCachedir.string(),        //
                    "file"_a = tmpsketchhash.string());

    if (verbose) {
        LockedOStream(std::cout, cout_mutex) << cmd << std::endl;
    }

    result = exec(cmd);

    if (result.status == 0) {
        LockedGreenB(std::cout, cout_mutex)
            << "Built " << sketch.filename() << " successfully for board "
            << board << "! ✔" << std::endl;
    } else {
        LockedRedB(std::cout, cout_mutex)
            << "Builing " << sketch.filename() << " for " << board << " failed!"
            << std::endl;
    }
}

/// Get the Arduino IDE installation folder
static fs::path getArduinoFolder() {
    auto whichResult = exec("which arduino-builder");
    if (whichResult.status != 0)
        throw std::runtime_error(
            "Error: arduino-builder was not found in $PATH");
    fs::path arduinoBuilderPath = fs::canonical(rtrim(whichResult.output));
    return arduinoBuilderPath.parent_path();
}

struct ArduinoVersion {
    int major, minor, patch;
    operator std::string() const {
        return fmt::format("{:d}{:02d}{:02d}", major, minor, patch);
    }
    std::string str() const { return *this; }
};

/// Get the version of the Arduino IDE installation at the given folder
static ArduinoVersion getArduinoVersion(const fs::path &arduinoFolder) {
    std::ifstream versionfile = arduinoFolder / "lib" / "version.txt";
    if (!versionfile)
        throw std::runtime_error("Error: couldn't find `lib/version.txt` in "
                                 "Arduino IDE installation folder");
    ArduinoVersion version;
    versionfile >> version.major;
    versionfile.ignore();
    versionfile >> version.minor;
    versionfile.ignore();
    versionfile >> version.patch;
    return version;
}

#include <fstream>
#include <regex>

/// Return true if string starts with whitespace followed by a # character
static bool isComment(const std::string &line) {
    return std::regex_match(line, std::regex("\\s*#.*"));
}

/// Load the board name to fully qualified board name mappings from a file
void ArduinoBuildJob::loadBoardOptions(const Options &options) {
    std::ifstream file = options.boardOptions;
    if (!file.good())
        Yellow(std::cerr) << "Warning: board options file doesn't exist ("
                          << options.boardOptions << ")" << std::endl;
    std::string line = options.defaultBoardOptions;
    do {
        if (isComment(line))
            continue;
        std::smatch match;
        std::regex re("^([^=]+)=(.+)$");
        if (std::regex_match(line, match, re) && match.size() == 3) {
            boardOptions[tolower_copy(match[1])] = match[2];
        }
    } while (std::getline(file, line));
}

/// Get the supported board names from a given sketch
/// Expects a "@boards board name, another board name" comment to be present
std::vector<std::string> ArduinoBuildJob::getBoards(const fs::path &sketch) {
    std::string line;
    std::ifstream file = sketch;
    while (std::getline(file, line)) {
        std::smatch match;
        std::regex re("^[\\s/*]*@boards\\s+(.*)$");
        if (std::regex_match(line, match, re) && match.size() == 2) {
            std::vector<std::string> boards;
            std::istringstream ss(match[1]);
            std::string substr;
            while (getline(ss, substr, ',')) {
                boards.push_back(trim(substr));
            }
            return boards;
        }
    }
    return {};
}

void ArduinoBuildJob::configure(const Options &options) {
    loadBoardOptions(options);
    verbose = options.verbose;
    fs::path home = getenv("HOME");
    arduinoFolder = getArduinoFolder();
    fs::path defaultLibraries = home / "Arduino" / "libraries";
    std::string defaultVersion = getArduinoVersion(arduinoFolder).str();

    std::string defaults = "-hardware {arduino}/hardware "
                           "-hardware {home}/.arduino15/packages "
                           "-tools {arduino}/tools-builder "
                           "-tools {home}/.arduino15/packages "
                           "-built-in-libraries {arduino}/libraries "
                           "-libraries {libraries} "
                           "-core-api-version {version} "
                           "-warnings all "
                           "-jobs {jobs} ";
    using namespace fmt::literals;
    defaults = fmt::format(defaults,
                           "arduino"_a = arduinoFolder.string(),      //
                           "home"_a = home.string(),                  //
                           "libraries"_a = defaultLibraries.string(), //
                           "version"_a = defaultVersion,
                           "jobs"_a = options.jobs); //

    command = "arduino-builder ";
    if (!options.noDefaults)
        command += defaults;

    cachedir = options.cacheDirectory;
    fs::create_directories(cachedir);
}

std::string ArduinoBuildJob::command;
fs::path ArduinoBuildJob::cachedir;
fs::path ArduinoBuildJob::arduinoFolder;
std::unordered_map<std::string, std::string> ArduinoBuildJob::boardOptions;
bool ArduinoBuildJob::verbose = false;