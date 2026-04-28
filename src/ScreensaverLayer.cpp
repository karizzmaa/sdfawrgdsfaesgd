#include "ScreensaverLayer.hpp"

#include "ScreensaverController.hpp"
#include "Settings.hpp"

#include <Geode/Geode.hpp>

#include <algorithm>
#include <cmath>

using namespace geode::prelude;

cocos2d::CCScene* ScreensaverLayer::scene() {
    auto layer = ScreensaverLayer::create();
    if (!layer) {
        return nullptr;
    }
    auto scene = cocos2d::CCScene::create();
    scene->addChild(layer);
    return scene;
}

ScreensaverLayer* ScreensaverLayer::create() {
    auto ret = new ScreensaverLayer();
    if (ret && ret->init()) {
        ret->autorelease();
        return ret;
    }
    delete ret;
    return nullptr;
}

bool ScreensaverLayer::init() {
    if (!CCLayer::init()) {
        return false;
    }

    this->setTouchEnabled(true);
    this->setKeyboardEnabled(true);
    this->setKeypadEnabled(true);
    this->setMouseEnabled(true);

    auto winSize = CCDirector::get()->getWinSize();

    auto background = cocos2d::CCLayerColor::create({0, 0, 0, 255}, winSize.width, winSize.height);
    background->setPosition({0.f, 0.f});
    this->addChild(background, 0);

    m_levelService = RandomLevelService::create();
    this->addChild(m_levelService, 1);
    m_levelService->setFeaturedEpicOnly(gdss::settings::featuredEpicOnly());

    m_fade = cocos2d::CCLayerColor::create({0, 0, 0, 255}, winSize.width, winSize.height);
    m_fade->setPosition({0.f, 0.f});
    this->addChild(m_fade, 200);

    auto audio = FMODAudioEngine::get();
    if (audio) {
        m_savedMusic = audio->getBackgroundMusicVolume();
        m_savedSfx = audio->getEffectsVolume();
        audio->setBackgroundMusicVolume(0.f);
        audio->setEffectsVolume(0.f);
        m_audioCaptured = true;
    }

    this->scheduleUpdate();
    m_levelService->warmQueue(2);

    return true;
}

void ScreensaverLayer::onExit() {
    this->unscheduleUpdate();
    if (m_levelService) {
        m_levelService->stopService();
    }
    this->restoreAudio();
    CCLayer::onExit();
}

void ScreensaverLayer::restoreAudio() {
    if (!m_audioCaptured) {
        return;
    }
    auto audio = FMODAudioEngine::get();
    if (!audio) {
        return;
    }
    audio->setBackgroundMusicVolume(m_savedMusic);
    audio->setEffectsVolume(m_savedSfx);
    m_audioCaptured = false;
}

void ScreensaverLayer::beginTransition() {
    if (m_transitioning || !m_fade) {
        return;
    }

    m_transitioning = true;
    auto duration = std::max(0.1f, gdss::settings::transitionSpeed());

    m_fade->stopAllActions();
    m_fade->runAction(cocos2d::CCSequence::create(
        cocos2d::CCFadeTo::create(duration * 0.5f, 255),
        cocos2d::CCCallFunc::create(this, callfunc_selector(ScreensaverLayer::swapToNextLevel)),
        cocos2d::CCFadeTo::create(duration * 0.5f, 0),
        cocos2d::CCCallFunc::create(this, callfunc_selector(ScreensaverLayer::endTransition)),
        nullptr
    ));
}

void ScreensaverLayer::swapToNextLevel() {
    if (!m_levelService || !m_levelService->hasReadyLevel()) {
        return;
    }

    auto level = m_levelService->popReadyLevel();
    if (!level) {
        return;
    }

    if (m_playLayer) {
        m_playLayer->removeFromParentAndCleanup(true);
        m_playLayer = nullptr;
    }

    auto play = PlayLayer::create(level, false, false);
    if (!play) {
        return;
    }

    this->addChild(play, 5);
    m_playLayer = play;

    this->applyPresentationMode(play);

    m_elapsed = 0.f;

    m_cameraX = play->m_cameraPosition.x;
    if (m_cameraX <= 0.f && play->m_player1) {
        m_cameraX = play->m_player1->getPositionX();
    }

    m_cameraY = play->m_cameraPosition.y;
    if (play->m_player1 && m_cameraY <= 0.f) {
        m_cameraY = play->m_player1->getPositionY();
    }

    if (!m_transitioning && m_fade && m_fade->getOpacity() > 0) {
        m_fade->stopAllActions();
        m_fade->runAction(cocos2d::CCFadeTo::create(std::max(0.1f, gdss::settings::transitionSpeed()), 0));
    }

    m_levelService->warmQueue(2);
}

