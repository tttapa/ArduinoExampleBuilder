#include <Printing.hpp>
#include <Trim.hpp>
#include <fmt/format.h>
#include <fstream>
#include <unordered_map>

#include <ArduinoBuildJob.hpp>

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