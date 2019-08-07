#include <array>
#include <filesystem>
#include <fmt/format.h>
#include <fstream>
#include <iomanip>
#include <stdexcept>
#include <string>
#include <unordered_map>

#include <ANSIColors.hpp>
#include <JobServer.hpp>

struct ExecResult {
    int status;
    std::string output;
};

ExecResult exec(const char *cmd) {
    std::array<char, 128> buffer;
    FILE *pipe = popen(cmd, "r");
    if (!pipe) {
        throw std::runtime_error("popen() failed!");
    }
    std::string output;
    while (fgets(buffer.data(), buffer.size(), pipe) != nullptr) {
        output += buffer.data();
    }
    int status = pclose(pipe);
    status = WEXITSTATUS(status);
    return {status, output};
}
ExecResult exec(const std::string &cmd) { return exec(cmd.c_str()); }

namespace fs = std::filesystem;

static std::unordered_map<std::string, std::string> fqbns = {
    {
        "esp32",
        "esp32:esp32:esp32thing:"
        "FlashFreq=80,PartitionScheme=default,UploadSpeed=921600,DebugLevel="
        "none",
    },
    {
        "teensy32",
        "teensy:avr:teensy31:"
        "speed=96,usb=serialmidiaudio,opt=o2std,keys=en-us",
    },
};

std::string &rtrim(std::string &str) {
    auto it1 = std::find_if(str.rbegin(), str.rend(), [](char ch) {
        return !std::isspace<char>(ch, std::locale::classic());
    });
    str.erase(it1.base(), str.end());
    return str;
}

class LockedOStream {
  private:
    std::ostream &os;
    std::lock_guard<std::mutex> lock;

  public:
    LockedOStream(std::ostream &os, std::mutex &mutex) : os(os), lock{mutex} {}
    template <class T>
    std::ostream &operator<<(T &&t) const {
        return os << t;
    }
};

std::mutex cout_mutex;

class ArduinoBuildJob {
  public:
    ArduinoBuildJob(fs::path sketch) : sketch(sketch) {}
    void run();

    ExecResult getResult() const { return result; }

    const fs::path &getSketch() const { return sketch; }

  private:
    fs::path sketch;
    ExecResult result;
};

int main() {
    JobServer<ArduinoBuildJob> js = 8;

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
        RedB(std::cout)
            << "\n"
            << " ╔═══════════════════════════════════════════════╗\n"
            << " ║     " << std::setfill(' ') << std::setw(3)
            << failedJobs.size() << " of " << std::setw(3)
            << (failedJobs.size() + successfulJobs)
            << " examples failed to build.      ║\n"
            << " ╚═══════════════════════════════════════════════╝\n"
            << std::endl;
        for (auto &failedJob : failedJobs) {
            WhiteB(std::cout)
                << "Sketch: " << failedJob.getSketch() << "\n"
                << "Status: " << failedJob.getResult().status << "\n"
                << "Output: \n";
            std::cout << failedJob.getResult().output << std::endl;
        }
    }

    return failedJobs.size();
}

void ArduinoBuildJob::run() {
    using namespace fmt::literals;

    LockedBlueB(std::cout, cout_mutex)
        << "Building Example: " << sketch.filename() << std::endl;

    auto whichResult = exec("which arduino-builder");
    if (whichResult.status != 0)
        throw std::runtime_error(
            "Error: arduino-builder was not found in $PATH");

    fs::path home = getenv("HOME");
    fs::path arduinoBuilderPath = fs::canonical(rtrim(whichResult.output));
    fs::path arduino = arduinoBuilderPath.parent_path();
    fs::path libraries = home / "Arduino" / "libraries";

    int version_maj, version_min, version_pat;
    std::ifstream versionf = arduino / "lib" / "version.txt";
    versionf >> version_maj;
    versionf.ignore();
    versionf >> version_min;
    versionf.ignore();
    versionf >> version_pat;
    std::string version =
        fmt::format("{:d}{:02d}{:02d}", version_maj, version_min, version_pat);
    fs::path cachedir = "/tmp/arduino-example-builder/cache";
    fs::create_directories(cachedir);

    std::string cmd = //
        fmt::format("cd \"{folder}\" && "
                    "unbuffer "
                    "arduino-builder "
                    "-hardware {arduino}/hardware "
                    "-hardware {home}/.arduino15/packages "
                    "-tools {arduino}/tools-builder "
                    "-tools {home}/.arduino15/packages "
                    "-libraries {libraries} "
                    "-fqbn {fqbn} "
                    "-core-api-version {version} "
                    "-warnings all "
                    "-jobs 1 "
                    // "-build-cache \"{cachedir}\" "
                    "-compile \"{file}\" 2>&1",

                    "folder"_a = sketch.parent_path().string(), //
                    "arduino"_a = arduino.string(),             //
                    "home"_a = home.string(),                   //
                    "libraries"_a = libraries.string(),         //
                    "fqbn"_a = fqbns["teensy32"],               //
                    "version"_a = version,                      //
                    // "cachedir"_a = cachedir.string(),           //
                    "file"_a = sketch.filename().string());

    bool printCommands = false;
    if (printCommands) {
        LockedOStream(std::cout, cout_mutex) << cmd << std::endl;
    }

    result = exec(cmd);

    if (result.status == 0) {
        LockedGreenB(std::cout, cout_mutex)
            << "Built " << sketch.filename() << " successfully! ✔" << std::endl;
    } else {
        LockedRedB(std::cout, cout_mutex)
            << "Builing " << sketch.filename() << " failed!" << std::endl;
    }
}