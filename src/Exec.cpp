#include <array>
#include <fstream>
#include <stdexcept>

#include <Exec.hpp>

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