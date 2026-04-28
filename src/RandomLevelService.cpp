#include "RandomLevelService.hpp"

#include <Geode/Geode.hpp>

#include <algorithm>
#include <sstream>

using namespace geode::prelude;

RandomLevelService* RandomLevelService::create() {
    auto ret = new RandomLevelService();
    if (ret && ret->init()) {
        ret->autorelease();
        return ret;
    }
    delete ret;
    return nullptr;
}

bool RandomLevelService::init() {
    return CCNode::init();
}

void RandomLevelService::setFeaturedEpicOnly(bool value) {
    m_featuredEpicOnly = value;
}

bool RandomLevelService::hasReadyLevel() const {
    return !m_readyLevels.empty();
}

Ref<GJGameLevel> RandomLevelService::popReadyLevel() {
    if (m_readyLevels.empty()) {
        return nullptr;
    }
    auto level = m_readyLevels.front();
    m_readyLevels.pop_front();
    return level;
}

void RandomLevelService::rememberSeen(int levelID) {
    m_recentIDs.insert(levelID);
    m_recentOrder.push_back(levelID);
    while (m_recentOrder.size() > 600) {
        auto old = m_recentOrder.front();
        m_recentOrder.pop_front();
        m_recentIDs.erase(old);
    }
}

std::string RandomLevelService::buildRandomIDList(size_t count) const {
    std::ostringstream out;
    for (size_t i = 0; i < count; ++i) {
        if (i > 0) {
            out << ',';
        }
        out << utils::random::generate(128, 130000000);
    }
    return out.str();
}

GJGameLevel* RandomLevelService::buildLevelFromEncoded(std::string const& encoded) const {
    if (encoded.empty()) {
        return nullptr;
    }
    auto dict = cocos2d::CCDictionary::create();
    auto parts = utils::string::split(encoded, ":");
    if (parts.size() < 2) {
        return nullptr;
    }
    for (size_t i = 0; i + 1 < parts.size(); i += 2) {
        if (parts[i].empty()) {
            continue;
        }
        dict->setObject(cocos2d::CCString::create(parts[i + 1]), parts[i].c_str());
    }
    return GJGameLevel::create(dict, false);
}

bool RandomLevelService::passesFilters(GJGameLevel* level) const {
    if (!level) {
        return false;
    }
    if (level->m_levelID.value() <= 0) {
        return false;
    }
    if (level->m_stars.value() <= 0) {
        return false;
    }
    if (level->isPlatformer()) {
        return false;
    }
    if (level->m_levelLength < 1) {
        return false;
    }
    if (level->m_autoLevel) {
        return false;
    }
    if (level->m_downloads < 300) {
        return false;
    }
    if (m_featuredEpicOnly && level->m_featured <= 0 && level->m_isEpic <= 0) {
        return false;
    }
    return true;
}

std::vector<Ref<GJGameLevel>> RandomLevelService::parseCandidates(std::string const& response) const {
    std::vector<Ref<GJGameLevel>> out;
    if (response.empty() || response == "-1") {
        return out;
    }

    auto parts = utils::string::split(response, "#");
    if (parts.empty() || parts[0] == "-1") {
        return out;
    }

    auto encodedLevels = utils::string::split(parts[0], "|");
    for (auto const& encoded : encodedLevels) {
        auto level = this->buildLevelFromEncoded(encoded);
        if (!this->passesFilters(level)) {
            continue;
        }
        out.emplace_back(level);
    }

    for (size_t i = 0; i < out.size(); ++i) {
        if (out.empty()) {
            break;
        }
        auto a = static_cast<int>(i);
        auto b = static_cast<int>(out.size() - 1);
        auto j = utils::random::generate(a, b);
        std::swap(out[i], out[static_cast<size_t>(j)]);
    }

    return out;
}

void RandomLevelService::requestCandidateBatch() {
    if (m_requestInFlight) {
        return;
    }
    m_requestInFlight = true;

    auto req = web::WebRequest();
    req.userAgent("");
    req.header("Content-Type", "application/x-www-form-urlencoded");
    req.bodyString(fmt::format("str={}&type=19&secret=Wmfd2893gb7", this->buildRandomIDList(180)));

    m_requestTask.spawn(req.post("http://www.boomlings.com/database/getGJLevels21.php"), [this](web::WebResponse response) {
        m_requestInFlight = false;
        this->onCandidateBatch(response);
    });
}

void RandomLevelService::onCandidateBatch(web::WebResponse response) {
    if (!response.ok()) {
        return;
    }
    auto text = response.string();
    if (!text) {
        return;
    }

    auto levels = this->parseCandidates(text.unwrap());
    for (auto const& level : levels) {
        auto levelID = level->m_levelID.value();
        if (levelID <= 0 || m_recentIDs.contains(levelID)) {
            continue;
        }
        m_candidateIDs.push_back(levelID);
        this->rememberSeen(levelID);
    }

    this->warmQueue(2);
}

void RandomLevelService::startNextDownload() {
    if (m_pendingDownloadID != 0 || m_candidateIDs.empty()) {
        return;
    }

    auto glm = GameLevelManager::get();
    if (!glm) {
        return;
    }

    while (!m_candidateIDs.empty()) {
        auto levelID = m_candidateIDs.front();
        m_candidateIDs.pop_front();
        if (levelID <= 0) {
            continue;
        }
        if (!m_prevDownloadDelegate) {
            m_prevDownloadDelegate = glm->m_levelDownloadDelegate;
        }
        glm->m_levelDownloadDelegate = this;
        m_pendingDownloadID = levelID;
        glm->downloadLevel(levelID, false, 0);
        return;
    }
}

void RandomLevelService::warmQueue(size_t targetSize) {
    if (targetSize == 0) {
        return;
    }

    while (m_readyLevels.size() + (m_pendingDownloadID != 0 ? 1 : 0) < targetSize) {
        if (!m_candidateIDs.empty()) {
            this->startNextDownload();
            if (m_pendingDownloadID != 0) {
                return;
            }
            continue;
        }

        if (!m_requestInFlight) {
            this->requestCandidateBatch();
        }
        return;
    }
}

void RandomLevelService::levelDownloadFinished(GJGameLevel* level) {
    if (!level) {
        m_pendingDownloadID = 0;
        this->warmQueue(2);
        return;
    }

    auto levelID = level->m_levelID.value();
    if (m_pendingDownloadID != 0 && levelID != m_pendingDownloadID) {
        return;
    }

    m_pendingDownloadID = 0;

    if (this->passesFilters(level) && !level->m_levelString.empty()) {
        m_readyLevels.emplace_back(level);
    }

    this->warmQueue(2);
}

void RandomLevelService::levelDownloadFailed(int) {
    m_pendingDownloadID = 0;
    this->warmQueue(2);
}

void RandomLevelService::stopService() {
    m_requestInFlight = false;
    m_pendingDownloadID = 0;
    m_candidateIDs.clear();
    m_readyLevels.clear();

    auto glm = GameLevelManager::get();
    if (glm && glm->m_levelDownloadDelegate == this) {
        glm->m_levelDownloadDelegate = m_prevDownloadDelegate;
    }
    m_prevDownloadDelegate = nullptr;
}
