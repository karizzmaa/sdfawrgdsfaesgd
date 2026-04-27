#include "ScreensaverController.hpp"

#include "IdleTracker.hpp"
#include "ScreensaverLayer.hpp"
#include "Settings.hpp"

#include <Geode/Geode.hpp>

using namespace geode::prelude;

namespace gdss {
    ScreensaverController& ScreensaverController::get() {
        static ScreensaverController instance;
        return instance;
    }

    bool ScreensaverController::canActivateFromCurrentScene() const {
        if (!CCDirector::get()->getRunningScene()) {
            return false;
        }
        if (PlayLayer::get()) {
            return false;
        }
        if (LevelEditorLayer::get()) {
            return false;
        }
        if (EditorUI::get()) {
            return false;
        }
        return true;
    }

    void ScreensaverController::activate() {
        if (m_active) {
            return;
        }
        auto scene = ScreensaverLayer::scene();
        if (!scene) {
            return;
        }
        m_active = true;
        auto duration = settings::transitionSpeed();
        CCDirector::get()->pushScene(CCTransitionFade::create(duration, scene));
    }

    void ScreensaverController::deactivate() {
        if (!m_active || m_transitioningOut) {
            return;
        }
        m_transitioningOut = true;
        m_active = false;
        IdleTracker::get().markInput();
        auto duration = settings::transitionSpeed();
        CCDirector::get()->popSceneWithTransition(duration, PopTransition::kPopTransitionFade);
        m_transitioningOut = false;
    }

    void ScreensaverController::reportInput() {
        IdleTracker::get().markInput();
        if (m_active) {
            this->deactivate();
        }
    }

    void ScreensaverController::onFrame() {
        if (m_active || m_transitioningOut) {
            return;
        }
        if (!settings::enabled()) {
            return;
        }
        if (!this->canActivateFromCurrentScene()) {
            return;
        }
        auto idleNeeded = settings::idleTimeoutMinutes() * 60.0;
        if (IdleTracker::get().secondsSinceInput() < idleNeeded) {
            return;
        }
        this->activate();
    }

    bool ScreensaverController::isActive() const {
        return m_active;
    }
}
