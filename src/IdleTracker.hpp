#pragma once

#include <chrono>

namespace gdss {
    class IdleTracker {
    public:
        static IdleTracker& get();

        void markInput();
        double secondsSinceInput() const;

    private:
        IdleTracker() = default;

        std::chrono::steady_clock::time_point m_lastInput = std::chrono::steady_clock::now();
    };
}
