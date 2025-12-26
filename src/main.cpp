#include <Geode/Geode.hpp>
#include <Geode/utils/web.hpp>
#include <Geode/loader/Event.hpp> 
#include <matjson.hpp>
#include <Geode/modify/LevelInfoLayer.hpp>

using namespace geode::prelude;

class $modify(MyLevelInfoLayer, LevelInfoLayer) {
    struct Fields {
        EventListener<web::WebTask> m_listener;
        int m_count = -1;
        std::vector<int> m_listIds;
        bool m_failed = false;
        bool m_alertLoadingShown = false;
        Ref<FLAlertLayer> m_alert;
    };

    bool init(GJGameLevel* level, bool challenge) {
        if (!LevelInfoLayer::init(level, challenge)) {
            return false;
        }

        if (auto leftMenu = this->getChildByID("left-side-menu")) {
            auto btnSprite = CCSprite::createWithSpriteFrameName("GJ_viewListsBtn_001.png");
            auto btn = CCMenuItemSpriteExtra::create(btnSprite, this, menu_selector(MyLevelInfoLayer::onListCounterButton));
            btn->setID("list-counter"_spr);
            leftMenu->addChild(btn);
            leftMenu->updateLayout();
        }
        return true;
    }

    void downloadLevelCount(GJGameLevel* level) {
        if (!level) return;
        int levelId = level->m_levelID;
        bool includeUnrated = Mod::get()->getSettingValue<bool>("unrated");
        auto url = std::string("https://ihaveawebsiteidea.com/lists/levelcount/") + std::to_string(levelId); // don't hate it's a random domain i have no usage of
        if(includeUnrated){
            url = std::string("https://ihaveawebsiteidea.com/alllists/levelcount/") + std::to_string(levelId); // don't hate it's a random domain i have no usage of
        }

        this->m_fields->m_listener.bind([this](web::WebTask::Event* e) {
            this->m_fields->m_failed = false;

            auto* res = e->getValue();
            if (!res) {
                this->m_fields->m_failed = true;
                this->m_fields->m_count = -1;
                this->m_fields->m_listIds.clear();
                return;
            }

            auto body = res->string().unwrapOr("");
            auto parsed = matjson::parse(body);
            if (!parsed.isOk()) {
                this->m_fields->m_failed = true;
                this->m_fields->m_count = -1;
                this->m_fields->m_listIds.clear();
                return;
            }

            auto json = parsed.unwrap();

            int count = 0;
            std::vector<int> listIds;
            
            if (json.isObject()) {
                auto countRes = json["count"].as<int>();
                count = countRes.unwrapOr(0);
                if (json["list_ids"].isArray()) {
                    auto arrRes = json["list_ids"].asArray();
                    if (arrRes.isOk()) {
                        auto& arr = arrRes.unwrap();
                        for (auto& item : arr) {
                            auto idRes = item.as<int>();
                            if (idRes.isOk()) {
                                listIds.push_back(idRes.unwrap());
                            }
                        }
                    }
                }
            } else {
                this->m_fields->m_failed = true;
                this->m_fields->m_count = -1;
                this->m_fields->m_listIds.clear();
                queueInMainThread([this] { this->onListCounterButton(nullptr); });
            }

            // there is no space on level info layer to fit this label :sob: lmao
            auto text = std::string("Lists: ") + std::to_string(count);
            this->m_fields->m_count = count;
            this->m_fields->m_listIds = std::move(listIds);
            if (this->m_fields->m_alertLoadingShown) {
                this->m_fields->m_alertLoadingShown = false;
                queueInMainThread([this] {
                    if (this->m_fields->m_alert) {
                        this->m_fields->m_alert->removeFromParentAndCleanup(true);
                        this->m_fields->m_alert = nullptr;
                    }
                    this->onListCounterButton(nullptr);
                });
            }
        });
        auto req = web::WebRequest();
        this->m_fields->m_listener.setFilter(req.get(url));
    }

    void onListCounterButton(CCObject*) {
        int count = this->m_fields->m_count;
        const auto& ids = this->m_fields->m_listIds;
        std::string msg;
        if (this->m_fields->m_failed) {
            msg = "Error loading lists. Try again later.";
        } else if (count < 0) {
            msg = "Loading...";
            if (this->m_level) {
                this->downloadLevelCount(this->m_level);
            }
            this->m_fields->m_alertLoadingShown = true;
        } else {
            // i will be making id's clickable (some day :sob:)
            msg = std::string("Lists: ") + std::to_string(count);
            if (!ids.empty()) {
                msg += "\nIDs: ";
                for (size_t i = 0; i < ids.size(); ++i) {
                    msg += std::to_string(ids[i]);
                    if (i + 1 < ids.size()) msg += ", ";
                }
            } else {
                msg += "\nIDs: None";
            }
        }
        auto alert = FLAlertLayer::create("List Counter", msg.c_str(), "OK");
        alert->show();
        this->m_fields->m_alert = alert;
    }
};