#include <Geode/Geode.hpp>
#include <Geode/utils/web.hpp>
#include <Geode/loader/Event.hpp>
#include <matjson.hpp>
using namespace geode::prelude;

#include <Geode/modify/LevelInfoLayer.hpp>
class $modify(MyLevelInfoLayer, LevelInfoLayer) {
    struct Fields {
        EventListener<web::WebTask> m_listener;
        int m_count = -1;
        std::vector<int> m_listIds;
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

        downloadLevelCount(level);
        return true;
    }

    void downloadLevelCount(GJGameLevel* level) {
        if (!level) return;
        int levelId = level->m_levelID;
        auto url = std::string("https://ihaveawebsiteidea.com/lists/levelnumber/") + std::to_string(levelId); // don't hate it's a random domain i have no usage of

        this->m_fields->m_listener.bind([this](web::WebTask::Event* e) {
            if (auto* res = e->getValue()) {
                auto body = res->string().unwrapOr("");
                auto parsed = matjson::parse(body);
                
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
                }
                // there is no space on level info layer to fit this label :sob: lmao
                auto text = std::string("Lists: ") + std::to_string(count);
                this->m_fields->m_count = count;
                this->m_fields->m_listIds = std::move(listIds);
            }
        });
        auto req = web::WebRequest();
        this->m_fields->m_listener.setFilter(req.get(url));
    }

    void onListCounterButton(CCObject*) {
        int count = this->m_fields->m_count;
        const auto& ids = this->m_fields->m_listIds;
        std::string msg;
        if (count < 0) {
            msg = "Wait a second";
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
        FLAlertLayer::create("List Counter", msg.c_str(), "OK")->show();
    }
};