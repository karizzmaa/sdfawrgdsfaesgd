#pragma once

#include <Geode/Geode.hpp>

#include "RandomLevelService.hpp"

class ScreensaverLayer : public cocos2d::CCLayer {
public:
    static cocos2d::CCScene* scene();
    static ScreensaverLayer* create();

    bool init() override;
    void onExit() override;
    void update(float dt) override;

    bool ccTouchBegan(cocos2d::CCTouch* touch, cocos2d::CCEvent* event) override;
    void keyDown(cocos2d::enumKeyCodes key, double dt) override;

private:
    void beginTransition();
    void swapToNextLevel();
    void endTransition();
    void applyPresentationMode(PlayLayer* playLayer);
    void updateCinematicCamera(float dt);
    void restoreAudio();

    geode::Ref<RandomLevelService> m_levelService;
    geode::Ref<PlayLayer> m_playLayer;
    geode::Ref<cocos2d::CCLayerColor> m_fade;

    float m_elapsed = 0.f;
    float m_cameraX = 0.f;
    float m_cameraY = 0.f;
    bool m_transitioning = false;

    float m_savedMusic = 1.f;
    float m_savedSfx = 1.f;
    bool m_audioCaptured = false;
};
