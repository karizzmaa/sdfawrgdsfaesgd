#include "IdleTracker.hpp"

namespace gdss {
    IdleTracker& IdleTracker::get() {
        static IdleTracker instance;
        return instance;
    }

    void IdleTracker::markInput() {
        m_lastInput = std::chrono::steady_clock::now();
    }

    double IdleTracker::secondsSinceInput() const {
        auto now = std::chrono::steady_clock::now();
        auto delta = std::chrono::duration_cast<std::chrono::duration<double>>(now - m_lastInput);
        return delta.count();
    }
}
