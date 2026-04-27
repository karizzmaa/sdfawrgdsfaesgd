#pragma once

namespace gdss {
    class ScreensaverController {
    public:
        static ScreensaverController& get();

        void onFrame();
        void reportInput();
        void deactivate();
        bool isActive() const;

    private:
        ScreensaverController() = default;

        bool canActivateFromCurrentScene() const;
        void activate();

        bool m_active = false;
        bool m_transitioningOut = false;
    };
}
