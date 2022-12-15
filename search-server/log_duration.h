#pragma once

#ifndef CPP_SEARCH_SERVER_LOG_DURATION_H
#define CPP_SEARCH_SERVER_LOG_DURATION_H

#include <algorithm>
#include <chrono>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <thread>
#include <vector>

using namespace std::literals;

#define LOG_DURATION(x) LogDuration UNIQUE_VAR_NAME_PROFILE(x)
#define LOG_DURATION_STREAM(x, stream) LogDuration UNIQUE_VAR_NAME_PROFILE(x, stream)

#define PROFILE_CONCAT_INTERNAL(X, Y) X##Y
#define PROFILE_CONCAT(X, Y) PROFILE_CONCAT_INTERNAL(X, Y)
#define UNIQUE_VAR_NAME_PROFILE PROFILE_CONCAT(profile_guard_, __LINE__)

class LogDuration {
public:
    using Clock = std::chrono::steady_clock;

    LogDuration(const std::string operation_name, std::ostream& os = std::cerr) : operation_name_(operation_name), os_{os} {}

    ~LogDuration() {
        using namespace std::chrono;
        using namespace std::literals;

        const auto end_time = Clock::now();
        const auto dur = end_time - start_time_;
        os_ << "Operation time: "s << std::chrono::duration_cast<std::chrono::microseconds>(dur).count() << " mcs"s << std::endl;
    }

private:
    const Clock::time_point start_time_ = Clock::now();
    const std::string operation_name_ = ""s;
    std::ostream& os_;
};

#endif //CPP_SEARCH_SERVER_LOG_DURATION_H
