#pragma once

#include <chrono>
#include <cstdio>
#include <future>
#include <iostream>
#include <memory>
#include <optional>
#include <vector>

#include <Printing.hpp>

template <class Job>
class JobServer {
  public:
    JobServer(unsigned int threads = 1) : futures(threads) { jobs.reserve(10); }

    template <class... Args>
    void schedule(Args &&... args) {
        if (started)
            throw std::logic_error("Error: cannot schedule new job, job server "
                                   "is already started.");
        // Adding more jobs could cause reallocation, and would invalidate
        // iterators passed to other threads
        this->jobs.emplace_back(std::forward<Args>(args)...);
    }

    void start() { started = true; }

    std::optional<Job> run() {
        using namespace std::chrono_literals;

        if (finished)
            return std::nullopt;

        if (!started)
            start();

        Job *finishedJob = nullptr;
        if (nextToLaunch < jobs.size()) { // There are still scheduled jobs
                                          // waiting to be started
            for (auto &fut : futures) {
                if (fut.valid() == false || // uninitialized
                    fut.wait_for(10ms) == std::future_status::ready) {
                    // Get the result of the finished future
                    finishedJob = fut.valid() ? fut.get() : nullptr;

                    // Prepare the new task
                    Job *job = &jobs[nextToLaunch++];
                    auto launch = [job]() -> Job * {
                        job->run();
                        return job;
                    };

                    size_t progress = nextToLaunch;
                    size_t total = jobs.size();
                    LockedBlue(std::cout, cout_mutex)
                        << "Starting job " << progress << " of " << total
                        << std::endl;

                    // And re-use the finished future to launch it
                    fut = std::async(std::launch::async, launch);
                    break; // for
                }
            }
        } else { // All scheduled jobs have been started
                 // (or have already finished)
            finished = true;
            for (auto &fut : futures) {
                if (fut.valid() == false) {
                    // finished, or not launched
                } else if (fut.wait_for(10ms) == std::future_status::ready) {
                    finishedJob = fut.get();
                    finished = false;
                    break; // for
                } else {
                    // still running
                    finished = false;
                }
            }
        }

        if (finishedJob)
            return std::move(*finishedJob);
        return std::nullopt;
    }

    bool isFinished() const { return finished; }
    bool isStarted() const { return started; }

  private:
    std::vector<std::future<Job *>> futures;
    std::vector<Job> jobs;
    size_t nextToLaunch = 0;
    bool started = false;
    bool finished = false;
};
