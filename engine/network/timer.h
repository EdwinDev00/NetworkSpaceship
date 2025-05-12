#pragma once

#include <chrono>


struct Time {
    static uint64_t Now() {
        const auto now = std::chrono::system_clock::now();
        const auto duration = now.time_since_epoch();
        return std::chrono::duration_cast<std::chrono::milliseconds>(duration).count();
    }
};