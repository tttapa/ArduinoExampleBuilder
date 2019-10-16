#include <algorithm>
#include <filesystem>
#include <iomanip>
#include <regex>
#include <set>
#include <stdexcept>
#include <string>

#include <ArduinoBuildJob.hpp>
#include <JobServer.hpp>
#include <Printing.hpp>

namespace fs = std::filesystem;

class ArgMatcher {
  public:
    ArgMatcher(std::string flag, std::string abbr, int nb_arguments,
               std::string docstr)
        : flag(flag), abbr(abbr), nb_arguments(nb_arguments), docstr(docstr) {
        matchers.insert(matchers.end(), this);
    }

    ~ArgMatcher() { matchers.erase(this); }

    int match(int argc, const char *argv[]) {
        std::smatch m;
        std::regex flagpattern1{"^--" + flag + "$"};
        std::regex abbrpattern1{"^-" + abbr + "$"};
        std::string s = argv[0];
        if ((!flag.empty() && std::regex_match(s, m, flagpattern1)) ||
            (!abbr.empty() && std::regex_match(s, m, abbrpattern1))) {
            if (argc < nb_arguments + 1) {
                throw std::invalid_argument("Error: " + flag + " expects " +
                                            std::to_string(nb_arguments) +
                                            " parameter(s)");
            }
            matched = true;
            arguments.resize(arguments.size() + nb_arguments);
            std::copy_n(argv + 1, nb_arguments, arguments.end() - nb_arguments);
            return nb_arguments + 1;
        }
        std::regex flagpattern2{"^--" + flag + "=(.*)$"};
        std::regex abbrpattern2{"^-" + abbr + "(.*)$"};
        if (nb_arguments == 1 &&
            ((!flag.empty() && std::regex_match(s, m, flagpattern2)) ||
             (!abbr.empty() && std::regex_match(s, m, abbrpattern2)))) {
            matched = true;
            arguments.push_back(m.size() == 1 ? std::string("") : m[1]);
            return 1;
        }
        return 0;
    }

    template <class T = std::string>
    T getValueOrDefault(const T &defaultval = "") {
        return arguments.size() == 0 ? defaultval : T(arguments[0]);
    }

    std::string getDocStr() const {
        std::string docstr = "";
        if (!flag.empty())
            docstr += "  --" + flag;
        if (!flag.empty() && !abbr.empty())
            docstr += ", ";
        else
            docstr += "  ";
        if (!abbr.empty())
            docstr += "-" + abbr;
        docstr += "\n    " + this->docstr + "\n";
        return docstr;
    }

    std::string flag;
    std::string abbr;
    int nb_arguments;
    std::string docstr;
    std::vector<std::string> arguments;
    bool matched = false;

    static std::set<ArgMatcher *> matchers;

    static int matchAll(int argc, const char *argv[]) {
        int argc_initial = argc;
        while (argc) {
            int argc_before = argc;
            for (ArgMatcher *matcher : matchers) {
                int matched = matcher->match(argc, argv);
                argc -= matched;
                argv += matched;
                if (argc <= 0)
                    break;
            }
            if (argc == argc_before)
                break;
        }
        return argc_initial - argc;
    }

    static std::string getHelp() {
        std::string help = "";
        for (ArgMatcher *matcher : matchers)
            help += matcher->getDocStr() + "\n";
        return help;
    }
};

std::set<ArgMatcher *> ArgMatcher::matchers;

void printJobs(const std::vector<ArduinoBuildJob> &jobs) {
    for (auto &job : jobs) {
        WhiteB(std::cout) << "\n"
                          << "Sketch: " << job.getSketch() << "\n"
                          << "Board: " << job.getBoard() << "\n"
                          << "Status: " << job.getResult().status << "\n"
                          << "Output: \n\n";
        std::cout << job.getResult().output << std::endl;
    }
}

void printSkippedJobs(const std::vector<ArduinoBuildJob> &jobs) {
    for (auto &job : jobs) {
        std::cout << "\n"
                  << "Sketch: " << job.getSketch() << "\n"
                  << "Board: " << job.getBoard() << std::endl;
    }
}

