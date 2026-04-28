#include <Geode/Geode.hpp>

#include <Geode/modify/CCDirector.hpp>
#include <Geode/modify/CCKeyboardDispatcher.hpp>
#include <Geode/modify/CCMouseDispatcher.hpp>
#include <Geode/modify/CCTouchDispatcher.hpp>
#include <Geode/modify/PlayLayer.hpp>

#if defined(GEODE_IS_WINDOWS)
#include <Geode/modify/CCEGLView.hpp>
struct GLFWwindow;
#endif

#include "ScreensaverController.hpp"

using namespace geode::prelude;

class $modify(GDSSDirectorHook, cocos2d::CCDirector) {
    void drawScene() {
        CCDirector::drawScene();
        gdss::ScreensaverController::get().onFrame();
    }
};

class $modify(GDSSKeyboardHook, cocos2d::CCKeyboardDispatcher) {
    bool dispatchKeyboardMSG(cocos2d::enumKeyCodes key, bool down, bool repeat, double time) {
        if (down || repeat) {
            gdss::ScreensaverController::get().reportInput();
        }
        return CCKeyboardDispatcher::dispatchKeyboardMSG(key, down, repeat, time);
    }
};

class $modify(GDSSTouchHook, cocos2d::CCTouchDispatcher) {
    void touchesBegan(cocos2d::CCSet* touches, cocos2d::CCEvent* event) {
        gdss::ScreensaverController::get().reportInput();
        CCTouchDispatcher::touchesBegan(touches, event);
    }

    void touchesMoved(cocos2d::CCSet* touches, cocos2d::CCEvent* event) {
        gdss::ScreensaverController::get().reportInput();
        CCTouchDispatcher::touchesMoved(touches, event);
    }

    void touchesEnded(cocos2d::CCSet* touches, cocos2d::CCEvent* event) {
        gdss::ScreensaverController::get().reportInput();
        CCTouchDispatcher::touchesEnded(touches, event);
    }

    void touchesCancelled(cocos2d::CCSet* touches, cocos2d::CCEvent* event) {
        gdss::ScreensaverController::get().reportInput();
        CCTouchDispatcher::touchesCancelled(touches, event);
    }
};

class $modify(GDSSMouseScrollHook, cocos2d::CCMouseDispatcher) {
    bool dispatchScrollMSG(float x, float y) {
        gdss::ScreensaverController::get().reportInput();
        return CCMouseDispatcher::dispatchScrollMSG(x, y);
    }
};

#if defined(GEODE_IS_WINDOWS)
class $modify(GDSSGLViewHook, cocos2d::CCEGLView) {
    void onGLFWMouseCallBack(GLFWwindow* window, int button, int action, int mods) {
        gdss::ScreensaverController::get().reportInput();
        CCEGLView::onGLFWMouseCallBack(window, button, action, mods);
    }

    void onGLFWMouseMoveCallBack(GLFWwindow* window, double x, double y) {
        gdss::ScreensaverController::get().reportInput();
        CCEGLView::onGLFWMouseMoveCallBack(window, x, y);
    }
};
#endif

class $modify(GDSSPlayLayerHook, PlayLayer) {
    void destroyPlayer(PlayerObject* player, GameObject* object) {
        if (gdss::ScreensaverController::get().isActive()) {
            return;
        }
        PlayLayer::destroyPlayer(player, object);
    }
};
