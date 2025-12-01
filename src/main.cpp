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
                if (json.isObject()) {
                    auto countRes = json["count"].as<int>();
                    count = countRes.unwrapOr(0);
                }
                // there is no space on level info layer to fit this label :sob: lmao
                auto text = std::string("Lists: ") + std::to_string(count);
                this->m_fields->m_count = count;
            }
        });
        auto req = web::WebRequest();
        this->m_fields->m_listener.setFilter(req.get(url));
    }

    void onListCounterButton(CCObject*) {
        int count = this->m_fields->m_count;
        std::string msg;
        if (count < 0) {
            msg = "Wait a second";
        } else {
            msg = std::string("Lists: ") + std::to_string(count);
        }
        FLAlertLayer::create("List Counter", msg.c_str(), "OK")->show();
    }
};