void ScreensaverLayer::endTransition() {
    m_transitioning = false;
}

void ScreensaverLayer::applyPresentationMode(PlayLayer* playLayer) {
    if (!playLayer) {
        return;
    }

    playLayer->toggleIgnoreDamage(true);
    playLayer->toggleHideAttempts(true);
    playLayer->m_isSilent = true;

    if (playLayer->m_progressBar) {
        playLayer->m_progressBar->setVisible(false);
    }
    if (playLayer->m_progressFill) {
        playLayer->m_progressFill->setVisible(false);
    }
    if (playLayer->m_attemptLabel) {
        playLayer->m_attemptLabel->setVisible(false);
    }
    if (playLayer->m_percentageLabel) {
        playLayer->m_percentageLabel->setVisible(false);
    }

    if (playLayer->m_player1) {
        playLayer->m_player1->setVisible(false);
        playLayer->m_player1->setOpacity(0);
    }
    if (playLayer->m_player2) {
        playLayer->m_player2->setVisible(false);
        playLayer->m_player2->setOpacity(0);
    }
}

void ScreensaverLayer::updateCinematicCamera(float dt) {
    if (!m_playLayer) {
        return;
    }

    auto speed = 320.f * gdss::settings::cameraSpeed();
    m_cameraX += speed * dt;

    if (m_playLayer->m_endXPosition > 0.f) {
        m_cameraX = std::min(m_cameraX, m_playLayer->m_endXPosition);
    }

    float sumY = 0.f;
    float minY = 99999.f;
    float maxY = -99999.f;
    int count = 0;

    for (auto* object : m_playLayer->m_activeObjects) {
        if (!object || !object->isVisible()) {
            continue;
        }

        auto pos = object->getPosition();
        if (std::abs(pos.x - m_cameraX) > 550.f) {
            continue;
        }

        sumY += pos.y;
        minY = std::min(minY, pos.y);
        maxY = std::max(maxY, pos.y);
        ++count;
    }

    float targetY = m_cameraY;
    if (count > 0) {
        targetY = sumY / static_cast<float>(count);
        targetY = std::clamp(targetY, minY - 120.f, maxY + 120.f);
    }
    else if (m_playLayer->m_player1) {
        targetY = m_playLayer->m_player1->getPositionY();
    }

    auto winHeight = CCDirector::get()->getWinSize().height;
    targetY = std::clamp(targetY, 40.f, winHeight + 240.f);
    m_cameraY += (targetY - m_cameraY) * std::min(1.f, dt * 2.5f);

    cocos2d::CCPoint cameraPos = {m_cameraX, m_cameraY};
    m_playLayer->m_cameraPosition = cameraPos;
    m_playLayer->m_cameraPosition2 = cameraPos;
    m_playLayer->moveCameraToPos(cameraPos);

    if (m_playLayer->m_player1) {
        m_playLayer->m_player1->setVisible(false);
        m_playLayer->m_player1->setOpacity(0);
    }
    if (m_playLayer->m_player2) {
        m_playLayer->m_player2->setVisible(false);
        m_playLayer->m_player2->setOpacity(0);
    }
}

void ScreensaverLayer::update(float dt) {
    if (!m_levelService) {
        return;
    }

    m_levelService->setFeaturedEpicOnly(gdss::settings::featuredEpicOnly());
    m_levelService->warmQueue(2);

    if (!m_playLayer) {
        if (m_levelService->hasReadyLevel()) {
            this->swapToNextLevel();
        }
        return;
    }

    m_elapsed += dt;
    this->updateCinematicCamera(dt);

    if (m_elapsed >= 20.f && m_levelService->hasReadyLevel() && !m_transitioning) {
        this->beginTransition();
    }
}

bool ScreensaverLayer::ccTouchBegan(cocos2d::CCTouch*, cocos2d::CCEvent*) {
    gdss::ScreensaverController::get().deactivate();
    return true;
}

void ScreensaverLayer::keyDown(cocos2d::enumKeyCodes, double) {
    gdss::ScreensaverController::get().deactivate();
}
