#include <algorithm>
#include <filesystem>
#include <iomanip>
#include <stdexcept>
#include <string>

#include <ArduinoBuildJob.hpp>
#include <JobServer.hpp>
#include <Printing.hpp>

namespace fs = std::filesystem;

int main(int argc, const char *argv[]) {

    unsigned int threads = std::thread::hardware_concurrency();
    if (argc == 2) {
        std::string arg1 = argv[1];
        if (std::all_of(arg1.begin(), arg1.end(), isdigit)) {
            threads = std::stoi(arg1);
        }
    }
    JobServer<ArduinoBuildJob> js = threads;

    for (auto &p : fs::recursive_directory_iterator(fs::current_path())) {
        if (p.path().extension() == ".ino")
            js.schedule(p.path());
    }

    size_t successfulJobs = 0;
    std::vector<ArduinoBuildJob> failedJobs;

    while (!js.isFinished()) {
        if (auto finishedJob = js.run()) {
            auto result = finishedJob->getResult();
            if (result.status == 0)
                ++successfulJobs;
            else
                failedJobs.emplace_back(std::move(*finishedJob));
        }
    }

    if (failedJobs.size() == 0) {
        GreenB(std::cout)
            << "\n"
            << " ╔═══════════════════════════════════════════════╗\n"
            << " ║    All " << std::setfill(' ') << std::setw(3)
            << successfulJobs << " examples built successfully!  ✔    ║\n"
            << " ╚═══════════════════════════════════════════════╝\n"
            << std::endl;
    } else {
        for (auto &failedJob : failedJobs) {
            WhiteB(std::cout)
                << "\n"
                << "Sketch: " << failedJob.getSketch() << "\n"
                << "Status: " << failedJob.getResult().status << "\n"
                << "Output: \n";
            std::cout << failedJob.getResult().output << std::endl;
        }
        RedB(std::cout)
            << "\n"
            << " ╔═══════════════════════════════════════════════╗\n"
            << " ║     " << std::setfill(' ') << std::setw(3)
            << failedJobs.size() << " of " << std::setw(3)
            << (failedJobs.size() + successfulJobs)
            << " examples failed to build.      ║\n"
            << " ╚═══════════════════════════════════════════════╝\n"
            << std::endl;
    }

    return failedJobs.size();
}
