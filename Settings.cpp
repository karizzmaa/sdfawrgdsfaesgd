#include "Settings.hpp"

#include <algorithm>

using namespace geode::prelude;

namespace {
    template <class T>
    T readSetting(char const* key, T fallback) {
        auto mod = Mod::get();
        if (!mod) {
            return fallback;
        }
        return mod->getSettingValue<T>(key);
    }
}

namespace gdss::settings {
    bool enabled() {
        return readSetting<bool>("enabled", true);
    }

    int idleTimeoutMinutes() {
        return std::clamp(readSetting<int>("idle-timeout-minutes", 3), 1, 10);
    }

    float transitionSpeed() {
        return std::clamp(readSetting<float>("transition-speed", 0.6f), 0.1f, 2.0f);
    }

    bool featuredEpicOnly() {
        return readSetting<bool>("featured-epic-only", false);
    }

    float cameraSpeed() {
        return std::clamp(readSetting<float>("camera-speed", 1.0f), 0.5f, 2.5f);
    }
}
