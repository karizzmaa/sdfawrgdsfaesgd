#pragma once

#include <Geode/Geode.hpp>
#include <Geode/utils/web.hpp>

#include <deque>
#include <string>
#include <unordered_set>
#include <vector>

using namespace geode::prelude;

class RandomLevelService : public cocos2d::CCNode, public LevelDownloadDelegate {
public:
    static RandomLevelService* create();
    bool init() override;

    void setFeaturedEpicOnly(bool value);
    void warmQueue(size_t targetSize);
    bool hasReadyLevel() const;
    geode::Ref<GJGameLevel> popReadyLevel();
    void stopService();

    void levelDownloadFinished(GJGameLevel* level) override;
    void levelDownloadFailed(int response) override;

private:
    void requestCandidateBatch();
    void onCandidateBatch(web::WebResponse response);
    void startNextDownload();
    bool passesFilters(GJGameLevel* level) const;
    std::vector<geode::Ref<GJGameLevel>> parseCandidates(std::string const& response) const;
    GJGameLevel* buildLevelFromEncoded(std::string const& encoded) const;
    std::string buildRandomIDList(size_t count) const;
    void rememberSeen(int levelID);

    bool m_featuredEpicOnly = false;
    bool m_requestInFlight = false;
    int m_pendingDownloadID = 0;
    std::deque<int> m_candidateIDs;
    std::deque<geode::Ref<GJGameLevel>> m_readyLevels;
    std::unordered_set<int> m_recentIDs;
    std::deque<int> m_recentOrder;
    LevelDownloadDelegate* m_prevDownloadDelegate = nullptr;
    geode::async::TaskHolder<web::WebResponse> m_requestTask;
};
