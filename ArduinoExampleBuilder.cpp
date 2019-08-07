#include <array>
#include <cassert>
#include <chrono>
#include <cstdio>
#include <future>
#include <iostream>
#include <memory>
#include <optional>
#include <stdexcept>
#include <string>
#include <vector>

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

struct MyJob {
  MyJob() : i(++instances) { std::cout << __PRETTY_FUNCTION__ << std::endl; }
  MyJob(const MyJob &j) : i(j.i) {
    std::cout << __PRETTY_FUNCTION__ << std::endl;
  }
  MyJob(MyJob &&o) {
    std::swap(this->i, o.i);
    std::cout << __PRETTY_FUNCTION__ << std::endl;
  }
  MyJob &operator=(const MyJob &o) {
    this->i = o.i;
    std::cout << __PRETTY_FUNCTION__ << std::endl;
    return *this;
  }
  MyJob &operator=(MyJob &&o) {
    std::swap(this->i, o.i);
    std::cout << __PRETTY_FUNCTION__ << std::endl;
    return *this;
  }

  void run() { std::cout << __PRETTY_FUNCTION__ << ": " << i << std::endl; }

  int i;
  static int instances;
};

int MyJob::instances = 0;

int main() {
  JobServer<MyJob> js;

  std::cout << "lvalue\n";
  MyJob lv;
  js.schedule(lv);
  std::cout << "temporary\n";
  js.schedule(MyJob());
  std::cout << "move\n";
  js.schedule(std::move(lv));

  std::cout << "\nlvalue vector\n";
  std::vector<MyJob> lvs(2);
  js.scheduleJobs(lvs);
  std::cout << "temporary vector\n";
  js.scheduleJobs(std::vector<MyJob>(2));
  std::cout << "move vector\n";
  js.scheduleJobs(std::move(lvs));

  while (!js.isFinished())
    js.run();

  auto res = exec("bash -c 'echo $0; exit 42'");
  std::cout << res.output << res.status << std::endl;
}