#pragma once

#include <cassert>
#include <chrono>
#include <cstdio>
#include <future>
#include <iostream>
#include <memory>
#include <optional>
#include <vector>

template <class Job>
class JobServer {
  public:
    JobServer(size_t threads = 1) : queue(threads) { jobs.reserve(10); }

#if 0
    void schedule(Job &&job) {
        std::cout << __PRETTY_FUNCTION__ << std::endl;
        this->jobs.push_back(std::forward<Job>(job));
    }
    void schedule(const Job &job) {
        std::cout << __PRETTY_FUNCTION__ << std::endl;
        this->jobs.emplace_back(job);
    }
#else
    template <class... Args>
    void schedule(Args &&... args) {
        std::cout << __PRETTY_FUNCTION__ << std::endl;
        assert(!started);
        this->jobs.emplace_back(std::forward<Args>(args)...);
    }
#endif
    void scheduleJobs(std::vector<Job> &&jobs) {
        std::cout << __PRETTY_FUNCTION__ << std::endl;
        assert(!started);
        this->jobs.insert(this->jobs.end(),                      //
                          std::make_move_iterator(jobs.begin()), //
                          std::make_move_iterator(jobs.end()));
    }
    void scheduleJobs(const std::vector<Job> &jobs) {
        std::cout << __PRETTY_FUNCTION__ << std::endl;
        assert(!started);
        this->jobs.insert(this->jobs.end(), //
                          jobs.begin(),     //
                          jobs.end());
    }

    void start() { started = true; }

    std::optional<Job> run() {
        using namespace std::chrono_literals;

        if (finished)
            return std::nullopt;

        if (!started)
            start();

        Job *finishedJob = nullptr;
        if (nextToLaunch < jobs.size()) {
            for (auto &fut : queue) {
                if (fut.valid() == false || // uninitialized
                    fut.wait_for(10ms) == std::future_status::ready) {
                    finishedJob = fut.valid() ? fut.get() : nullptr;
                    Job *job = &jobs[nextToLaunch++];
                    size_t nextToLaunchCopy = nextToLaunch;
                    size_t numJobs = jobs.size();
                    auto launch = [job, nextToLaunchCopy, numJobs]() -> Job * {
                        LockedYellow(std::cout, cout_mutex)
                            << "lambda: starting job " << nextToLaunchCopy
                            << " of " << numJobs << std::endl;
                        job->run();
                        LockedYellow(std::cout, cout_mutex)
                            << "lambda: job done" << std::endl;
                        return job;
                    };
                    fut = std::async(std::launch::async, launch);
                    break; // for
                }
            }
        } else {
            finished = true;
            for (auto &fut : queue) {
                if (fut.valid() == false) {
                    // finished, or not launched
                } else if (fut.wait_for(10ms) == std::future_status::ready) {
                    finishedJob = fut.get();
                    finished = false;
                    LockedRed(std::cout, cout_mutex)
                        << (fut.valid() ? "true" : "false") << std::endl;
                    break; // for
                } else {
                    // still running
                    finished = false;
                }
            }
        }
        if (finished) {
            LockedYellowB(std::cout, cout_mutex) << "FINISHED" << std::endl;
        }
        if (finishedJob) {
            LockedYellow(std::cout, cout_mutex)
                << "JobServer::run: returning job " << finishedJob->getSketch()
                << std::endl
                << "finishedJob - &jobs[0] = " << (finishedJob - &jobs[0])
                << std::endl;
            Job finishedJobRet = std::move(*finishedJob);
            return finishedJobRet;
        }
        return std::nullopt;
    }

    bool isFinished() const { return finished; }
    bool isStarted() const { return started; }

  private:
    std::vector<std::future<Job *>> queue;
    std::vector<Job> jobs;
    size_t nextToLaunch = 0;
    bool started = false;
    bool finished = false;
};