int main_application(int argc, const char *argv[]) {
    auto starttime = std::chrono::steady_clock::now();

    // Command line arguments
    ArgMatcher help = {"help", "h", 0, "Print this help page."};
    ArgMatcher verbose = {"verbose", "v", 0,
                          "Print the commands being executed."};
    ArgMatcher printsuccessful = {
        "print-successful", "s", 0,
        "Print the output of successful compilations as well."};
    ArgMatcher directory = {"directory", "d", 1,
                            "The directory with examples to compile."};
    ArgMatcher cachedirectory = {
        "cache-directory", "c", 1,
        "The directory for caching the core libraries."};
    ArgMatcher parallel = {"parallel", "p", 1,
                           "The number of examples to compile in parallel."};
    ArgMatcher jobs = {
        "jobs", "j", 1,
        "The number of files to compile in parallel (for each example)."};
    ArgMatcher boardoptions = {"board-options-file", "b", 1,
                               "The file with board options."};
    ArgMatcher defaultBoard = {"default-board", "", 1,
                               "The board to compile for when the sketch "
                               "doesn't contain an @boards tag."};
    ArgMatcher defaultBoardOptions = {
        "default-board-options", "", 1,
        "The default board options to use when no board "
        "options file is specified"};
    ArgMatcher args = {
        "args", "a", 0,
        "The arguments to pass to arduino-builder.\n    This should be the "
        "last argument, all arguments after it will be passed\n    directly to "
        "arduino-builder.\n    If specified, no default settings will be "
        "passed to arduino-builder,\n    just the sketch to compile and the "
        "fully qualified board name."};

    // Parse command line arguments
    Options options;
    --argc;
    ++argv;
    int used = ArgMatcher::matchAll(argc, argv);
    argc -= used;
    argv += used;

    // Save other arguments to pass to arduino-builder
    if (argc != 0 && !args.matched) {
        std::cerr << "Error: unknown option `" << *argv << "`" << std::endl;
        exit(1);
    } else {
        if (args.matched) {
            options.options = "";
            options.noDefaults = true;
            while (argc-- > 0) {
                options.options += std::string(*(argv++)) + " ";
            }
            std::cout << "other options = " << options.options << std::endl;
        }
    }

    // Print help
    if (help.matched) {
        std::cout << ArgMatcher::getHelp();
        exit(0);
    }

    // Save options
    try {
        options.jobs = jobs.matched ? std::stoi(jobs.arguments[0]) : 1;
    } catch (std::invalid_argument &e) {
        std::cerr << "Error: --jobs expects an integer value" << std::endl;
        exit(1);
    }
    try {
        options.parallel = parallel.matched
                               ? std::stoi(parallel.arguments[0])
                               : std::thread::hardware_concurrency();
    } catch (std::invalid_argument &e) {
        std::cerr << "Error: --jobs expects an integer value" << std::endl;
        exit(1);
    }
    options.directory = directory.getValueOrDefault(fs::current_path());
    options.boardOptions = boardoptions.getValueOrDefault(
        fs::canonical("/proc/self/exe").parent_path() /
        "../share/arduino-example-builder/board-options.txt");
    options.cacheDirectory = cachedirectory.getValueOrDefault(
        fs::path("/tmp/arduino-example-builder"));
    options.defaultBoard = defaultBoard.getValueOrDefault<std::string>("uno");
    options.defaultBoardOptions =
        defaultBoardOptions.getValueOrDefault<std::string>(
            "uno=arduino:avr:uno");
    options.verbose = verbose.matched;

    // Configure the build process
    ArduinoBuildJob::configure(options);

    // Start a job server for building
    JobServer<ArduinoBuildJob> js = options.parallel;

    // Schedule all .ino examples in this directory
    for (auto &p : fs::recursive_directory_iterator(
             options.directory,
             fs::directory_options::skip_permission_denied)) {
        if (p.path().extension() == ".ino") {
            auto boards = ArduinoBuildJob::getBoards(p.path());
            if (boards.empty())
                js.schedule(p.path(), options.defaultBoard);
            for (auto &board : boards)
                js.schedule(p.path(), board);
        }
    }

    size_t numberSuccessfulJobs = 0;
    std::vector<ArduinoBuildJob> failedJobs;
    std::vector<ArduinoBuildJob> successfulJobs;
    std::vector<ArduinoBuildJob> skippedJobs;
    while (!js.isFinished()) {
        if (auto finishedJob = js.run()) {
            auto result = finishedJob->getResult();
            bool skipped = finishedJob->getSkipped();
            if (skipped) {
                skippedJobs.emplace_back(std::move(*finishedJob));
            } else if (result.status == 0) {
                ++numberSuccessfulJobs;
                if (printsuccessful.matched)
                    successfulJobs.emplace_back(std::move(*finishedJob));
            } else {
                failedJobs.emplace_back(std::move(*finishedJob));
            }
        }
    }
    size_t totalJobs = failedJobs.size() + numberSuccessfulJobs;

    if (totalJobs == 0) {
        Red(std::cerr) << "Error: no examples were found" << std::endl;
        exit(42);
    }

    if (printsuccessful.matched) {
        printJobs(successfulJobs);
    }

    if (!skippedJobs.empty()) {
        YellowB(std::cout) << "\n"
                           << "The following examples were skipped: \n";
        printSkippedJobs(skippedJobs);
    }

    if (failedJobs.size() == 0) {
        GreenB(std::cout)
            << "\n"
            << " ╔═══════════════════════════════════════════════╗\n"
            << " ║    All " << std::setfill(' ') << std::setw(3)
            << numberSuccessfulJobs << " examples built successfully!  ✔    ║\n"
            << " ╚═══════════════════════════════════════════════╝\n"
            << std::endl;
    } else {
        printJobs(failedJobs);
        RedB(std::cout)
            << "\n"
            << " ╔═══════════════════════════════════════════════╗\n"
            << " ║     " << std::setfill(' ') << std::setw(3)
            << failedJobs.size() << " of " << std::setw(3) << totalJobs
            << " examples failed to build.      ║\n"
            << " ╚═══════════════════════════════════════════════╝\n"
            << std::endl;
    }

    auto endtime = std::chrono::steady_clock::now();
    std::chrono::duration<double> diff = endtime - starttime;
    std::cout << "Total time: " << std::setprecision(3) << diff.count()
              << " s\n";
    std::cout << "Average time per example: " << std::setprecision(3)
              << (diff.count() / totalJobs) << " s\n";

    return failedJobs.size();
}

int main(int argc, const char *argv[]) {
    try {
        return main_application(argc, argv);
    } catch (std::exception &e) {
        std::cerr << e.what() << std::endl;
        exit(2);
    }
}